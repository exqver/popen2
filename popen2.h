#ifndef POPEN2_H
#define POPEN2_H

typedef struct {
	int	fd;				       /* Descriptor to capture */
	char   *data;
	int	max_size;			       /* Received data, stored to malloced buffer. */
						       /* May be preallocated before popen2_add_stream (). */
	int	alloced;			       /* Allocated memory size. */
						       /* May be preallocated before popen2_add_stream (). */
	int	size;
	int	pipes[2];
} popen2_stream_t;

typedef struct {
	int	      search_path;		       /* Option: Run similar to execlp(), execvp(), execvpe(). */
	int	      exclusive;		       /* Option: Close all files descriptors but listened ones. Set by default. */
	int	      poll_fd;
	popen2_stream_t **str;			       /* Streams, added with popen2_add_stream (). */
	int	      nstr;			       /* Number of streams */
	const char   *what;			       /* Reason of failure */
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
