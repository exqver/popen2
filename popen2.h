#ifndef POPEN2_H
#define POPEN2_H

typedef struct {
	int	fd;
	char   *data;
	int	max_size;
	int	alloced;
	int	size;
	int	pipes[2];
} popen2_stream_t;

typedef struct {
	int	      poll_fd;
	popen2_stream_t **str;
	int	      nstr;
	const char   *what;
} popen2_t;

int popen2_init (popen2_t *);
int popen2_destroy (popen2_t *);
int popen2_add_stream (popen2_t *, popen2_stream_t *, int nstream);
int popen2_exec (popen2_t *, const char *cmd, char *const argv[]);

#endif
