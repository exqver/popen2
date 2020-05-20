#ifndef POPEN2_H
#define POPEN2_H

typedef struct {
	int	fd;
	char   *data;
	int	max_size;
	int	alloced;
	int	size;
	int	pipes[2];
} POpenStream;

typedef struct {
	int	      poll_fd;
	POpenStream **str;
	int	      nstr;
	const char   *what;
} POpen;

int popen2_init (POpen *);
int popen2_destroy (POpen *);
int popen2_add_stream (POpen *, POpenStream *, int nstream);
int popen2_exec (POpen *, const char *cmd, char *const argv[]);

#endif
