/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include "deadbeef.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static DB_functions_t *deadbeef;
typedef struct {
    DB_vfs_t *vfs;
    FILE *stream;
} STDIO_FILE;

static DB_vfs_t plugin;

DB_FILE *
stdio_open (const char *fname) {
    FILE *file = fopen (fname, "rb");
    if (!file) {
        return NULL;
    }
    STDIO_FILE *fp = malloc (sizeof (STDIO_FILE));
    fp->vfs = &plugin;
    fp->stream = file;
    return (DB_FILE*)fp;
}

void
stdio_close (DB_FILE *stream) {
    assert (stream);
    fclose (((STDIO_FILE *)stream)->stream);
    free (stream);
}

size_t stdio_read (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    assert (stream);
    assert (ptr);
    return fread (ptr, size, nmemb, ((STDIO_FILE *)stream)->stream);
}

int
stdio_seek (DB_FILE *stream, long offset, int whence) {
    assert (stream);
    return fseek (((STDIO_FILE *)stream)->stream, offset, whence);
}

long
stdio_tell (DB_FILE *stream) {
    assert (stream);
    return ftell (((STDIO_FILE *)stream)->stream);
}

void
stdio_rewind (DB_FILE *stream) {
    assert (stream);
    rewind (((STDIO_FILE *)stream)->stream);
}

// standard stdio vfs
static DB_vfs_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_VFS,
    .plugin.name = "STDIO VFS",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .open = stdio_open,
    .close = stdio_close,
    .read = stdio_read,
    .seek = stdio_seek,
    .tell = stdio_tell,
    .rewind = stdio_rewind,
    .scheme_names = NULL // this is NULL because that's a fallback vfs, used when no other matching vfs plugin found
};

DB_plugin_t *
stdio_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
