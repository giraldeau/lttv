#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>

#include "blktrace_api.h"

int start(int fd)
{
	struct blk_user_trace_setup buts;

	memset(&buts, 0, sizeof(buts));
//	buts.buf_size = 512*1024;
//	buts.buf_nr = 4;
//	buts.act_mask = ~0;

	if (ioctl(fd, BLKTRACESETUP, &buts) < 0) {
		perror("BLKTRACESETUP");
		return 1;
	}

	return 0;
}

int stop(int fd)
{
	ioctl(fd, BLKTRACESTOP);
	if(ioctl(fd, BLKTRACETEARDOWN) < 0)
		perror("BLKTRACETEARDOWN");

	return 0;
}

int main(int argc, char **argv)
{
	int fd;

	if(argc < 3) {
		fprintf(stderr, "usage: --start|--stop %s BLKDEV\n", argv[0]);
		return 1;
	}

	fd = open(argv[2], O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror(argv[0]);
		return 1;
	}

	if(!strcmp("--start", argv[1]))
		start(fd);
	else if(!strcmp("--stop", argv[1]))
		stop(fd);

	close(fd);

	return 0;
}
