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
#include <string.h>
#include "vfs.h"
#include "plugins.h"

DB_FILE *
vfs_fopen (const char *fname) {
    DB_vfs_t **plugs = plug_get_vfs_list ();
    DB_vfs_t *fallback = NULL;
    int i;
    for (i = 0; plugs[i]; i++) {
        DB_vfs_t *p = plugs[i];
        if (!p->scheme_names) {
            fallback = p;
            continue;
        }
        int n;
        for (n = 0; p->scheme_names[n]; n++) {
            size_t l = strlen (p->scheme_names[n]);
            if (!strncasecmp (p->scheme_names[n], fname, l)) {
                return p->open (fname);
            }
        }
    }
    if (fallback) {
        return fallback->open (fname);
    }
    return NULL;
}

void
vfs_fclose (DB_FILE *stream) {
    return stream->vfs->close (stream);
}

size_t
vfs_fread (void *ptr, size_t size, size_t nmemb, DB_FILE *stream) {
    return stream->vfs->read (ptr, size, nmemb, stream);
}

int
vfs_fseek (DB_FILE *stream, long offset, int whence) {
    return stream->vfs->seek (stream, offset, whence);
}

long
vfs_ftell (DB_FILE *stream) {
    return stream->vfs->tell (stream);
}

void
vfs_rewind (DB_FILE *stream) {
    stream->vfs->rewind (stream);
}

