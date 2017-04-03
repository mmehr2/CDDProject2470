/*  CDD2app.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define CMD1 1
#define CMD2 2
#define MYNUM 0x88888888
#define MYSTR "Eureka!"
#define MYSTR2 "Hello World 13049872138472130984721398749832174."

int seek_file(int fd, const char* tag, int offs, int whence) {
	int len;
	fprintf(stdout, "%s-X. seek(%d,%d)..", tag, offs, whence);
	if ((len = lseek(fd, offs, whence)) < 0) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(-1);
	} else {
		fprintf(stdout, "OK.\n");
	}
	return len;
}

int test_readback(int fd, const char* str, const char* tag, int offset, int whence) {
	char str2[128];

	int newofs, len, wlen = strlen(str)+1; // include the null

	if ((newofs = seek_file(fd, tag, offset, whence)) < 0)
		return 1;

	fprintf(stdout, "%s-1. write(%d chrs):%s..", tag, (int)wlen, str);
	if ((len = write(fd, str, wlen)) < 0) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(1);
	} else { fprintf(stdout, "OK.\n"); }

	// reset file pos to where we wrote the string
	if (seek_file(fd, tag, newofs, SEEK_SET) < 0)
		return 1;

	fprintf(stdout, "%s-3. read()..", tag);
	if ((len = read(fd, str2, 128)) < 0) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(1);
	} else {
		str2[len] = 0; // terminate the received string
		fprintf(stdout, "OK.(%d chrs):%s\n", (int)len, str2);
	}

	fprintf(stdout, "%s-4. compare()..", tag);
	if ((len = strcmp(str, str2)) != 0) {
		fprintf(stdout," ERR(%d):%s\n", len, "UNEQUAL");
	} else { fprintf(stdout, "OK.\n"); }

	return 0;
}

static char* devnames[] = {
	"CDD2", "CDD16", "CDD64", "CDD128", "CDD256",
};
int NUMDEVS = sizeof(devnames)/sizeof(devnames[0]);

const char* get_devname(const char* prefix, int index) {
	static char str[128];
	int i= index<0? 0: index>NUMDEVS? NUMDEVS: index;
	strcpy(str, prefix);
	strcat(str, devnames[i]);
	return str;
}

int main(int argc, char**argv) {
	int fd, len, wlen, i=0, test=-1;
	char str[128];
	const char* devname, * tag;

	if (argc > 1) {
		test = atoi(argv[1]);
		fprintf(stdout, "Executing test command %d\n", test);
		switch (test) {
		case 0:
			tag = "RDBACK";
			break;
		case 1:
			tag = "SEEK";
			break;
		default:
			// just open/close testing
			tag = "OPEN";
			break;
		}
} else {
		fprintf(stdout,
			"Usage: %s [testnum]\n"
			"where testnum is as follows:\n"
			"\t0\tRun readback test (write, seek, read, compare) on all CDD drivers.\n"
			"\t1\tRun advanced seek test on all CDD drivers.\n"
			"\tElse\tRun open/close test on all CDD drivers.\n"
			, argv[0]);
		exit(0);
	}
	i = 0;
	while (i<NUMDEVS) {
		devname = get_devname("/dev/", i);
		fprintf(stdout, "#1-%s. open(%s)..", tag, devname);
		if((fd = open(devname, O_RDWR | O_TRUNC)) == -1) {
			fprintf(stdout,"ERR:%s\n", strerror(errno));
		} else { fprintf(stdout, "OK.\n"); }

		switch (test) {
		case 0:
			strcpy(str, MYSTR2);
			test_readback(fd, str, "RDBACK-long", 0, SEEK_SET);
			strcpy(str, MYSTR);
			test_readback(fd, str, "RDBACK-short", 0, SEEK_SET);
			break;
		case 1:
			strcpy(str, MYSTR);
			test_readback(fd, str, "SEEK-start-short", 0, SEEK_SET);
			test_readback(fd, str, "SEEK-(END-50)-short", -50, SEEK_END);
			test_readback(fd, str, "SEEK-(END-5)-short", -5, SEEK_END);
			test_readback(fd, str, "SEEK-(END+5)-short", 5, SEEK_END);
			test_readback(fd, str, "SEEK-(END+500)-short", 500, SEEK_END);
			test_readback(fd, str, "SEEK-(START-5)-short", -5, SEEK_SET);
			test_readback(fd, str, "SEEK-(START+5)-short", 5, SEEK_SET);
			test_readback(fd, str, "SEEK-(START+50)-short", 50, SEEK_SET);
			test_readback(fd, str, "SEEK-(CUR-5)-short", -5, SEEK_CUR);
			test_readback(fd, str, "SEEK-(CUR+5)-short", 5, SEEK_CUR);
			break;
		default:
		// just open/close testing
			break;
		}

		close(fd);
		++i;
	}
}
