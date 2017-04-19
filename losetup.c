
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/loop.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "busybox.h"

static char dospfx[] = "\\DosDevices\\";

static void
fixsep(char *path)
{
	char *p;

	do {
		p = strchr(path, '/');
		if (p)
			*p++ = '\\';
	} while ((path = p));
}

int
losetup_main (int argc, char **argv)
{
	int newfile, delete;
	char *offset, *fssize;
	char *rpath;
	struct loopsetup loset;
	int ch, fd, rv;

	newfile = delete = 0;
	offset = fssize = NULL;
	bzero(&loset, sizeof (struct loopsetup));
	while ((ch = getopt(argc, argv, "ndo:s:")) != -1)
		switch (ch) {
		case 'd':
			delete++;
			break;
		case 'n':
			newfile++;
			break;
		case 'o':
			offset = optarg;
			loset.offset = strtoll(offset, NULL, 0);
			break;
		case 's':
			fssize = optarg;
			loset.size = strtoll(fssize, NULL, 0);
			break;
		default:
			show_usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;
	if (delete) {
 		if (!argc || offset || fssize || newfile)
			show_usage();

		loset.loname = basename(*argv++);
		fd = open("/dev/loop", O_RDONLY);
		if (fd < 0)
			perror_msg_and_die("/dev/loop");
		rv = ioctl(fd, LOOP_DEL_FILE, &loset);
		if (rv < 0)
			perror_msg_and_die("ioctl");
		return EXIT_SUCCESS;
	}

	if (argc < 2 || (newfile && !fssize))
		show_usage();

	loset.loname = *argv++;
	if (strncmp(loset.loname, "/dev/", 5) == 0)
		loset.loname += 5;
	loset.filename = *argv;
	fixsep(loset.filename);
	if (!fssize) {
		struct _stat sb;

		if (_stat(loset.filename, &sb) < 0)
			perror_msg_and_die(loset.filename);
		loset.size = sb.st_size;
	}
	if (strncmp(loset.filename, dospfx, sizeof (dospfx)-1)) {
		rpath = xmalloc(strlen(loset.filename) + sizeof (dospfx));
		strcpy(rpath, dospfx);
		strcat(rpath, loset.filename);
		loset.filename = rpath;
	}
	if (newfile) {
		int fh = devcreate(loset.filename, loset.size);
		if (fh == -1)
			error_msg_and_die("cannot create file %s\n", loset.filename);
	}
	
	fd = open("/dev/loop", O_RDONLY);
	if (fd < 0)
		perror_msg_and_die("/dev/loop");
	rv =  ioctl(fd, LOOP_SET_FILE, &loset);
	close(fd);
	if (rv < 0)
		perror_msg_and_die("ioctl");
	return EXIT_SUCCESS;
}
