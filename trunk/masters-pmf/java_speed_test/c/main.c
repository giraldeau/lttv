#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	int result;
	FILE *fp;
	int print = 0;

	if(argc >= 2 && !strcmp(argv[1], "-p"))
		print = 1;

	fp = fopen("../trace.dat", "r");
	if(fp == NULL) {
		perror("fopen");
		return 1;
	}

	while(1) {
		unsigned long timestamp;
		unsigned short id;
		unsigned char arglen;
		char *args;

		fscanf(fp, "%4c", &timestamp);
		if(feof(fp))
			break;

		fscanf(fp, "%2c", &id);

		fscanf(fp, "%1c", &arglen);

		args = malloc(arglen);

		// manually specify length of args
		fscanf(fp, "%15c", args);

		unsigned short arg1;
		char *arg2;

		arg1 = *(unsigned short *)args;
		arg2 = args+2;

		if(print)
			printf("timestamp %lu id %hu args=(arg1=%hu arg2=\"%s\")\n", timestamp, id, arg1, arg2);

		free(args);

	}
	fclose(fp);

	return 0;
}
