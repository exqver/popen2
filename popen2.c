#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <alloca.h>
#include "popen2.h"

#ifdef MAX
#undef MAX
#endif
#define MAX(x,y) ((x) >= (y)? (x): (y))

extern char **environ;

int popen2_init (popen2_t *p)
{
	p->search_path = 0;
	p->poll_fd = -1;
	p->str  = 0;
	p->nstr = 0;
	p->what = "";

	return 0;
}

void popen2_set_search_path (popen2_t *p, int f)
{
	p->search_path = f;
}

int popen2_add_stream (popen2_t *p, popen2_stream_t *s, int nstream)
{
	int i;

	p->str = (popen2_stream_t**) realloc (p->str, sizeof *p->str * (p->nstr + nstream));\

	for (i = 0; i < nstream; i++) {
		p->str[p->nstr++] = s + i;
		s[i].pipes[0] = s[i].pipes[1] = -1;
	}

	return 0;
}

int popen2_destroy (popen2_t *p)
{
	int i;
	popen2_stream_t *s;

	for (i = 0; i < p->nstr; i++) {
		s = p->str[i];

		if (s->pipes[0] >= 0)
			close (s->pipes[0]);

		if (s->pipes[1] >= 0)
		    close (s->pipes[1]);

		if (s->alloced && s->data) {
			free (s->data);
			s->data = 0;
			s->alloced = s->size = 0;
		}
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

int popen2_execve (popen2_t *po, const char *path, char *const argv[], char *const envp[])
{
	popen2_stream_t **s = po->str, **e = po->str + po->nstr, *ss;
	int i, maxfd = -1, maxpipe = -1, n, l;
	struct epoll_event event_data, fdevents[10];
	long flags;
	char buf[1024], *path_env = 0, *p, *next, *f = 0;
	struct stat st;
	
	po->poll_fd = epoll_create1 (EPOLL_CLOEXEC);
	if (po->poll_fd == -1) {
		po->what = "epoll_create1";
		return -1;
	}

	for (i = 0; i < po->nstr; i++) {
		s = po->str + i;
		event_data.events   = EPOLLIN | EPOLLHUP;
		event_data.data.ptr = *s;
		if (pipe ((*s)->pipes) != 0) {
			po->what = "pipe";
			return -1;
		}
		if (epoll_ctl (po->poll_fd, EPOLL_CTL_ADD, (*s)->pipes[0], &event_data) != 0) {
			po->what = "epoll_ctl";
			return -1;
		}
	}

	pid_t pid = fork ();

	if (!pid) {				       // child
		for (s = po->str; s < e; s++)
			close ((*s)->pipes[0]);
		
		for (s = po->str; s < e; s++) {
			if (maxfd < 0 || maxfd < (*s)->fd)
				maxfd = (*s)->fd;
			if (maxpipe < 0 || maxpipe < (*s)->pipes[1])
				maxpipe = (*s)->pipes[1];
		}

		if (maxpipe < maxfd)
			maxpipe = maxfd;

		for (s = po->str; s < e; s++) {
			if ((*s)->pipes[1] <= maxfd) {
				dup2 ((*s)->pipes[1], ++maxpipe);
				close ((*s)->pipes[1]);
				(*s)->pipes[1] = maxpipe;
			}
		}
	
		for (s = po->str; s < e; s++) {
			if ((*s)->fd != (*s)->pipes[1])
				dup2 ((*s)->pipes[1], (*s)->fd);
		}

		if (po->search_path && path[0] != '/') {
#if _GNU_SOURCE
			execvpe (path, argv, envp);
#else
			// if (envp) {
			// 	path_env = 0;
			// 	for (i = 0; envp[i]; i++) {
			// 		if (strncmp (envp[i], "PATH=", 5) == 0) {
			// 			path_env = envp[i] + 5;
			// 			break;
			// 		}
			// 	}
			// }

			if (!path_env)
				path_env = getenv ("PATH");

			if (path_env) {
				path_env = strdup (path_env);

				for (p = path_env; *p; p = next)
				{
					next = p + strcspn (p, ":");
					if (*next)
						*next++ = 0;
					if (!*p)
						p = ".";
					f = (char*)realloc (f, strlen (p) + 1 + strlen (path) + 1);
					strcpy (f, p);
					strcat (f, "/");
					strcat (f, path);
					if (stat (f, &st) == 0 && S_ISREG (st.st_mode) && access (f, X_OK) == 0)
						break;
				}

				if (p && *p)
					execve (f, argv, envp);

				free (path_env);
				free (f);
			} else execve (path, argv, envp);
#endif
		} else execve (path, argv, envp);

		perror ("exec");
		exit (1);
	}

	if (pid == -1) {
		po->what = "fork";
		return -1;
	}

	for (s = po->str; s < e; s++) {
		close ((*s)->pipes[1]);
		(*s)->pipes[1] = -1;
		flags = fcntl ((*s)->pipes[0], F_GETFL);
		if (flags == -1) {
			po->what = "fcntl(G_GETFL)";
			return -1;
		}
		if (fcntl ((*s)->pipes[0], F_SETFL, flags | O_NONBLOCK) == -1) {
			po->what = "fcntl(F_SETF,O_NONBLOCK)";
			return -1;
		}
	}

	while ((n = epoll_wait (po->poll_fd, fdevents, sizeof fdevents / sizeof fdevents[0], -1)) > 0) {
		for (i = 0; i < n; i++) {
			ss = (popen2_stream_t*)fdevents[i].data.ptr;
			while ((l = read (ss->pipes[0], buf, sizeof buf)) > 0) {
				if (ss->size + l > ss->alloced) {
					ss->alloced = MAX (ss->alloced * 2, ss->size + l);
					ss->data = (char*)realloc (ss->data, ss->alloced);
				}
				memmove (ss->data + ss->size, buf, l);
				ss->size += l;
			}
			if (l == 0) {
				close (ss->pipes[0]);
				ss->pipes[0] = -1;
			} else if (l == -1) {
				if (errno != EAGAIN) {
					po->what = "read";
					return -1;
				} else continue;
			}
		}
		n = 0;
		for (s = po->str; s < e; s++)
			if ((*s)->pipes[0] != -1) {
				n = 1;
				break;
			}
		if (!n)
			break;
	}

	if (n < 0) {
		po->what = "epoll_wait";
		return -1;
	}

	close (po->poll_fd);
	po->poll_fd = -1;

	waitpid (pid, 0, 0);

	return 0;
}

int  popen2_execv (popen2_t *po, const char *cmd, char *const argv[])
{
	return popen2_execve (po, cmd, argv, environ);
}

int popen2_execl (popen2_t *po, const char *cmd, ...)
{
	va_list ap;
	char **args = 0;
	int    nargs = 1, i;
	const char *arg;

	va_start (ap, cmd);
	while ((arg = va_arg (ap, const char*)) != 0)
		nargs++;
	va_end (ap);

	args = (char **) alloca (nargs * sizeof args[0]);

	va_start (ap, cmd);
	i = 0;
	while ((args[i++] = va_arg (ap, char *)) != 0);
	va_end (ap);

	return popen2_execv (po, cmd, args);
}

int  popen2_execle (popen2_t *po, const char *cmd, ...)
{
	va_list ap;
	char **args = 0;
	int    nargs = 1, i;
	const char *arg;
	char **envp;

	va_start (ap, cmd);
	while ((arg = va_arg (ap, const char*)) != 0)
		nargs++;
	va_end (ap);

	args = (char **) alloca (nargs * sizeof args[0]);

	va_start (ap, cmd);
	i = 0;
	while ((args[i++] = va_arg (ap, char *)) != 0);
	envp = va_arg (ap, char **);
	va_end (ap);

	return popen2_execve (po, cmd, args, envp);
}
