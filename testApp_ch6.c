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

int test_readback(int fd, const char* str, const char* tag) {
	char str2[128];

	int len, wlen = strlen(str);
	fprintf(stdout, "%s-1. write(%d chrs):%s..", tag, (int)wlen, str);
	if ((len = write(fd, str, wlen)) == -1) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(1);
	} else { fprintf(stdout, "OK.\n"); }

	fprintf(stdout, "%s-2. read()..", tag);
	if ((len = read(fd, str2, 128)) == -1) {
		fprintf(stdout," ERR(%d):%s\n", len, strerror(errno));
		return(1);
	} else {
		fprintf(stdout, "OK.(%d chrs):%s\n", (int)len, str2);
	}

	fprintf(stdout, "%s-3. compare()..", tag);
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
		if((fd = open(devname, O_RDWR | O_APPEND)) == -1) {
			fprintf(stderr,"#1-RDBACK. ERR:on open(%s):%s\n",devname, strerror(errno));
		}

		strcpy(str, MYSTR);
		test_readback(fd, str, "RDBACK-short");
		strcpy(str, MYSTR2);
		test_readback(fd, str, "RDBACK-long");

		close(fd);
		++i;
	}
}
