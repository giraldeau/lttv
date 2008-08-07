#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

int main()
{
	int result;
	int fd;

	result = fd = open("../trace.dat", O_RDONLY);
	if(result == -1) {
		perror("open");
		return 1;
	}

	while(1) {
		unsigned long timestamp;
		unsigned short id;
		unsigned char arglen;
		char *args;

		result = read(fd, &timestamp, 4);
		if(result == 0)
			break;
		if(result < 4) {
			perror("read");
			return 1;
		}

		result = read(fd, &id, 2);
		if(result < 2) {
			perror("read");
			return 1;
		}

		result = read(fd, &arglen, 1);
		if(result < 1) {
			perror("read");
			return 1;
		}

		args = malloc(arglen);

		result = read(fd, args, arglen);
		if(result < arglen) {
			perror("read");
			return 1;
		}

		unsigned short arg1;
		char *arg2;

		arg1 = *(unsigned short *)args;
		arg2 = args+2;

		printf("timestamp %u id %hu args=(arg1=%hu arg2=\"%s\")\n", timestamp, id, arg1, arg2);

		free(args);
	}
	close(fd);

	return 0;
}
