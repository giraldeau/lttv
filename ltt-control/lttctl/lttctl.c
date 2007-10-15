/* lttctl
 *
 * Linux Trace Toolkit Control
 *
 * Small program that controls LTT through libltt.
 *
 * Copyright 2005 -
 * 	Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <liblttctl/lttctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

/* Buffer for file copy : 4k seems optimal. */
#define BUF_SIZE 4096

enum trace_ctl_op {
	CTL_OP_CREATE_START,
	CTL_OP_CREATE,
	CTL_OP_DESTROY,
	CTL_OP_STOP_DESTROY,
	CTL_OP_START,
	CTL_OP_STOP,
	CTL_OP_DAEMON,
	CTL_OP_DAEMON_HYBRID_FINISH,
	CTL_OP_NONE
};

static char *trace_name = NULL;
static char *trace_type = "relay";
static char *mode_name = NULL;
static unsigned subbuf_size_low = 0;
static unsigned n_subbufs_low = 0;
static unsigned subbuf_size_med = 0;
static unsigned n_subbufs_med = 0;
static unsigned subbuf_size_high = 0;
static unsigned n_subbufs_high = 0;
static unsigned append_trace = 0;
static enum trace_mode mode = LTT_TRACE_NORMAL;
static enum trace_ctl_op op = CTL_OP_NONE;
static char *channel_root = NULL;
static char *trace_root = NULL;
static char *num_threads = "1";

/* Args :
 *
 */
void show_arguments(void)
{
	printf("Please use the following arguments :\n");
	printf("\n");
	printf("-n name       Name of the trace.\n");
	printf("-b            Create trace channels and start tracing (no daemon).\n");
	printf("-c            Create trace channels.\n");
	printf("-m mode       Normal, flight recorder or hybrid mode.\n");
	printf("              Mode values : normal (default), flight or hybrid.\n");
	printf("-r            Destroy trace channels.\n");
	printf("-R            Stop tracing and destroy trace channels.\n");
	printf("-s            Start tracing.\n");
	//printf("              Note : will automatically create a normal trace if "
	//                      "none exists.\n");
	printf("-q            Stop tracing.\n");
	printf("-d            Create trace, spawn a lttd daemon, start tracing.\n");
	printf("              (optionally, you can set LTT_DAEMON\n");
	printf("              env. var.)\n");
	printf("-f            Stop tracing, dump flight recorder trace, destroy channels\n");
	printf("              (for hybrid traces)\n");
	printf("-t            Trace root path. (ex. /root/traces/example_trace)\n");
	printf("-T            Type of trace (ex. relay)\n");
	printf("-l            LTT channels root path. (ex. /mnt/debugfs/ltt)\n");
	printf("-Z            Size of the low data rate subbuffers (will be rounded to next page size)\n");
	printf("-X            Number of low data rate subbuffers\n");
	printf("-V            Size of the medium data rate subbuffers (will be rounded to next page size)\n");
	printf("-B            Number of medium data rate subbuffers\n");
	printf("-z            Size of the high data rate subbuffers (will be rounded to next page size)\n");
	printf("-x            Number of high data rate subbuffers\n");
	printf("-a            Append to trace\n");
	printf("-N            Number of lttd threads\n");
	printf("\n");
}


/* parse_arguments
 *
 * Parses the command line arguments.
 *
 * Returns -1 if the arguments were correct, but doesn't ask for program
 * continuation. Returns EINVAL if the arguments are incorrect, or 0 if OK.
 */
int parse_arguments(int argc, char **argv)
{
	int ret = 0;
	int argn = 1;
	
	if(argc == 2) {
		if(strcmp(argv[1], "-h") == 0) {
			return -1;
		}
	}

	while(argn < argc) {

		switch(argv[argn][0]) {
			case '-':
				switch(argv[argn][1]) {
					case 'n':
						if(argn+1 < argc) {
							trace_name = argv[argn+1];
							argn++;
						} else {
							printf("Specify a trace name after -n.\n");
							printf("\n");
							ret = EINVAL;
						}

						break;
					case 'b':
						op = CTL_OP_CREATE_START;
						break;
					case 'c':
						op = CTL_OP_CREATE;
						break;
					case 'm':
						if(argn+1 < argc) {
							mode_name = argv[argn+1];
							argn++;
							if(strcmp(mode_name, "normal") == 0)
								mode = LTT_TRACE_NORMAL;
							else if(strcmp(mode_name, "flight") == 0)
								mode = LTT_TRACE_FLIGHT;
							else if(strcmp(mode_name, "hybrid") == 0)
								mode = LTT_TRACE_HYBRID;
							else {
								printf("Invalid mode '%s'.\n", argv[argn]);
								printf("\n");
								ret = EINVAL;
							}
						} else {
								printf("Specify a mode after -m.\n");
								printf("\n");
								ret = EINVAL;
						}
						break;
					case 'r':
						op = CTL_OP_DESTROY;
						break;
					case 'R':
						op = CTL_OP_STOP_DESTROY;
						break;
					case 's':
						op = CTL_OP_START;
						break;
					case 'q':
						op = CTL_OP_STOP;
						break;
					case 'Z':
						if(argn+1 < argc) {
							subbuf_size_low = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a number of low traffic subbuffers after -Z.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'X':
						if(argn+1 < argc) {
							n_subbufs_low = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a low traffic subbuffer size after -X.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'V':
						if(argn+1 < argc) {
							subbuf_size_med = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a number of medium traffic subbuffers after -V.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'B':
						if(argn+1 < argc) {
							n_subbufs_med = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a medium traffic subbuffer size after -B.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'z':
						if(argn+1 < argc) {
							subbuf_size_high = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a number of high traffic subbuffers after -z.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'x':
						if(argn+1 < argc) {
							n_subbufs_high = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a high traffic subbuffer size after -x.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'd':
						op = CTL_OP_DAEMON;
						break;
					case 'f':
						op = CTL_OP_DAEMON_HYBRID_FINISH;
						break;
					case 't':
						if(argn+1 < argc) {
							trace_root = argv[argn+1];
							argn++;
						} else {
							printf("Specify a trace root path after -t.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'l':
						if(argn+1 < argc) {
							channel_root = argv[argn+1];
							argn++;
						} else {
							printf("Specify a channel root path after -l.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'a':
						append_trace = 1;
						break;
					case 'N':
						if(argn+1 < argc) {
							num_threads = argv[argn+1];
							argn++;
						}
						break;
					case 'T':
						if(argn+1 < argc) {
							trace_type = argv[argn+1];
							argn++;
						} else {
							printf("Specify a trace type after -T.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					default:
						printf("Invalid argument '%s'.\n", argv[argn]);
						printf("\n");
						ret = EINVAL;
				}
				break;
			default:
				printf("Invalid argument '%s'.\n", argv[argn]);
				printf("\n");
				ret = EINVAL;
		}
		argn++;
	}
	
	if(trace_name == NULL) {
		printf("Please specify a trace name.\n");
		printf("\n");
		ret = EINVAL;
	}

	if(op == CTL_OP_NONE) {
		printf("Please specify an operation.\n");
		printf("\n");
		ret = EINVAL;
	}

	if(op == CTL_OP_DAEMON || op == CTL_OP_DAEMON_HYBRID_FINISH) {
		if(trace_root == NULL) {
			printf("Please specify -t trace_root_path with the -d option.\n");
			printf("\n");
			ret = EINVAL;
		}
		if(channel_root == NULL) {
			printf("Please specify -l ltt_root_path with the -d option.\n");
			printf("\n");
			ret = EINVAL;
		}
	}

	return ret;
}

void show_info(void)
{
	printf("Linux Trace Toolkit Trace Control " VERSION"\n");
	printf("\n");
	if(trace_name != NULL) {
		printf("Controlling trace : %s\n", trace_name);
		printf("\n");
	}
}

int lttctl_daemon(struct lttctl_handle *handle, char *trace_name)
{
	char channel_path[PATH_MAX] = "";
	pid_t pid;
	int ret;
	char *lttd_path = getenv("LTT_DAEMON");

	if(lttd_path == NULL) lttd_path = 
		PACKAGE_BIN_DIR "/lttd";
	
	strcat(channel_path, channel_root);
	strcat(channel_path, "/");
	strcat(channel_path, trace_name);

	
	ret = lttctl_create_trace(handle, trace_name, mode, trace_type,
		subbuf_size_low, n_subbufs_low,
		subbuf_size_med, n_subbufs_med,
		subbuf_size_high, n_subbufs_high);
	if(ret != 0) goto create_error;

	pid = fork();

	if(pid > 0) {
		int status = 0;
		/* parent */
		
		ret = waitpid(pid, &status, 0);
		if(ret == -1) {
			ret = errno;
			perror("Error in waitpid");
			goto start_error;
		}

		ret = 0;
		if(WIFEXITED(status))
			ret = WEXITSTATUS(status);
		if(ret) goto start_error;

	} else if(pid == 0) {
		/* child */
		int ret;
		if(mode != LTT_TRACE_HYBRID) {
			if(append_trace) 
				ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
					 channel_path, "-d", "-a", "-N", num_threads, NULL);
			else
				ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
					 channel_path, "-d", "-N", num_threads, NULL);
		} else {
			if(append_trace) 
				ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
					 channel_path, "-d", "-a", "-N", num_threads, "-n", NULL);
			else
				ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
					 channel_path, "-d", "-N", num_threads, "-n", NULL);
		}
		if(ret) {
			ret = errno;
			perror("Error in executing the lttd daemon");
			exit(ret);
		}
	} else {
		/* error */
		perror("Error in forking for lttd daemon");
	}

	ret = lttctl_start(handle, trace_name);
	if(ret != 0) goto start_error;

	return 0;

	/* error handling */
start_error:
	printf("Trace start error\n");
	ret |= lttctl_destroy_trace(handle, trace_name);
create_error:
	return ret;
}




int lttctl_daemon_hybrid_finish(struct lttctl_handle *handle, char *trace_name)
{
	char channel_path[PATH_MAX] = "";
	pid_t pid;
	int ret;
	char *lttd_path = getenv("LTT_DAEMON");

	if(lttd_path == NULL) lttd_path = 
		PACKAGE_BIN_DIR "/lttd";
	
	strcat(channel_path, channel_root);
	strcat(channel_path, "/");
	strcat(channel_path, trace_name);

	
	ret = lttctl_stop(handle, trace_name);
	if(ret != 0) goto stop_error;

	pid = fork();

	if(pid > 0) {
		int status = 0;
		/* parent */
		
		ret = waitpid(pid, &status, 0);
		if(ret == -1) {
			ret = errno;
			perror("Error in waitpid");
			goto destroy_error;
		}

		ret = 0;
		if(WIFEXITED(status))
			ret = WEXITSTATUS(status);
		if(ret) goto destroy_error;

	} else if(pid == 0) {
		/* child */
		int ret;
		if(append_trace) 
			ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
					 channel_path, "-d", "-a", "-N", num_threads, "-f", NULL);
		else
			ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
					 channel_path, "-d", "-N", num_threads, "-f", NULL);
		if(ret) {
			ret = errno;
			perror("Error in executing the lttd daemon");
			exit(ret);
		}
	} else {
		/* error */
		perror("Error in forking for lttd daemon");
	}

	ret = lttctl_destroy_trace(handle, trace_name);
	if(ret != 0) goto destroy_error;

	return 0;

	/* error handling */
destroy_error:
	printf("Hybrid trace destroy error\n");
stop_error:
	return ret;
}



int main(int argc, char ** argv)
{
	int ret;
	struct lttctl_handle *handle;
	
	ret = parse_arguments(argc, argv);

	if(ret != 0) show_arguments();
	if(ret == EINVAL) return EINVAL;
	if(ret == -1) return 0;

	show_info();
	
	handle = lttctl_create_handle();
	
	if(handle == NULL) return -1;
	
	switch(op) {
		case CTL_OP_CREATE_START:
			ret = lttctl_create_trace(handle, trace_name, mode, trace_type,
			subbuf_size_low, n_subbufs_low,
			subbuf_size_med, n_subbufs_med,
			subbuf_size_high, n_subbufs_high);
			if(!ret)
				ret = lttctl_start(handle, trace_name);
			break;
		case CTL_OP_CREATE:
			ret = lttctl_create_trace(handle, trace_name, mode, trace_type,
			subbuf_size_low, n_subbufs_low,
			subbuf_size_med, n_subbufs_med,
			subbuf_size_high, n_subbufs_high);
			break;
		case CTL_OP_DESTROY:
			ret = lttctl_destroy_trace(handle, trace_name);
			break;
		case CTL_OP_STOP_DESTROY:
			ret = lttctl_stop(handle, trace_name);
			if(!ret)
				ret = lttctl_destroy_trace(handle, trace_name);
			break;
		case CTL_OP_START:
			ret = lttctl_start(handle, trace_name);
			break;
		case CTL_OP_STOP:
			ret = lttctl_stop(handle, trace_name);
			break;
		case CTL_OP_DAEMON:
			ret = lttctl_daemon(handle, trace_name);
			break;
		case CTL_OP_DAEMON_HYBRID_FINISH:
			ret = lttctl_daemon_hybrid_finish(handle, trace_name);
			break;
		case CTL_OP_NONE:
			break;
	}

	ret |= lttctl_destroy_handle(handle);
	
	return ret;
}
