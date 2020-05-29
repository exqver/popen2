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
	int	      search_path;		       /* Option: set to 1 for similar to execlp(), execvp(), execvpe(). */
	int	      poll_fd;
	popen2_stream_t **str;
	int	      nstr;
	const char   *what;
} popen2_t;

int  popen2_init (popen2_t *);
int  popen2_destroy (popen2_t *);
void popen2_set_search_path (popen2_t *, int);
int  popen2_add_stream (popen2_t *, popen2_stream_t *, int nstream);
int  popen2_execve (popen2_t *, const char *cmd, char *const argv[], char *const envp[]);
int  popen2_execv (popen2_t *, const char *cmd, char *const argv[]);
int  popen2_execl (popen2_t *, const char *cmd, ...);   /*, (char*) NULL */
int  popen2_execle (popen2_t *, const char *cmd, ...);   /*, (char*) NULL, char * const envp[] */


#endif
