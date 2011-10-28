/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <gtk/gtk.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "coverart.h"
#include "../artwork/artwork.h"
#include "gtkui.h"

static GtkWidget *artworkcont;

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(...)

extern DB_artwork_plugin_t *coverart_plugin;

#define MAX_ID 256
#define CACHE_SIZE 20

typedef struct {
    struct timeval tm;
    char *fname;
    time_t filetime;
    int width;
    GdkPixbuf *pixbuf;
} cached_pixbuf_t;

typedef struct load_query_s {
    char *fname;
    int width;
    struct load_query_s *next;
} load_query_t;

static cached_pixbuf_t cache[CACHE_SIZE];
static int terminate = 0;
static uintptr_t mutex;
static uintptr_t cond;
static uintptr_t tid;
load_query_t *queue;
load_query_t *tail;

static void
queue_add (const char *fname, int width) {
    deadbeef->mutex_lock (mutex);
    load_query_t *q;
    for (q = queue; q; q = q->next) {
        if (!strcmp (q->fname, fname) && width == q->width) {
            deadbeef->mutex_unlock (mutex);
            return; // dupe
        }
    }
    q = malloc (sizeof (load_query_t));
    memset (q, 0, sizeof (load_query_t));
    q->fname = strdup (fname);
    q->width = width;
    if (tail) {
        tail->next = q;
        tail = q;
    }
    else {
        queue = tail = q;
    }
    deadbeef->mutex_unlock (mutex);
    deadbeef->cond_signal (cond);
}

static void
queue_pop (void) {
    deadbeef->mutex_lock (mutex);
    load_query_t *next = queue->next;
    if (queue->fname) {
        free (queue->fname);
    }
    free (queue);
    queue = next;
    if (!queue) {
        tail = NULL;
    }
    deadbeef->mutex_unlock (mutex);
}

gboolean
redraw_playlist_cb (gpointer dt) {
    void main_refresh (void);
    main_refresh ();
    return FALSE;
}

void
loading_thread (void *none) {
    for (;;) {
        trace ("covercache: waiting for signal\n");
        deadbeef->cond_wait (cond, mutex);
        trace ("covercache: signal received\n");
        deadbeef->mutex_unlock (mutex);
        while (!terminate && queue) {
            int cache_min = 0;
            deadbeef->mutex_lock (mutex);
            for (int i = 0; i < CACHE_SIZE; i++) {
                if (!cache[i].pixbuf) {
                    cache_min = i;
                    break;
                }
                if (cache[cache_min].pixbuf && cache[i].pixbuf) {
                    if (cache[cache_min].tm.tv_sec > cache[i].tm.tv_sec) {
                        cache_min = i;
                    }
                }
            }
            if (cache_min != -1) {
                if (cache[cache_min].pixbuf) {
                    g_object_unref (cache[cache_min].pixbuf);
                    cache[cache_min].pixbuf = NULL;
                }
                if (cache[cache_min].fname) {
                    free (cache[cache_min].fname);
                    cache[cache_min].fname = NULL;
                }
            }
            deadbeef->mutex_unlock (mutex);
            if (cache_min == -1) {
                trace ("coverart pixbuf cache overflow, waiting...\n");
                usleep (500000);
                continue;
            }
            struct stat stat_buf;
            if (stat (queue->fname, &stat_buf) < 0) {
                trace ("failed to stat file %s\n", queue->fname);
            }
            GdkPixbuf *pixbuf = NULL;
            GError *error = NULL;
            pixbuf = gdk_pixbuf_new_from_file_at_scale (queue->fname, queue->width, queue->width, TRUE, &error);
            if (!pixbuf) {
                unlink (queue->fname);
                fprintf (stderr, "gdk_pixbuf_new_from_file_at_scale %s %d failed, error: %s\n", queue->fname, queue->width, error->message);
                if (error) {
                    g_error_free (error);
                    error = NULL;
                }
                const char *defpath = coverart_plugin->get_default_cover ();
                if (stat (defpath, &stat_buf) < 0) {
                    trace ("failed to stat file %s\n", queue->fname);
                }
                pixbuf = gdk_pixbuf_new_from_file_at_scale (defpath, queue->width, queue->width, TRUE, &error);
                if (!pixbuf) {
                    fprintf (stderr, "gdk_pixbuf_new_from_file_at_scale %s %d failed, error: %s\n", defpath, queue->width, error->message);
                }
            }
            if (error) {
                g_error_free (error);
                error = NULL;
            }
            if (!pixbuf) {
                // make default empty image
                pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 2, 2);
                stat_buf.st_mtime = 0;
            }
            if (cache_min != -1) {
                deadbeef->mutex_lock (mutex);
                cache[cache_min].filetime = stat_buf.st_mtime;
                cache[cache_min].pixbuf = pixbuf;
                cache[cache_min].fname = strdup (queue->fname);
                gettimeofday (&cache[cache_min].tm, NULL);
                cache[cache_min].width = queue->width;
                struct stat stat_buf;
                deadbeef->mutex_unlock (mutex);
            }
            queue_pop ();
            g_idle_add (redraw_playlist_cb, NULL);
        }
        if (terminate) {
            break;
        }
    }
}

void
cover_avail_callback (const char *fname, const char *artist, const char *album, void *user_data) {
    // means requested image is now in disk cache
    // load it into main memory
    GdkPixbuf *pb = get_cover_art (fname, artist, album, (intptr_t)user_data);
    if (pb) {
        g_object_unref (pb);
        // already in cache, redraw
        g_idle_add (redraw_playlist_cb, NULL);
    }
}

static GdkPixbuf *
get_pixbuf (const char *fname, int width) {
    int requested_width = width;
    // find in cache
    deadbeef->mutex_lock (mutex);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            if (!strcmp (fname, cache[i].fname) && cache[i].width == width) {
                // check if cached filetime hasn't changed
                struct stat stat_buf;
                if (!stat (fname, &stat_buf) && stat_buf.st_mtime == cache[i].filetime) {
                    gettimeofday (&cache[i].tm, NULL);
                    GdkPixbuf *pb = cache[i].pixbuf;
                    g_object_ref (pb);
                    deadbeef->mutex_unlock (mutex);
                    return pb;
                }
            }
        }
    }
#if 0
    printf ("cache miss: %s/%d\n", fname, width);
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            printf ("    cache line %d: %s/%d\n", i, cache[i].fname, cache[i].width);
        }
    }
#endif
    deadbeef->mutex_unlock (mutex);
    queue_add (fname, width);
    return NULL;
}

GdkPixbuf *
get_cover_art (const char *fname, const char *artist, const char *album, int width) {
    if (!coverart_plugin) {
        return NULL;
    }
    char *image_fname = coverart_plugin->get_album_art (fname, artist, album, -1, cover_avail_callback, (void *)(intptr_t)width);
    if (image_fname) {
        GdkPixbuf *pb = get_pixbuf (image_fname, width);
        free (image_fname);
        return pb;
    }
    return NULL;
}

void
coverart_reset_queue (void) {
    deadbeef->mutex_lock (mutex);
    if (queue) {
        load_query_t *q = queue->next;
        while (q) {
            load_query_t *next = q->next;
            if (q->fname) {
                free (q->fname);
            }
            free (q);
            q = next;
        }
        queue->next = NULL;
        tail = queue;
    }
    deadbeef->mutex_unlock (mutex);
    if (coverart_plugin) {
        coverart_plugin->reset (1);
    }
}

void
cover_art_init (void) {
    terminate = 0;
    mutex = deadbeef->mutex_create_nonrecursive ();
    cond = deadbeef->cond_create ();
    tid = deadbeef->thread_start_low_priority (loading_thread, NULL);
}

void
cover_art_free (void) {
    trace ("terminating cover art loader...\n");

    if (coverart_plugin) {
        trace ("resetting artwork plugin...\n");
        coverart_plugin->reset (0);
    }
    
    if (tid) {
        terminate = 1;
        trace ("sending terminate signal to art loader thread...\n");
        deadbeef->cond_signal (cond);
        deadbeef->thread_join (tid);
        tid = 0;
    }
    while (queue) {
        queue_pop ();
    }
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].pixbuf) {
            g_object_unref (cache[i].pixbuf);
        }
    }
    memset (cache, 0, sizeof (cache));
    deadbeef->cond_free (cond);
    deadbeef->mutex_free (mutex);
}

void
artwork_window_show (void) {
    if (!artworkcont)
        artworkcont = lookup_widget (mainwin, "img_art");
    gtk_widget_show (artworkcont);

}

void
artwork_window_hide (void) {
    if (!artworkcont)
        artworkcont = lookup_widget (mainwin, "img_art");
    gtk_widget_hide (artworkcont);
}

