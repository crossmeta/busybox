/* vi: set sw=4 ts=4: */

#include "busybox.h"

#include <devioctl.h>
#include <sys/msgbuf.h>
#include <stdlib.h>

int dmesg_main(int argc, char **argv)
{
	register int ch, newl, skip;
	register char *p, *ep;
	struct msgbuf *bufp, cur;
	char *bp;
	char buf[5];
	int	hdev;
	int	error, nbytes;

	hdev = devopen(KLOGDEVSTR, 0);
	if (hdev < 0)
		error_msg_and_die("devopen %s error(%d)", KLOGDEVSTR, geterrno());

	/* Read in kernel message buffer, do sanity checks. */
	error = devioctl(hdev, KLOG_MSGBUF_HEADER, NULL, 0, 
				&cur, sizeof (cur));
	if (error < 0)
		error_msg_and_die("devioctl(KLOG_MSGBUF_HEADER) error(%d)",
			geterrno());
	if (cur.msg_magic != MSG_MAGIC)
		error_msg_and_die("kernel message buffer has different magic number");
	bp = malloc(cur.msg_size);
	if (!bp)
		error_msg_and_die("malloc failed");
	error = devioctl(hdev, KLOG_MSGBUF_DATA, NULL, 0, bp, cur.msg_size);
	if (error < 0)
		error_msg_and_die("devioctl(KLOG_MSGBUF_DATA) error(%d)",
			geterrno());
	if (cur.msg_bufx >= cur.msg_size)
		cur.msg_bufx = 0;
	devclose(hdev);

	/*
	 * The message buffer is circular.  If the buffer has wrapped, the
	 * write pointer points to the oldest data.  Otherwise, the write
	 * pointer points to \0's following the data.  Read the entire
	 * buffer starting at the write pointer and ignore nulls so that
	 * we effectively start at the oldest data.
	 */
	p = bp + cur.msg_bufx;
	ep = (cur.msg_bufx == 0 ? bp + cur.msg_size : p);
	newl = skip = 0;
	do {
		if (p == bp + cur.msg_size)
			p = bp;
		ch = *p;
		/* Skip "\n<.*>" syslog sequences. */
		if (skip) {
			if (ch == '>')
				newl = skip = 0;
			continue;
		}
		if (newl && ch == '<') {
			skip = 1;
			continue;
		}
		if (ch == '\0')
			continue;
		newl = ch == '\n';
		(void)vis(buf, ch, 0, 0);
		if (buf[1] == 0)
			(void)putchar(buf[0]);
		else
			(void)printf("%s", buf);
	} while (++p != ep);
	if (!newl)
		(void)putchar('\n');
	return EXIT_SUCCESS;
}

int
vis(char *buf, int ch, int x, int y)
{
	buf[0] = (char)ch;
	buf[1] = (char)0;
	return (0);

}
