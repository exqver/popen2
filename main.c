#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "popen2.h"

int main ()
{
	// POpenStream s1 = { 1 } , s2 = { 2 }, s5 = { 5 };
	POpenStream s[] = {
		{ 1 },
		{ 2 },
		{ 5 }
	};
	POpen po;
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

	// if (popen2_add_stream (&po, &s1, 1) != 0) {
	// 	fputs ("Add stream 's1' failed.", stderr);
	// 	return 1;
	// }

	// if (popen2_add_stream (&po, &s2, 1) != 0) {
	// 	fputs ("Add stream 's2' failed.", stderr);
	// 	return 1;
	// }

	// if (popen2_add_stream (&po, &s5, 1) != 0) {
	// 	fputs ("Add stream 's5' failed.", stderr);
	// 	return 1;
	// }

	if (popen2_add_stream (&po, s, 3) != 0) {
		fputs ("Add stream 's1' failed.", stderr);
		return 1;
	}

	if (popen2_exec (&po, cmd, args) != 0) {
		err = errno;
		fprintf (stderr, "popen2_exec() failed. %s:%s\n", po.what, strerror (err));
		return 1;
	}

	printf ("Command: %s\nArgs: [", cmd);
	for (i = 0; args[i]; i++, sep = ", ")
		printf ("%s\"%s\"", sep, args[i]);
	printf ("]\n");
	
	for (i = 0; i < sizeof s / sizeof s[0]; i++)
		printf ("fd %d: %d bytes [%.*s]\n",
			s[i].fd,
			s[i].size,
			s[i].size,
			s[i].data);

	// fprintf (stdout, "fd %d: %d bytes [%.*s]\n", s1.fd, s1.size, s1.size, s1.buf);
	// fprintf (stdout, "fd %d: %d bytes [%.*s]\n", s2.fd, s2.size, s2.size, s2.buf);
	// fprintf (stdout, "fd %d: %d bytes [%.*s]\n", s5.fd, s5.size, s5.size, s5.buf);

	popen2_destroy (&po);

	return 0;
}
