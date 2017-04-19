#include <sys/types.h>
#include <libbb.h>
#include <stdio.h>
#include <signal.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <malloc.h>

int kill(pid_t pid, int sig)
{
#if 0
        HANDLE h;

        if (pid > 0 && sig == SIGTERM) {
                if ((h=OpenProcess(PROCESS_TERMINATE, FALSE, pid)) != NULL &&
                                TerminateProcess(h, 0)) {
                        CloseHandle(h);
                        return 0;
                }

                errno = mapwinerr(GetLastError());
                if (h != NULL)
                        CloseHandle(h);
                return -1;
        }

        errno = EINVAL;
#endif
        return -1;
}


int
sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
	printf("sigprocmask: ignored\n");
	return 0;
}

int
killpg(int pgrp, int sig)
{
	printf("killpg %d sig %d ignored\n", pgrp, sig);

}

int sigaction(int signum, const struct sigaction *act,
                     struct sigaction *oldact)
{
	if (!act) {
		oldact->sa_handler = SIG_DFL;
		oldact->sa_mask = 0;
	}
}

int setpgid(pid_t pid, pid_t pgid)
{
	printf("setpgid ignored\n");

}

int getpgrp(void)
{
	return -1;
}


pid_t tcgetpgrp(int fd)
{
	return -1;

}

pid_t tcsetpgrp(int fd, pid_t pgrp)
{

}

int getrlimit(int resource, struct rlimit *rlim)
{
	rlim->rlim_cur = 0;
	rlim->rlim_max = -1;
}

int setrlimit(int resource, struct rlimit *rlim)
{
	return 0;
}

void
unsetenv(const char *name)
{
	char *env;
	int ret;

	env = xmalloc(strlen(name)+4);
	sprintf(env, "%s=", name);
	ret = putenv(env);
	free(env);
	return (ret);
}


int
stime(time_t *t)
{
	printf("stime: unsupported\n");
	return -1;
}

int
mkfifo(const char *path, int mode)
{
	return -1;
}

unsigned int alarm(unsigned int seconds)
{

}
