/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999,2000,2001 by Erik Andersen <andersee@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "libbb.h"


#ifndef DMALLOC
extern void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL && size != 0)
		error_msg_and_die(memory_exhausted);
	return ptr;
}

extern void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size);
	if (ptr == NULL && size != 0)
		error_msg_and_die(memory_exhausted);
	return ptr;
}

extern void *xcalloc(size_t nmemb, size_t size)
{
	void *ptr = calloc(nmemb, size);
	if (ptr == NULL && nmemb != 0 && size != 0)
		error_msg_and_die(memory_exhausted);
	return ptr;
}

void *xzalloc(size_t size)
{
	void *ptr = xmalloc(size);

	memset(ptr, 0, size);
	return ptr;
}

extern char * xstrdup (const char *s) {
	char *t;

	if (s == NULL)
		return NULL;

	t = strdup (s);

	if (t == NULL)
		error_msg_and_die(memory_exhausted);

	return t;
}
#endif

extern char * xstrndup (const char *s, int n) {
	char *t;

	if (s == NULL)
		error_msg_and_die("xstrndup bug");

	t = xmalloc(++n);
	
	return safe_strncpy(t,s,n);
}

FILE *xfopen(const char *path, const char *mode)
{
	FILE *fp;
	if ((fp = fopen(path, mode)) == NULL)
		perror_msg_and_die("%s", path);
	return fp;
}

extern void xferror(FILE *fp, const char *fn)
{
    if (ferror(fp)) {
        error_msg_and_die("%s", fn);
    }
}


/* Stupid gcc always includes its own builtin strlen()... */
#undef strlen
size_t xstrlen(const char *string)
{
	return(strlen(string));
}

int xmkstemp(char *template)
{
    int fd = mkstemp(template);

    if (fd < 0)
        perror_msg_and_die("can't create temp file '%s'", template);
    return fd;
}

// xstat() - a stat() which dies on failure with meaningful error message
void xstat(const char *name, struct stat *stat_buf)
{
    if (stat(name, stat_buf))
        perror_msg_and_die("can't stat '%s'", name);
}

// Die with an error message if we can't lseek to the right spot.
off_t xlseek(int fd, off_t offset, int whence)
{
    off_t off = lseek(fd, offset, whence);
    if (off == (off_t)-1) {
        if (whence == SEEK_SET)
            perror_msg_and_die("lseek(%lu)", offset);
        perror_msg_and_die("lseek");
    }
    return off;
}

// Die with an error message if we can't malloc() enough space and do an
// sprintf() into that space.
char *xasprintf(const char *format, ...)
{
	va_list p;
	int r;
	char *string_ptr;
	int size;

	va_start(p, format);
	size = _vscprintf(format, p);
	string_ptr = xmalloc(size+1);
	r = _vsnprintf(string_ptr, size+1, format, p);
	va_end(p);

	if (r < 0)
		error_msg_and_die(memory_exhausted);
	return string_ptr;
}

// Warn if we can't open a file and return a fd.
int FAST_FUNC open3_or_warn(const char *pathname, int flags, int mode)
{
	int ret;

	ret = open(pathname, flags, mode);
	if (ret < 0) {
		perror_msg("can't open '%s'", pathname);
	}
	return ret;
}

// Warn if we can't open a file and return a fd.
int FAST_FUNC open_or_warn(const char *pathname, int flags)
{
	return open3_or_warn(pathname, flags, 0666);
}


/* END CODE */
/*
Local Variables:
c-file-style: "linux"
c-basic-offset: 4
tab-width: 4
End:
*/
