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

int reset_file(int fd, const char* tag) {
	int len;
	fprintf(stdout, "%s-X. seek(0,SET)..", tag);
	if ((len = lseek(fd, 0, SEEK_SET)) < 0) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(1);
	} else {
		fprintf(stdout, "OK.\n");
	}
}

int test_readback(int fd, const char* str, const char* tag) {
	char str2[128];

	int len, wlen = strlen(str)+1; // include the null
	fprintf(stdout, "%s-1. write(%d chrs):%s..", tag, (int)wlen, str);
	if ((len = write(fd, str, wlen)) < 0) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(1);
	} else { fprintf(stdout, "OK.\n"); }

	reset_file(fd, tag);

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

int main() {
	int fd, len, wlen, i=0;
	char str[128];
	const char* devname;

	i = 0;
	while (i<NUMDEVS) {
		devname = get_devname("/dev/", i);
		fprintf(stdout, "#1-RDBACK. open(%s)..", devname);
		if((fd = open(devname, O_RDWR | O_TRUNC)) == -1) {
			fprintf(stdout,"ERR:%s\n", strerror(errno));
		} else { fprintf(stdout, "OK.\n"); }

		strcpy(str, MYSTR2);
		test_readback(fd, str, "RDBACK-long");
		reset_file(fd, "RDBACK");
		strcpy(str, MYSTR);
		test_readback(fd, str, "RDBACK-short");

		close(fd);
		++i;
	}
}
