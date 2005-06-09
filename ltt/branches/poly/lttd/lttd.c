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

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/poll.h>

enum {
	GET_SUBBUF,
	PUT_SUBBUF,
	GET_N_BUBBUFS,
	GET_SUBBUF_SIZE
};

struct fd_pair {
	int channel;
	int trace;
};

struct channel_trace_fd {
	struct fd_pair *pair;
	int num_pairs;
};

static char *trace_name = NULL;
static char *channel_name = NULL;
static int	daemon_mode = 0;
static int	append_mode = 0;

/* Args :
 *
 * -t directory		Directory name of the trace to write to. Will be created.
 * -c directory		Root directory of the relayfs trace channels.
 * -d          		Run in background (daemon).
 * -a							Trace append mode.
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

	if(channel_dir == NULL) {
		perror(subchannel_name);
		return ENOENT;
	}

	//FIXME : check if the directory already exist, and ask the user if he wants
	//to append to the traces.
	printf("Creating trace subdirectory %s\n", subtrace_name);
	ret = mkdir(subtrace_name, S_IRWXU|S_IRWXG|S_IRWXO);
	if(ret == -1) {
		if(errno == EEXIST && append_mode) {
			printf("Appending to directory %s as resquested\n", subtrace_name);
		} else {
			perror(subtrace_name);
			return -1;
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
			}
			/* Open the trace in write mode, only append if append_mode */
			ret = stat(path_trace, &stat_buf);
			if(ret == 0) {
				if(append_mode) {
					printf("Appending to file %s as resquested\n", path_trace);

					fd_pairs->pair[fd_pairs->num_pairs-1].trace = 
						open(path_trace, O_WRONLY|O_APPEND,
								S_IRWXU|S_IRWXG|S_IRWXO);

					if(fd_pairs->pair[fd_pairs->num_pairs-1].trace == -1) {
						perror(path_trace);
					}
				} else {
					printf("File %s exists, cannot open. Try append mode.\n", path_trace);
					return -1;
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
	
	closedir(channel_dir);

	return 0;
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
	int i;
	int num_rdy;

	pollfd = malloc(fd_pairs->num_pairs * sizeof(struct pollfd));

	for(i=0;i<fd_pairs->num_pairs;i++) {
		pollfd[i].fd = fd_pairs->pair[i].channel;
		pollfd[i].events = POLLIN|POLLPRI;
	}

	while(1) {
		
		num_rdy = poll(pollfd, fd_pairs->num_pairs, -1);
		if(num_rdy == -1) {
			perror("Poll error");
			return -1;
		}

		printf("Data received\n");

		for(i=0;i<fd_pairs->num_pairs;i++) {
			switch(pollfd[i].revents) {
				case POLLERR:
					printf("Error returned in polling fd %d.\n", pollfd[i].fd);
					break;
				case POLLHUP:
					printf("Polling fd %d tells it has hung up.\n", pollfd[i].fd);
					break;
				case POLLNVAL:
					printf("Polling fd %d tells fd is not open.\n", pollfd[i].fd);
					break;
				case POLLPRI:
					/* Take care of high priority channels first. */
					break;
				default:
		}

		for(i=0;i<fd_pairs->num_pairs;i++) {
			switch(pollfd[i].revents) {
				case POLLIN:
					/* Take care of low priority channels. */
					break;
				default:
		}

	}

	free(pollfd);

	return 0;
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
	pid_t pid;
	struct channel_trace_fd fd_pairs = { NULL, 0 };
	
	ret = parse_arguments(argc, argv);

	if(ret != 0) show_arguments();
	if(ret < 0) return EINVAL;
	if(ret > 0) return 0;

	show_info();

	if(daemon_mode) {
		pid = fork();
		
		if(pid > 0) {
			/* parent */
			return 0;
		} else if(pid < 0) {
			/* error */
			printf("An error occured while forking.\n");
			return -1;
		}
		/* else, we are the child, continue... */
	}

	if(ret = open_channel_trace_pairs(channel_name, trace_name, &fd_pairs))
		goto close_channel;

	ret = read_channels(&fd_pairs);

close_channel:
	close_channel_trace_pairs(&fd_pairs);

	return ret;
}
