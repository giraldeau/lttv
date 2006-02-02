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
	CTL_OP_DESCRIPTION,
	CTL_OP_NONE
};

static char *trace_name = NULL;
static char *mode_name = NULL;
static unsigned subbuf_size = 0;
static unsigned n_subbufs = 0;
static unsigned append_trace = 0;
static enum trace_mode mode = LTT_TRACE_NORMAL;
static enum trace_ctl_op op = CTL_OP_NONE;
static char *channel_root = NULL;
static char *trace_root = NULL;
static char *num_threads = "1";

static int sigchld_received = 0;

void sigchld_handler(int signo)
{
	printf("signal %d received\n", signo);
	sigchld_received = 1;
}


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
	printf("-m mode       Normal or flight recorder mode.\n");
	printf("              Mode values : normal (default) or flight.\n");
	printf("-r            Destroy trace channels.\n");
	printf("-R            Stop tracing and destroy trace channels.\n");
	printf("-s            Start tracing.\n");
	//printf("              Note : will automatically create a normal trace if "
	//											"none exists.\n");
	printf("-q            Stop tracing.\n");
	printf("-d            Create trace, spawn a lttd daemon, start tracing.\n");
	printf("              (optionnaly, you can set LTT_DAEMON\n");
	printf("              and the LTT_FACILITIES env. vars.)\n");
	printf("-t            Trace root path. (ex. /root/traces/example_trace)\n");
	printf("-l            LTT channels root path. (ex. /mnt/relayfs/ltt)\n");
	printf("-z            Size of the subbuffers (will be rounded to next page size)\n");
	printf("-x            Number of subbuffers\n");
	printf("-e            Get XML facilities description\n");
	printf("-a            Append to trace\n");
	printf("-N            Number of ltt threads\n");
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
					case 'z':
						if(argn+1 < argc) {
							subbuf_size = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a number of subbuffers after -z.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'x':
						if(argn+1 < argc) {
							n_subbufs = (unsigned)atoi(argv[argn+1]);
							argn++;
						} else {
							printf("Specify a subbuffer size after -x.\n");
							printf("\n");
							ret = EINVAL;
						}
						break;
					case 'd':
						op = CTL_OP_DAEMON;
						break;
          case 'e':
            op = CTL_OP_DESCRIPTION;
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
	
	if(op != CTL_OP_DESCRIPTION && trace_name == NULL) {
		printf("Please specify a trace name.\n");
		printf("\n");
		ret = EINVAL;
	}

	if(op == CTL_OP_NONE) {
		printf("Please specify an operation.\n");
		printf("\n");
		ret = EINVAL;
	}

	if(op == CTL_OP_DAEMON) {
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

  if(op == CTL_OP_DESCRIPTION) {
    if(trace_root == NULL) {
			printf("Please specify -t trace_root_path with the -e option.\n");
			printf("\n");
			ret = EINVAL;
    }
  }

	return ret;
}

void show_info(void)
{
	printf("Linux Trace Toolkit Trace Control\n");
	printf("\n");
  if(trace_name != NULL) {
  	printf("Controlling trace : %s\n", trace_name);
  	printf("\n");
  }
}

int create_eventdefs(void)
{
  int ret = 0;
  char eventdefs_path[PATH_MAX];
  char eventdefs_file[PATH_MAX];
  char facilities_file[PATH_MAX];
  char read_buf[BUF_SIZE];
  struct dirent *entry;
	char *facilities_path = getenv("LTT_FACILITIES");
	if(facilities_path == NULL) facilities_path =
          PACKAGE_DATA_DIR "/" PACKAGE "/facilities";

  ret = mkdir(trace_root, S_IRWXU|S_IRWXG|S_IRWXO);
  if(ret == -1 && errno != EEXIST) {
    ret = errno;
    perror("Cannot create trace_root directory");
    printf("trace_root is %s\n", trace_root);
    goto error;
  }
  ret = 0;
  
  size_t trace_root_len = strlen(trace_root);
  strncpy(eventdefs_path, trace_root, PATH_MAX);
  strncat(eventdefs_path, "/eventdefs/", PATH_MAX - trace_root_len);
  size_t eventdefs_path_len = strlen(eventdefs_path);
  ret = mkdir(eventdefs_path, S_IRWXU|S_IRWXG|S_IRWXO);
  if(ret == -1 && (!append_trace || errno != EEXIST)) {
    ret = errno;
    perror("Cannot create eventdefs directory");
    goto error;
  }
  ret = 0;
  
  DIR *facilities_dir = opendir(facilities_path);
  
  if(facilities_dir == NULL) {
    perror("Cannot open facilities directory");
    ret = EEXIST;
    goto error;
  }

  while((entry = readdir(facilities_dir)) != NULL) {
    if(entry->d_name[0] == '.') continue;
    
    printf("Appending facility file %s\n", entry->d_name);
    strncpy(eventdefs_file, eventdefs_path, PATH_MAX);
    strncat(eventdefs_file, entry->d_name, PATH_MAX - eventdefs_path_len);
    /* Append to the file */
    FILE *dest = fopen(eventdefs_file, "a");
    if(!dest) {
      perror("Cannot create eventdefs file");
      continue;
    }
    strncpy(facilities_file, facilities_path, PATH_MAX);
    size_t facilities_dir_len = strlen(facilities_path);
    strncat(facilities_file, "/", PATH_MAX - facilities_dir_len);
    strncat(facilities_file, entry->d_name, PATH_MAX - facilities_dir_len-1);
    FILE *src = fopen(facilities_file, "r");
    if(!src) {
      ret = errno;
      perror("Cannot open eventdefs file for reading");
      goto close_dest;
    }

    do {
      size_t read_size, write_size;
      read_size = fread(read_buf, sizeof(char), BUF_SIZE, src);
      if(ferror(src)) {
        ret = errno;
        perror("Cannot read eventdefs file");
        goto close_src;
      }
      write_size = fwrite(read_buf, sizeof(char), read_size, dest);
      if(ferror(dest)) {
        ret = errno;
        perror("Cannot write eventdefs file");
        goto close_src;
      }
    } while(!feof(src));

    /* Add spacing between facilities */
    fwrite("\n", 1, 1, dest);
    
close_src:
    fclose(src);
close_dest:
    fclose(dest);
  }

  closedir(facilities_dir);

error:
  return ret;

}


int lttctl_daemon(struct lttctl_handle *handle, char *trace_name)
{
	char channel_path[PATH_MAX] = "";
	pid_t pid;
	int ret;
	char *lttd_path = getenv("LTT_DAEMON");
	struct sigaction act;

	if(lttd_path == NULL) lttd_path = 
    PACKAGE_BIN_DIR "/lttd";
	
	strcat(channel_path, channel_root);
	strcat(channel_path, "/");
	strcat(channel_path, trace_name);

	
	ret = lttctl_create_trace(handle, trace_name, mode, subbuf_size, n_subbufs);
	if(ret != 0) goto create_error;

	act.sa_handler = sigchld_handler;
	sigemptyset(&(act.sa_mask));
	sigaddset(&(act.sa_mask), SIGCHLD);
	sigaction(SIGCHLD, &act, NULL);
	
	pid = fork();

	if(pid > 0) {
    int status;
		/* parent */
		while(!(sigchld_received)) pause();

    waitpid(pid, &status, 0);
    ret = 0;
    if(WIFEXITED(status))
      ret = WEXITSTATUS(status);
    if(ret) goto start_error;

		printf("Creating supplementary trace files\n");
    ret = create_eventdefs();
    if(ret) goto start_error;

	} else if(pid == 0) {
		/* child */
    int ret;
    if(append_trace) 
  		ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
                       channel_path, "-d", "-a", "-N", num_threads, NULL);
    else
  		ret =	execlp(lttd_path, lttd_path, "-t", trace_root, "-c",
                       channel_path, "-d", "-N", num_threads, NULL);
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
			ret = lttctl_create_trace(handle, trace_name, mode, subbuf_size,
																n_subbufs);
      if(!ret)
        ret = lttctl_start(handle, trace_name);
      break;
  	case CTL_OP_CREATE:
			ret = lttctl_create_trace(handle, trace_name, mode, subbuf_size,
																n_subbufs);
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
    case CTL_OP_DESCRIPTION:
      ret = create_eventdefs();
      break;
		case CTL_OP_NONE:
			break;
	}

	ret |= lttctl_destroy_handle(handle);
	
	return ret;
}
