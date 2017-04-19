/* vi: set sw=4 ts=4: */
/*
 * Copyright (c) 1980, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1980, 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)df.c	8.9 (Berkeley) 5/8/95";
#else
static const char rcsid[] =
  "$FreeBSD: src/bin/df/df.c,v 1.23.2.1 2000/06/13 03:19:40 jwd Exp $";
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>
#if 0
#include <ufs/ufs/ufsmount.h>
#endif

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include "busybox.h"

#define UNITS_SI 1
#define UNITS_2 2

#define KILO_SZ(n) (n)
#define MEGA_SZ(n) ((n) * (n))
#define GIGA_SZ(n) ((n) * (n) * (n))
#define TERA_SZ(n) ((n) * (n) * (n) * (n))
#define PETA_SZ(n) ((n) * (n) * (n) * (n) * (n))

#define KILO_2_SZ (KILO_SZ((int64_t)1024))
#define MEGA_2_SZ (MEGA_SZ((int64_t)1024))
#define GIGA_2_SZ (GIGA_SZ((int64_t)1024))
#define TERA_2_SZ (TERA_SZ((int64_t)1024))
#define PETA_2_SZ (PETA_SZ((int64_t)1024))

#define KILO_SI_SZ (KILO_SZ((int64_t)1000))
#define MEGA_SI_SZ (MEGA_SZ((int64_t)1000))
#define GIGA_SI_SZ (GIGA_SZ((int64_t)1000))
#define TERA_SI_SZ (TERA_SZ((int64_t)1000))
#define PETA_SI_SZ (PETA_SZ((int64_t)1000))

int64_t vals_si [] = {1, KILO_SI_SZ, MEGA_SI_SZ, GIGA_SI_SZ, TERA_SI_SZ, PETA_SI_SZ};
int64_t vals_base2[] = {1, KILO_2_SZ, MEGA_2_SZ, GIGA_2_SZ, TERA_2_SZ, PETA_2_SZ};
int64_t *valp;

typedef enum { NONE, KILO, MEGA, GIGA, TERA, PETA, UNIT_MAX } unit_t;

int unitp [] = { NONE, KILO, MEGA, GIGA, TERA, PETA };

int	  checkvfsname __P((const char *, char **));
char	**makevfslist __P((char *));
long	  regetmntinfo __P((struct statfs **, long, char **));
int	  bread __P((off_t, void *, int));
char	 *getmntpt __P((char *));
void	  prthuman __P((struct statfs *, long));
void	  prthumanval __P((double));
void	  prtstat __P((struct statfs *, int));
int	  ufs_df __P((char *, int));
unit_t	  unit_adjust __P((double *));
int 	  ilogb(double);

int	aflag = 0, hflag, iflag, nflag;
struct	ufs_args mdev;

int
df_main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat stbuf;
	struct statfs statfsbuf, *mntbuf;
	long mntsize;
	int ch, err, i, maxwidth, rv, width;
	char *mntpt, *mntpath, **vfslist;

	vfslist = NULL;
	while ((ch = getopt(argc, argv, "abgHhikmnPt:")) != -1)
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		case 'b':
				/* FALLTHROUGH */
		case 'P':
			putenv("BLOCKSIZE=512");
			hflag = 0;
			break;
		case 'g':
			putenv("BLOCKSIZE=1g");
			hflag = 0;
			break;
		case 'H':
			hflag = UNITS_SI;
			valp = vals_si;
			break;
		case 'h':
			hflag = UNITS_2;
			valp = vals_base2;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'k':
			putenv("BLOCKSIZE=1k");
			hflag = 0;
			break;
		case 'm':
			putenv("BLOCKSIZE=1m");
			hflag = 0;
			break;
		case 'n':
			nflag = 1;
			break;
		case 't':
			if (vfslist != NULL)
				error_msg_and_die("only one -t option may be specified.");
			vfslist = makevfslist(optarg);
			break;
		case '?':
		default:
			show_usage();
		}
	argc -= optind;
	argv += optind;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	maxwidth = 0;
	for (i = 0; i < mntsize; i++) {
		width = strlen(mntbuf[i].f_mntfromname);
		if (width > maxwidth)
			maxwidth = width;
	}

	rv = 0;
	if (!*argv) {
		mntsize = regetmntinfo(&mntbuf, mntsize, vfslist);
		if (vfslist != NULL) {
			maxwidth = 0;
			for (i = 0; i < mntsize; i++) {
				width = strlen(mntbuf[i].f_mntfromname);
				if (width > maxwidth)
					maxwidth = width;
			}
		}
		for (i = 0; i < mntsize; i++) {
			if (aflag || (mntbuf[i].f_flags & MNT_IGNORE) == 0)
				prtstat(&mntbuf[i], maxwidth);
		}
		return (rv? EXIT_FAILURE : EXIT_SUCCESS);
	}

	for (; *argv; argv++) {
		if (stat(*argv, &stbuf) < 0) {
			err = errno;
			if ((mntpt = getmntpt(*argv)) == 0) {
				warn("%s", *argv);
				rv = 1;
				continue;
			}
#ifdef NOT_SUPPORTED
		} else if ((stbuf.st_mode & S_IFMT) == S_IFCHR) {
			rv = ufs_df(*argv, maxwidth) || rv;
			continue;
		} else if ((stbuf.st_mode & S_IFMT) == S_IFBLK) {
			if ((mntpt = getmntpt(*argv)) == 0) {
				mdev.fspec = *argv;
				mntpath = strdup("/tmp/df.XXXXXX");
				if (mntpath == NULL) {
					warn("strdup failed");
					rv = 1;
					continue;
				}
				mntpt = mkdtemp(mntpath);
				if (mntpt == NULL) {
					warn("mkdtemp(\"%s\") failed", mntpath);
					rv = 1;
					free(mntpath);
					continue;
				}
				if (mount("ufs", mntpt, MNT_RDONLY,
				    &mdev) != 0) {
					rv = ufs_df(*argv, maxwidth) || rv;
					(void)rmdir(mntpt);
					free(mntpath);
					continue;
				} else if (statfs(mntpt, &statfsbuf) == 0) {
					statfsbuf.f_mntonname[0] = '\0';
					prtstat(&statfsbuf, maxwidth);
				} else {
					warn("%s", *argv);
					rv = 1;
				}
				(void)unmount(mntpt, 0);
				(void)rmdir(mntpt);
				free(mntpath);
				continue;
			}
#endif /* NOT_SUPPORTED */
		} else
			mntpt = *argv;
		/*
		 * Statfs does not take a `wait' flag, so we cannot
		 * implement nflag here.
		 */
		if (statfs(mntpt, &statfsbuf) < 0) {
			warn("%s", mntpt);
			rv = 1;
			continue;
		}
		if (argc == 1)
			maxwidth = strlen(statfsbuf.f_mntfromname) + 1;
		prtstat(&statfsbuf, maxwidth);
	}
	return (rv);
}

char *
getmntpt(name)
	char *name;
{
	long mntsize, i;
	struct statfs *mntbuf;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = 0; i < mntsize; i++) {
		if (!strcmp(mntbuf[i].f_mntfromname, name))
			return (mntbuf[i].f_mntonname);
	}
	return (0);
}

/*
 * Make a pass over the filesystem info in ``mntbuf'' filtering out
 * filesystem types not in vfslist and possibly re-stating to get
 * current (not cached) info.  Returns the new count of valid statfs bufs.
 */
long
regetmntinfo(mntbufp, mntsize, vfslist)
	struct statfs **mntbufp;
	long mntsize;
	char **vfslist;
{
	int i, j;
	struct statfs *mntbuf;

	if (vfslist == NULL)
		return (nflag ? mntsize : getmntinfo(mntbufp, MNT_WAIT));

	mntbuf = *mntbufp;
	for (j = 0, i = 0; i < mntsize; i++) {
		if (checkvfsname(mntbuf[i].f_fstypename, vfslist))
			continue;
		if (!nflag)
			(void)statfs(mntbuf[i].f_mntonname,&mntbuf[j]);
		else if (i != j)
			mntbuf[j] = mntbuf[i];
		j++;
	}
	return (j);
}

/*
 * Output in "human-readable" format.  Uses 3 digits max and puts
 * unit suffixes at the end.  Makes output compact and easy to read,
 * especially on huge disks.
 *
 */
unit_t
unit_adjust(val)
	double *val;
{
	double abval;
	unit_t unit;
	unsigned int unit_sz;

	abval = fabs(*val);

	unit_sz = abval ? ilogb(abval) / 10 : 0;

	if (unit_sz >= UNIT_MAX) {
		unit = NONE;
	} else {
		unit = unitp[unit_sz];
		*val /= (double)valp[unit_sz];
	}

	return (unit);
}

void
prthuman(sfsp, used)
	struct statfs *sfsp;
	long used;
{

	prthumanval((double)(int64_t)sfsp->f_blocks * (double)(int64_t)sfsp->f_bsize);
	prthumanval((double)(int64_t)used * (double)sfsp->f_bsize);
	prthumanval((double)(int64_t)sfsp->f_bavail * (double)sfsp->f_bsize);
}

void
prthumanval(bytes)
	double bytes;
{

	unit_t unit;
	unit = unit_adjust(&bytes);

	if (bytes == 0)
		(void)printf("     0B");
	else if (bytes > 10)
		(void)printf(" %5.0f%c", bytes, "BKMGTPE"[unit]);
	else
		(void)printf(" %5.1f%c", bytes, "BKMGTPE"[unit]);
}

/*
 * Convert statfs returned filesystem size into BLOCKSIZE units.
 * Attempts to avoid overflow for large filesystems.
 */
#define fsbtoblk(num, fsbs, bs) \
	(((fsbs) != 0 && (fsbs) < (bs)) ? \
		(num) / ((bs) / (fsbs)) : (num) * ((fsbs) / (bs)))

/*
 * Print out status about a filesystem.
 */
void
prtstat(sfsp, maxwidth)
	struct statfs *sfsp;
	int maxwidth;
{
	static long blocksize;
	static int headerlen, timesthrough;
	static char *header;
	long used, availblks, inodes;

	if (maxwidth < 11)
		maxwidth = 11;
	if (++timesthrough == 1) {
		if (hflag) {
			header = "  Size";
			headerlen = strlen(header);
			(void)printf("%-*.*s %-s   Used  Avail Capacity",
				maxwidth, maxwidth, "Filesystem", header);
		} else {
			header = getbsize(&headerlen, &blocksize);
			(void)printf("%-*.*s %-s     Used    Avail Capacity",
				maxwidth, maxwidth, "Filesystem", header);
		}
		if (iflag)
			(void)printf(" iused   ifree  %%iused");
		(void)printf("  Mounted on\n");
	}
	(void)printf("%-*.*s", maxwidth, maxwidth, sfsp->f_mntfromname);
	used = sfsp->f_blocks - sfsp->f_bfree;
	availblks = sfsp->f_bavail + used;
	if (hflag) {
		prthuman(sfsp, used);
	} else {
		(void)printf(" %*ld %8ld %8ld", headerlen,
	            fsbtoblk(sfsp->f_blocks, sfsp->f_bsize, blocksize),
	            fsbtoblk(used, sfsp->f_bsize, blocksize),
	            fsbtoblk(sfsp->f_bavail, sfsp->f_bsize, blocksize));
	}
	(void)printf(" %5.0f%%",
	    availblks == 0 ? 100.0 : (double)used / (double)availblks * 100.0);
	if (iflag) {
		inodes = sfsp->f_files;
		used = inodes - sfsp->f_ffree;
		(void)printf(" %7ld %7ld %5.0f%% ", used, sfsp->f_ffree,
		   inodes == 0 ? 100.0 : (double)used / (double)inodes * 100.0);
	} else
		(void)printf("  ");
	(void)printf("  %s\n", sfsp->f_mntonname);
}

#if NO_LEGACY_HERE
/*
 * This code constitutes the pre-system call Berkeley df code for extracting
 * information from filesystem superblocks.
 */
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <errno.h>
#include <fstab.h>

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define sblock sb.iu_fs

int	rfd;

int
ufs_df(file, maxwidth)
	char *file;
	int maxwidth;
{
	struct statfs statfsbuf;
	struct statfs *sfsp;
	char *mntpt;
	static int synced;

	if (synced++ == 0)
		sync();

	if ((rfd = open(file, O_RDONLY)) < 0) {
		warn("%s", file);
		return (1);
	}
	if (bread((off_t)SBOFF, &sblock, SBSIZE) == 0) {
		(void)close(rfd);
		return (1);
	}
	sfsp = &statfsbuf;
	sfsp->f_type = 1;
	strcpy(sfsp->f_fstypename, "ufs");
	sfsp->f_flags = 0;
	sfsp->f_bsize = sblock.fs_fsize;
	sfsp->f_iosize = sblock.fs_bsize;
	sfsp->f_blocks = sblock.fs_dsize;
	sfsp->f_bfree = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
		sblock.fs_cstotal.cs_nffree;
	sfsp->f_bavail = freespace(&sblock, sblock.fs_minfree);
	sfsp->f_files =  sblock.fs_ncg * sblock.fs_ipg;
	sfsp->f_ffree = sblock.fs_cstotal.cs_nifree;
	sfsp->f_fsid.val[0] = 0;
	sfsp->f_fsid.val[1] = 0;
	if ((mntpt = getmntpt(file)) == 0)
		mntpt = "";
	memmove(&sfsp->f_mntonname[0], mntpt, MNAMELEN);
	memmove(&sfsp->f_mntfromname[0], file, MNAMELEN);
	prtstat(sfsp, maxwidth);
	(void)close(rfd);
	return (0);
}

int
bread(off, buf, cnt)
	off_t off;
	void *buf;
	int cnt;
{
	int nr;

	(void)lseek(rfd, off, SEEK_SET);
	if ((nr = read(rfd, buf, cnt)) != cnt) {
		/* Probably a dismounted disk if errno == EIO. */
		if (errno != EIO)
			(void)fprintf(stderr, "\ndf: %I64d: %s\n",
			    off, strerror(nr > 0 ? EIO : errno));
		return (0);
	}
	return (1);
}
#endif /* NO_LEGACY_HERE */

/*
 * lifted off from freebsd math lib
 */
static const unsigned short msign=0x7fff, mexp =0x7ff0  ;
static const short prep1=54, gap=4, bias=1023           ;
static const double novf=1.7E308, nunf=3.0E-308,zero=0.0;
#define national

int
ilogb(x)
	double x;
{
#ifdef national
        short *px=(short *) &x+3, k;
#else	/* national */
        short *px=(short *) &x, k;
#endif	/* national */

        if( (k= *px & mexp ) != mexp )
            if ( k != 0 )
                return ( (k>>gap) - bias );
            else if( x != zero)
                return ((int) -1022.0 );
            else
                return((int)-(1.0/zero));
        else if(x != x)
            return((int)x);
        else
            {*px &= msign; return((int)x);}
}


/*
Local Variables:
c-file-style: "linux"
c-basic-offset: 4
tab-width: 4
End:
*/
