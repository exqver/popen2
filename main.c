#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "popen2.h"

void dump_stream_data (popen2_stream_t *s, int cnt)
{
	int i;

	for (i = 0; i < cnt; i++)
		printf ("fd %d: %d bytes [%.*s]\n",
			s[i].fd,
			s[i].size,
			s[i].size,
			s[i].data);
}

void dump_strings (char * const *ss, int num)
{
	int i;
	const char *sep = "";

	putchar ('[');
	for (i = 0; (num >= 0 && i < num) || ss[i]; i++, sep = ",")
		printf ("%s\"%s\"", sep, ss[i]);
	putchar (']');
}

int main ()
{
	popen2_stream_t s[] = {
		{ 1 },				       // stdout
		{ 2 },				       // stderr
		{ 5 }				       // fd 5
	};
	popen2_t po;
	int err;
	unsigned i;
	// const char *cmd = "/bin/bash";
	const char *cmd = "bash";
	char *const args[] = { "bash", "-c", "echo 111; "
			       "echo 222 1>&2; "
			       "echo 555 1>&5; "
			       "no-such-command; "
			       "echo Environment variables:; set; "
			       "echo Opened files:; ls -l /proc/$$/fd;",
			       0 };
	char *const env[] = {
		"PATH=/bin1:/bin2::::/bin:aaa",
		"AAA=aaa",
		0
	};
	int search_path = 1;

	printf ("Search $PATH fom command: %s\n", search_path? "yes": "no");
	
	for (i = 0; i < 4; i++) {
		if (popen2_init (&po) != 0) {
			perror ("popen2_init");
			return 1;
		}

		if (popen2_add_stream (&po, s, sizeof s / sizeof s[0]) != 0) {
			fputs ("Failed to add stream.", stderr);
			return 1;
		}

		// Search executable in $PATH like shell do.
		po.search_path = search_path;

		switch (i) {
		case 0:
			printf ("---- Testing popen2_execve (): ----\n");
			printf ("Command path: \"%s\"\n", cmd);
			printf ("Args: ");
			dump_strings (args, -1);
			printf ("\nPassed environment: ");
			dump_strings (env, -1);
			printf ("\n");
			if (popen2_execve (&po, cmd, args, env) != 0) {
				err = errno;
				fprintf (stderr, "popen2_execve() failed. %s:%s\n", po.what, strerror (err));
				return 1;
			}
			break;
		case 1:
			printf ("---- Testing popen2_execv (): ----\n");
			printf ("Command path: \"%s\"\n", cmd);
			printf ("Args: ");
			dump_strings (args, -1);
			printf ("\n");
			if (popen2_execv (&po, cmd, args) != 0) {
				err = errno;
				fprintf (stderr, "popen2_execv() failed. %s:%s\n", po.what, strerror (err));
				return 1;
			}
			break;
		case 2:
			printf ("---- Testing popen2_execl (): ----\n");
			printf ("Command path: \"%s\"\n", cmd);
			printf ("Args: [\"bash\", \"-c\", \"ls -lF\"]\n");
			if (popen2_execl (&po, cmd, "bash", "-c", "ls -lF", NULL) != 0) {
				err = errno;
				fprintf (stderr, "popen2_execl() failed. %s:%s\n", po.what, strerror (err));
				return 1;
			}
			break;
		case 3:
			printf ("---- Testing popen2_execle (): ----\n");
			cmd = "/bin/ls";
			printf ("Command path: \"%s\"\n", cmd);
			printf ("Args: [\"ls\", \"-lF\", \"/proc/self/fd\"]\n");
			printf ("Passed environment: ");
			dump_strings (env, -1);
			printf ("\n");
			if (popen2_execle (&po, cmd, "ls", "-lF", "/proc/self/fd", "set", NULL, env) != 0) {
				err = errno;
				fprintf (stderr, "popen2_execle() failed. %s:%s\n", po.what, strerror (err));
				return 1;
			}
			break;
		}
		dump_stream_data (s, sizeof s / sizeof s[0]);
		printf ("---- done ----\n");
		popen2_destroy (&po);
	}

	return 0;
}
