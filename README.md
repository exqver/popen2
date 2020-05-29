# popen2
A small module to run an external program using exec () and get its output to arbitrary file descriptors.

All 'exec' flavors are supported (execve, execv, execl, execle, execvpe, execlpe).

 * execve --> popen2_execve
 * execv --> popen2_execv
 * execvpe--> { popen_t po; po.search_path = 1; popen2_execve (&po, args, envp); }
 * execl --> popen2_execl
 * execle --> popen2_execle
 * execlpe\* --> { popen_t po; po.search_path = 1; popen2_execve (&po, args, envp); }

 \* -- not in POSIX.

Types:

```
typedef struct {
	int	      search_path;		       /* Option: Run similar to execlp(), execvp(), execvpe(). */
	int	      exclusive;		       /* Option: Close all files descriptors but listened ones. Set by default. */
	popen2_stream_t **str;			       /* Streams, added with popen2_add_stream (). */
	int	      nstr;			       /* Number of streams */
	const char   *what;			       /* Reason of failure */
	...
} popen2_t;

typedef struct {
	int	fd;				/* Descriptor to capture */
	char   *data;				/* Received data, stored to malloced buffer. */
						/* May be preallocated before popen2_add_stream (). */
	int	max_size;			/* Not used */
	int	alloced;			/* Allocated memory size */
						/* May be preallocated before popen2_add_stream (). */
	int	size;				/* Length of stored data in 'data' */
	...
} popen2_stream_t;

```

Usage:

```
	#include <popen2.h>
	popen2_stream_t s[] = {
		{ 1 },				       // stdout
		{ 2 },				       // stderr
		{ 5 }				       // fd 5
	};
	popen2_t po;
	int err;
	unsigned i;
	const char *cmd = "/bin/bash";
	char *const args[] = { "bash", "-c", "echo file descriptor 1: 111; "
			       "echo file descriptor 2: 222 1>&2; "
			       "echo file descriptor 5: 555 1>&5; "
			       "no-such-command", 0 };

	if (popen2_init (&po) != 0) {
		perror ("popen2_init");
		return 1;
	}
  
	if (popen2_add_stream (&po, s, 3) != 0) {
		fputs ("Add stream 's1' failed.", stderr);
		return 1;
	}

	if (popen2_execv (&po, cmd, args) != 0) {
		err = errno;
		fprintf (stderr, "popen2_execv() failed. %s:%s\n", po.what, strerror (err));
		return 1;
	}

	// Some processing of collected data ...
	for (i = 0; i < sizeof s / sizeof s[0]; i++)
		printf ("fd %d: %d bytes [%.*s]\n",
			s[i].fd,
			s[i].size,
			s[i].size,
			s[i].data);
	// ...

	// Release resources.
	popen2_destroy (&po);

```