/* lttd
 *
 * Linux Trace Toolkit Daemon
 *
 * This is a simple daemon that reads a few relayfs channels and save them in a
 * trace.
 *
 *
 * Copyright 2005 -
 * 	Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <signal.h>

/* Relayfs IOCTL */
#include <asm/ioctl.h>
#include <asm/types.h>

/* Get the next sub buffer that can be read. */
#define RELAYFS_GET_SUBBUF        _IOR(0xF4, 0x00,__u32)
/* Release the oldest reserved (by "get") sub buffer. */
#define RELAYFS_PUT_SUBBUF        _IOW(0xF4, 0x01,__u32)
/* returns the number of sub buffers in the per cpu channel. */
#define RELAYFS_GET_N_SUBBUFS     _IOR(0xF4, 0x02,__u32)
/* returns the size of the sub buffers. */
#define RELAYFS_GET_SUBBUF_SIZE   _IOR(0xF4, 0x03,__u32)



enum {
	GET_SUBBUF,
	PUT_SUBBUF,
	GET_N_BUBBUFS,
	GET_SUBBUF_SIZE
};

struct fd_pair {
	int channel;
	int trace;
	unsigned int n_subbufs;
	unsigned int subbuf_size;
	void *mmap;
};

struct channel_trace_fd {
	struct fd_pair *pair;
	int num_pairs;
};

static char *trace_name = NULL;
static char *channel_name = NULL;
static int	daemon_mode = 0;
static int	append_mode = 0;
volatile static int	quit_program = 0;	/* For signal handler */

/* Args :
 *
 * -t directory		Directory name of the trace to write to. Will be created.
 * -c directory		Root directory of the relayfs trace channels.
 * -d          		Run in background (daemon).
 * -a							Trace append mode.
 * -s							Send SIGUSR1 to parent when ready for IO.
 */
void show_arguments(void)
{
	printf("Please use the following arguments :\n");
	printf("\n");
	printf("-t directory  Directory name of the trace to write to.\n"
				 "              It will be created.\n");
	printf("-c directory  Root directory of the relayfs trace channels.\n");
	printf("-d            Run in background (daemon).\n");
	printf("-a            Append to an possibly existing trace.\n");
	printf("\n");
}


/* parse_arguments
 *
 * Parses the command line arguments.
 *
 * Returns 1 if the arguments were correct, but doesn't ask for program
 * continuation. Returns -1 if the arguments are incorrect, or 0 if OK.
 */
int parse_arguments(int argc, char **argv)
{
	int ret = 0;
	int argn = 1;
	
	if(argc == 2) {
		if(strcmp(argv[1], "-h") == 0) {
			return 1;
		}
	}

	while(argn < argc) {

		switch(argv[argn][0]) {
			case '-':
				switch(argv[argn][1]) {
					case 't':
						if(argn+1 < argc) {
							trace_name = argv[argn+1];
							argn++;
						}
						break;
					case 'c':
						if(argn+1 < argc) {
							channel_name = argv[argn+1];
							argn++;
						}
						break;
					case 'd':
						daemon_mode = 1;
						break;
					case 'a':
						append_mode = 1;
						break;
					default:
						printf("Invalid argument '%s'.\n", argv[argn]);
						printf("\n");
						ret = -1;
				}
				break;
			default:
				printf("Invalid argument '%s'.\n", argv[argn]);
				printf("\n");
				ret = -1;
		}
		argn++;
	}
	
	if(trace_name == NULL) {
		printf("Please specify a trace name.\n");
		printf("\n");
		ret = -1;
	}
	
	if(channel_name == NULL) {
		printf("Please specify a channel name.\n");
		printf("\n");
		ret = -1;
	}
	
	return ret;
}

void show_info(void)
{
	printf("Linux Trace Toolkit Trace Daemon\n");
	printf("\n");
	printf("Reading from relayfs directory : %s\n", channel_name);
	printf("Writing to trace directory : %s\n", trace_name);
	printf("\n");
}


/* signal handling */

static void handler(int signo)
{
	printf("Signal %d received : exiting cleanly\n", signo);
	quit_program = 1;
}



int open_channel_trace_pairs(char *subchannel_name, char *subtrace_name,
		struct channel_trace_fd *fd_pairs)
{
	DIR *channel_dir = opendir(subchannel_name);
	struct dirent *entry;
	struct stat stat_buf;
	int ret;
	char path_channel[PATH_MAX];
	int path_channel_len;
	char *path_channel_ptr;
	char path_trace[PATH_MAX];
	int path_trace_len;
	char *path_trace_ptr;
  int open_ret = 0;

	if(channel_dir == NULL) {
		perror(subchannel_name);
		open_ret = ENOENT;
    goto end;
	}

	printf("Creating trace subdirectory %s\n", subtrace_name);
	ret = mkdir(subtrace_name, S_IRWXU|S_IRWXG|S_IRWXO);
	if(ret == -1) {
		if(errno != EEXIST) {
			perror(subtrace_name);
      open_ret = -1;
			goto end;
		}
	}

	strncpy(path_channel, subchannel_name, PATH_MAX-1);
	path_channel_len = strlen(path_channel);
	path_channel[path_channel_len] = '/';
	path_channel_len++;
	path_channel_ptr = path_channel + path_channel_len;

	strncpy(path_trace, subtrace_name, PATH_MAX-1);
	path_trace_len = strlen(path_trace);
	path_trace[path_trace_len] = '/';
	path_trace_len++;
	path_trace_ptr = path_trace + path_trace_len;
	
	while((entry = readdir(channel_dir)) != NULL) {

		if(entry->d_name[0] == '.') continue;
		
		strncpy(path_channel_ptr, entry->d_name, PATH_MAX - path_channel_len);
		strncpy(path_trace_ptr, entry->d_name, PATH_MAX - path_trace_len);
		
		ret = stat(path_channel, &stat_buf);
		if(ret == -1) {
			perror(path_channel);
			continue;
		}
		
		printf("Channel file : %s\n", path_channel);
		
		if(S_ISDIR(stat_buf.st_mode)) {

			printf("Entering channel subdirectory...\n");
			ret = open_channel_trace_pairs(path_channel, path_trace, fd_pairs);
			if(ret < 0) continue;
		} else if(S_ISREG(stat_buf.st_mode)) {
			printf("Opening file.\n");
			
			fd_pairs->pair = realloc(fd_pairs->pair,
					++fd_pairs->num_pairs * sizeof(struct fd_pair));

			/* Open the channel in read mode */
			fd_pairs->pair[fd_pairs->num_pairs-1].channel = 
				open(path_channel, O_RDONLY | O_NONBLOCK);
			if(fd_pairs->pair[fd_pairs->num_pairs-1].channel == -1) {
				perror(path_channel);
				fd_pairs->num_pairs--;
				continue;
			}
			/* Open the trace in write mode, only append if append_mode */
			ret = stat(path_trace, &stat_buf);
			if(ret == 0) {
				if(append_mode) {
					printf("Appending to file %s as requested\n", path_trace);

					fd_pairs->pair[fd_pairs->num_pairs-1].trace = 
						open(path_trace, O_WRONLY|O_APPEND,
								S_IRWXU|S_IRWXG|S_IRWXO);

					if(fd_pairs->pair[fd_pairs->num_pairs-1].trace == -1) {
						perror(path_trace);
					}
				} else {
					printf("File %s exists, cannot open. Try append mode.\n", path_trace);
					open_ret = -1;
          goto end;
				}
			} else {
				if(errno == ENOENT) {
					fd_pairs->pair[fd_pairs->num_pairs-1].trace = 
						open(path_trace, O_WRONLY|O_CREAT|O_EXCL,
								S_IRWXU|S_IRWXG|S_IRWXO);
					if(fd_pairs->pair[fd_pairs->num_pairs-1].trace == -1) {
						perror(path_trace);
					}
				}
			}
		}
	}
	
end:
	closedir(channel_dir);

	return open_ret;
}


int read_subbuffer(struct fd_pair *pair)
{
	unsigned int	consumed_old;
	int err, ret;


	err = ioctl(pair->channel, RELAYFS_GET_SUBBUF, 
								&consumed_old);
	printf("cookie : %u\n", consumed_old);
	if(err != 0) {
		perror("Error in reserving sub buffer");
		ret = -EPERM;
		goto get_error;
	}
	
	err = TEMP_FAILURE_RETRY(write(pair->trace,
				pair->mmap + (consumed_old & (~(pair->subbuf_size-1))),
				pair->subbuf_size));

	if(err < 0) {
		perror("Error in writing to file");
		ret = err;
		goto write_error;
	}


write_error:
	err = ioctl(pair->channel, RELAYFS_PUT_SUBBUF, &consumed_old);
	if(err != 0) {
		if(errno == -EFAULT) {
			perror("Error in unreserving sub buffer");
			ret = -EFAULT;
		} else if(errno == -EIO) {
			perror("Reader has been pushed by the writer, last subbuffer corrupted.");
			ret = -EIO;
		}
		goto get_error;
	}

get_error:
	return ret;
}


/* read_channels
 *
 * Read the realyfs channels and write them in the paired tracefiles.
 *
 * @fd_pairs : paired channels and trace files.
 *
 * returns 0 on success, -1 on error.
 *
 * Note that the high priority polled channels are consumed first. We then poll
 * again to see if these channels are still in priority. Only when no
 * high priority channel is left, we start reading low priority channels.
 *
 * Note that a channel is considered high priority when the buffer is almost
 * full.
 */

int read_channels(struct channel_trace_fd *fd_pairs)
{
	struct pollfd *pollfd;
	int i,j;
	int num_rdy, num_hup;
	int high_prio;
	int ret;

	if(fd_pairs->num_pairs <= 0) {
		printf("No channel to read\n");
		goto end;
	}
	
	/* Get the subbuf sizes and number */

	for(i=0;i<fd_pairs->num_pairs;i++) {
		struct fd_pair *pair = &fd_pairs->pair[i];

		ret = ioctl(pair->channel, RELAYFS_GET_N_SUBBUFS, 
							&pair->n_subbufs);
		if(ret != 0) {
			perror("Error in getting the number of subbuffers");
			goto end;
		}
		ret = ioctl(pair->channel, RELAYFS_GET_SUBBUF_SIZE, 
							&pair->subbuf_size);
		if(ret != 0) {
			perror("Error in getting the size of the subbuffers");
			goto end;
		}
	}

	/* Mmap each FD */
	for(i=0;i<fd_pairs->num_pairs;i++) {
		struct fd_pair *pair = &fd_pairs->pair[i];

		pair->mmap = mmap(0, pair->subbuf_size * pair->n_subbufs, PROT_READ,
				MAP_SHARED, pair->channel, 0);
		if(pair->mmap == MAP_FAILED) {
			perror("Mmap error");
			goto munmap;
		}
	}


	/* Start polling the FD */
	
	pollfd = malloc(fd_pairs->num_pairs * sizeof(struct pollfd));

	/* Note : index in pollfd is the same index as fd_pair->pair */
	for(i=0;i<fd_pairs->num_pairs;i++) {
		pollfd[i].fd = fd_pairs->pair[i].channel;
		pollfd[i].events = POLLIN|POLLPRI;
	}

	while(1) {
		high_prio = 0;
		num_hup = 0; 
#ifdef DEBUG
		printf("Press a key for next poll...\n");
		char buf[1];
		read(STDIN_FILENO, &buf, 1);
		printf("Next poll (polling %d fd) :\n", fd_pairs->num_pairs);
#endif //DEBUG
		
		/* Have we received a signal ? */
		if(quit_program) break;
		
		num_rdy = poll(pollfd, fd_pairs->num_pairs, -1);
		if(num_rdy == -1) {
			perror("Poll error");
			goto free_fd;
		}

		printf("Data received\n");

		for(i=0;i<fd_pairs->num_pairs;i++) {
			switch(pollfd[i].revents) {
				case POLLERR:
					printf("Error returned in polling fd %d.\n", pollfd[i].fd);
					num_hup++;
					break;
				case POLLHUP:
					printf("Polling fd %d tells it has hung up.\n", pollfd[i].fd);
					num_hup++;
					break;
				case POLLNVAL:
					printf("Polling fd %d tells fd is not open.\n", pollfd[i].fd);
					num_hup++;
					break;
				case POLLPRI:
					printf("Urgent read on fd %d\n", pollfd[i].fd);
					/* Take care of high priority channels first. */
					high_prio = 1;
					ret |= read_subbuffer(&fd_pairs->pair[i]);
					break;
			}
		}
		/* If every FD has hung up, we end the read loop here */
		if(num_hup == fd_pairs->num_pairs) break;

		if(!high_prio) {
			for(i=0;i<fd_pairs->num_pairs;i++) {
				switch(pollfd[i].revents) {
					case POLLIN:
						/* Take care of low priority channels. */
						printf("Normal read on fd %d\n", pollfd[i].fd);
						ret |= read_subbuffer(&fd_pairs->pair[i]);
						break;
				}
			}
		}

	}

free_fd:
	free(pollfd);

	/* munmap only the successfully mmapped indexes */
	i = fd_pairs->num_pairs;
munmap:
		/* Munmap each FD */
	for(j=0;j<i;j++) {
		struct fd_pair *pair = &fd_pairs->pair[j];
		int err_ret;

		err_ret = munmap(pair->mmap, pair->subbuf_size * pair->n_subbufs);
		if(err_ret != 0) {
			perror("Error in munmap");
		}
		ret |= err_ret;
	}

end:
	return ret;
}


void close_channel_trace_pairs(struct channel_trace_fd *fd_pairs)
{
	int i;
	int ret;

	for(i=0;i<fd_pairs->num_pairs;i++) {
		ret = close(fd_pairs->pair[i].channel);
		if(ret == -1) perror("Close error on channel");
		ret = close(fd_pairs->pair[i].trace);
		if(ret == -1) perror("Close error on trace");
	}
	free(fd_pairs->pair);
}

int main(int argc, char ** argv)
{
	int ret;
	struct channel_trace_fd fd_pairs = { NULL, 0 };
	struct sigaction act;
	
	ret = parse_arguments(argc, argv);

	if(ret != 0) show_arguments();
	if(ret < 0) return EINVAL;
	if(ret > 0) return 0;

	show_info();

	if(daemon_mode) {
		ret = daemon(0, 0);
    
    if(ret == -1) {
      perror("An error occured while daemonizing.");
      exit(-1);
    }
  }

	/* Connect the signal handlers */
	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&(act.sa_mask));
	sigaddset(&(act.sa_mask), SIGTERM);
	sigaddset(&(act.sa_mask), SIGQUIT);
	sigaddset(&(act.sa_mask), SIGINT);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGINT, &act, NULL);

	//return 0;
	if(ret = open_channel_trace_pairs(channel_name, trace_name, &fd_pairs))
		goto close_channel;

	ret = read_channels(&fd_pairs);

close_channel:
	close_channel_trace_pairs(&fd_pairs);

	return ret;
}
