# popen2
A small module to run an external program using exec () and get its output to arbitrary file descriptors.

Usage:

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
	const char *sep = "";

	if (popen2_init (&po) != 0) {
		perror ("popen2_init");
		return 1;
	}
  
	if (popen2_add_stream (&po, s, 3) != 0) {
		fputs ("Add stream 's1' failed.", stderr);
		return 1;
	}

	if (popen2_exec (&po, cmd, args) != 0) {
		err = errno;
		fprintf (stderr, "popen2_exec() failed. %s:%s\n", po.what, strerror (err));
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
