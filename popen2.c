#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "popen2.h"

#ifdef MAX
#undef MAX
#endif
#define MAX(x,y) ((x) >= (y)? (x): (y))

int popen2_init (POpen *p)
{
	p->poll_fd = -1;
	p->str  = 0;
	p->nstr = 0;
	p->what = "";

	return 0;
}

int popen2_add_stream (POpen *p, POpenStream *s, int nstream)
{
	int i;

	p->str = (POpenStream**) realloc (p->str, sizeof *p->str * (p->nstr + nstream));\

	for (i = 0; i < nstream; i++) {
		p->str[p->nstr++] = s + i;
		s[i].pipes[0] = s[i].pipes[1] = -1;
	}

	return 0;
}

int popen2_destroy (POpen *p)
{
	POpenStream **s = p->str, **e = p->str + p->nstr;

	for (; s < e; s++) {
		if ((*s)->pipes[0] >= 0)
			close ((*s)->pipes[0]);
		if ((*s)->pipes[1] >= 0)
		    close ((*s)->pipes[1]);
		if ((*s)->alloced)
			free ((*s)->data);
	}

	free (p->str);
	p->str  = 0;
	p->nstr = 0;

	if (p->poll_fd >= 0) {
		close (p->poll_fd);
		p->poll_fd = -1;
	}

	return 0;
}

int popen2_exec (POpen *p, const char *path, char *const argv[])
{
	POpenStream **s = p->str, **e = p->str + p->nstr, *ss;
	int i, maxfd = -1, maxpipe = -1, n, l;
	struct epoll_event event_data, fdevents[10];
	long flags;
	char buf[1024];
	
	p->poll_fd = epoll_create1 (EPOLL_CLOEXEC);
	if (p->poll_fd == -1) {
		p->what = "epoll_create1";
		return -1;
	}

	for (i = 0; i < p->nstr; i++) {
		s = p->str + i;
		event_data.events   = EPOLLIN | EPOLLHUP;
		event_data.data.ptr = *s;
		if (pipe ((*s)->pipes) != 0) {
			p->what = "pipe";
			return -1;
		}
		if (epoll_ctl (p->poll_fd, EPOLL_CTL_ADD, (*s)->pipes[0], &event_data) != 0) {
			p->what = "epoll_ctl";
			return -1;
		}
	}

	pid_t pid = fork ();

	if (!pid) {				       // child
		for (s = p->str; s < e; s++)
			close ((*s)->pipes[0]);
		
		for (s = p->str; s < e; s++) {
			if (maxfd < 0 || maxfd < (*s)->fd)
				maxfd = (*s)->fd;
			if (maxpipe < 0 || maxpipe < (*s)->pipes[1])
				maxpipe = (*s)->pipes[1];
		}

		if (maxpipe < maxfd)
			maxpipe = maxfd;

		for (s = p->str; s < e; s++) {
			if ((*s)->pipes[1] <= maxfd) {
				dup2 ((*s)->pipes[1], ++maxpipe);
				close ((*s)->pipes[1]);
				(*s)->pipes[1] = maxpipe;
			}
		}
	
		for (s = p->str; s < e; s++) {
			if ((*s)->fd != (*s)->pipes[1])
				dup2 ((*s)->pipes[1], (*s)->fd);
		}

		execv (path, argv);
		perror ("exec");
		exit (1);
	}

	if (pid == -1) {
		p->what = "fork";
		return -1;
	}

	for (s = p->str; s < e; s++) {
		close ((*s)->pipes[1]);
		(*s)->pipes[1] = -1;
		flags = fcntl ((*s)->pipes[0], F_GETFL);
		if (flags == -1) {
			p->what = "fcntl(G_GETFL)";
			return -1;
		}
		if (fcntl ((*s)->pipes[0], F_SETFL, flags | O_NONBLOCK) == -1) {
			p->what = "fcntl(F_SETF,O_NONBLOCK)";
			return -1;
		}
	}

	while ((n = epoll_wait (p->poll_fd, fdevents, sizeof fdevents / sizeof fdevents[0], -1)) > 0) {
		for (i = 0; i < n; i++) {
			ss = (POpenStream*)fdevents[i].data.ptr;
			while ((l = read (ss->pipes[0], buf, sizeof buf)) > 0) {
				if (ss->size + l > ss->alloced) {
					ss->alloced = MAX (ss->alloced * 2, ss->size + l);
					ss->data = (char*)realloc (ss->data, ss->alloced);
					memmove (ss->data + ss->size, buf, l);
					ss->size += l;
				}
			}
			if (l == 0) {
				close (ss->pipes[0]);
				ss->pipes[0] = -1;
			} else if (l == -1) {
				if (errno != EAGAIN) {
					p->what = "read";
					return -1;
				} else continue;
			}
		}
		n = 0;
		for (s = p->str; s < e; s++)
			if ((*s)->pipes[0] != -1) {
				n = 1;
				break;
			}
		if (!n)
			break;
	}

	if (n < 0) {
		p->what = "epoll_wait";
		return -1;
	}

	close (p->poll_fd);
	p->poll_fd = -1;

	waitpid (pid, 0, 0);

	return 0;
}
