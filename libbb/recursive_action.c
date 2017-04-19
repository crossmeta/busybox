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
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>	/* free() */
#include "libbb.h"

#undef DEBUG_RECURS_ACTION


/*
 * Walk down all the directories under the specified 
 * location, and do something (something specified
 * by the fileAction and dirAction function pointers).
 *
 * Unfortunately, while nftw(3) could replace this and reduce 
 * code size a bit, nftw() wasn't supported before GNU libc 2.1, 
 * and so isn't sufficiently portable to take over since glibc2.1
 * is so stinking huge.
 */
int recursive_action(const char *fileName,
					int recurse, int followLinks, int depthFirst,
					int (*fileAction) (const char *fileName,
									   struct stat * statbuf,
									   void* userData),
					int (*dirAction) (const char *fileName,
									  struct stat * statbuf,
									  void* userData),
					void* userData)
{
	int status;
	struct stat statbuf;
	struct dirent *next;

	if (followLinks == TRUE)
		status = stat(fileName, &statbuf);
	else
		status = lstat(fileName, &statbuf);

	if (status < 0) {
#ifdef DEBUG_RECURS_ACTION
		fprintf(stderr,
				"status=%d followLinks=%d TRUE=%d\n",
				status, followLinks, TRUE);
#endif
		perror_msg("%s", fileName);
		return FALSE;
	}

	if ((followLinks == FALSE) && (S_ISLNK(statbuf.st_mode))) {
		if (fileAction == NULL)
			return TRUE;
		else
			return fileAction(fileName, &statbuf, userData);
	}

	if (recurse == FALSE) {
		if (S_ISDIR(statbuf.st_mode)) {
			if (dirAction != NULL)
				return (dirAction(fileName, &statbuf, userData));
			else
				return TRUE;
		}
	}

	if (S_ISDIR(statbuf.st_mode)) {
		DIR *dir;

		if (dirAction != NULL && depthFirst == FALSE) {
			status = dirAction(fileName, &statbuf, userData);
			if (status == FALSE) {
				perror_msg("%s", fileName);
				return FALSE;
			} else if (status == SKIP)
				return TRUE;
		}
		dir = opendir(fileName);
		if (!dir) {
			perror_msg("%s", fileName);
			return FALSE;
		}
		status = TRUE;
		while ((next = readdir(dir)) != NULL) {
			char *nextFile;

			if ((strcmp(next->d_name, "..") == 0)
					|| (strcmp(next->d_name, ".") == 0)) {
				continue;
			}
			nextFile = concat_path_file(fileName, next->d_name);
			if (recursive_action(nextFile, TRUE, followLinks, depthFirst,
						fileAction, dirAction, userData) == FALSE) {
				status = FALSE;
			}
			free(nextFile);
		}
		closedir(dir);
		if (dirAction != NULL && depthFirst == TRUE) {
			if (dirAction(fileName, &statbuf, userData) == FALSE) {
				perror_msg("%s", fileName);
				return FALSE;
			}
		}
		if (status == FALSE)
			return FALSE;
	} else {
		if (fileAction == NULL)
			return TRUE;
		else
			return fileAction(fileName, &statbuf, userData);
	}
	return TRUE;
}


/* END CODE */
/*
Local Variables:
c-file-style: "linux"
c-basic-offset: 4
tab-width: 4
End:
*/
