/* LTTng user-space "fast" library
 *
 * This daemon is spawned by each traced thread (to share the mmap).
 *
 * Its job is to dump periodically this buffer to disk (when it receives a
 * SIGUSR1 from its parent).
 *
 * It uses the control information in the shared memory area (producer/consumer
 * count).
 *
 * When the parent thread dies (yes, those thing may happen) ;) , this daemon
 * will flush the last buffer and write it to disk.
 *
 * Supplement note for streaming : the daemon is responsible for flushing
 * periodically the buffer if it is streaming data.
 * 
 *
 * Notes :
 * shm memory is typically limited to 4096 units (system wide limit SHMMNI in
 * /proc/sys/kernel/shmmni). As it requires computation time upon creation, we
 * do not use it : we will use a shared mmap() instead which is passed through
 * the fork().
 * MAP_SHARED mmap segment. Updated when msync or munmap are called.
 * MAP_ANONYMOUS.
 * Memory  mapped  by  mmap()  is  preserved across fork(2), with the same
 *   attributes.
 * 
 * Eventually, there will be two mode :
 * * Slow thread spawn : a fork() is done for each new thread. If the process
 *   dies, the data is not lost.
 * * Fast thread spawn : a pthread_create() is done by the application for each
 *   new thread.
 *
 * We use a timer to check periodically if the parent died. I think it is less
 * intrusive than a ptrace() on the parent, which would get every signal. The
 * side effect of this is that we won't be notified if the parent does an
 * exec(). In this case, we will just sit there until the parent exits.
 * 
 *   
 * Copyright 2006 Mathieu Desnoyers
 *
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <syscall.h>
#include <features.h>
#include <pthread.h>
#include <malloc.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/param.h>
#include <linux/futex.h>
#include <sys/time.h>

#include <asm/timex.h>	//for get_cycles()

#include "ltt-usertrace-fast.h"


/* Writer (the traced application) */

__thread struct ltt_trace_info *thread_trace_info = NULL;

void ltt_usertrace_fast_buffer_switch(void)
{
	struct ltt_trace_info *tmp = thread_trace_info;
	if(tmp)
		kill(tmp->daemon_id, SIGUSR1);
}

/* The cleanup should never be called from a signal handler */
static void ltt_usertrace_fast_cleanup(void *arg)
{
	struct ltt_trace_info *tmp = thread_trace_info;
	if(tmp) {
		thread_trace_info = NULL;
		kill(tmp->daemon_id, SIGUSR2);
		munmap(tmp, sizeof(*tmp));
	}
}

/* Reader (the disk dumper daemon) */

static pid_t traced_pid = 0;
static pthread_t traced_thread = 0;
static int parent_exited = 0;

/* signal handling */
static void handler_sigusr1(int signo)
{
	printf("LTT Signal %d received : parent buffer switch.\n", signo);
}

static void handler_sigusr2(int signo)
{
	printf("LTT Signal %d received : parent exited.\n", signo);
	parent_exited = 1;
}

static void handler_sigalarm(int signo)
{
	printf("LTT Signal %d received\n", signo);

	if(getppid() != traced_pid) {
		/* Parent died */
		printf("LTT Parent %lu died, cleaning up\n", traced_pid);
		traced_pid = 0;
	}
	alarm(3);
}

/* Do a buffer switch. Don't switch if buffer is completely empty */
static void flush_buffer(struct ltt_buf *ltt_buf)
{


}

static inline int ltt_buffer_get(struct ltt_buf *ltt_buf,
		unsigned int *offset)
{
	unsigned int consumed_old, consumed_idx;
	consumed_old = atomic_read(&ltt_buf->consumed);
	consumed_idx = SUBBUF_INDEX(consumed_old, ltt_buf);
	
	if(atomic_read(&ltt_buf->commit_count[consumed_idx])
		!= atomic_read(&ltt_buf->reserve_count[consumed_idx])) {
		return -EAGAIN;
	}
	if((SUBBUF_TRUNC(atomic_read(&ltt_buf->offset), ltt_buf)
				-SUBBUF_TRUNC(consumed_old, ltt_buf)) == 0) {
		return -EAGAIN;
	}
	
	*offset = consumed_old;

	return 0;
}

static inline int ltt_buffer_put(struct ltt_buf *ltt_buf,
		unsigned int offset)
{
	unsigned int consumed_old, consumed_new;
	int ret;

	consumed_old = offset;
	consumed_new = SUBBUF_ALIGN(consumed_old, ltt_buf);
	if(atomic_cmpxchg(&ltt_buf->consumed, consumed_old, consumed_new)
			!= consumed_old) {
		/* We have been pushed by the writer : the last buffer read _is_
		 * corrupted!
		 * It can also happen if this is a buffer we never got. */
		return -EIO;
	} else {
		if(atomic_read(&ltt_buf->full) == 1) {
			/* tell the client that buffer is now unfull */
			ret = futex(&ltt_buf->full, FUTEX_WAKE, 1, NULL, NULL, 0);
			if(ret != 1) {
				printf("LTT warning : race condition : writer not waiting or too many writers\n");
			}
			atomic_set(&ltt_buf->full, 0);
		}
	}
}

/* In the writer :
 *
 * if(buffer full condition) {
 *   put myself in the wait queue
 *   ltt_buf->full = 1;
 *   schedule
 * }
 *{
	if(buffer_is_full) {
		atomic_set(&ltt_buf->full, 1);
		ret = futex(&ltt_buf->full, 1, NULL, NULL, 0);
	}
}

 */

static int read_subbuffer(struct ltt_buf *ltt_buf, int fd)
{
	int err;
	printf("LTT read buffer\n");


	err = ltt_buffer_get(&shared_trace_info->channel.cpu, &consumed_old);
	if(err != -EAGAIN && err != 0) {
		printf("LTT Reserving sub buffer failed\n");
		goto get_error;
	}

	err = TEMP_FAILURE_RETRY(write(fd,
				ltt_buf->start 
					+ (consumed_old & ((ltt_buf->alloc_size)-1)),
				ltt_buf->subbuf_size));

	if(err < 0) {
		perror("Error in writing to file");
		goto write_error;
	}
#if 0
	err = fsync(pair->trace);
	if(err < 0) {
		ret = errno;
		perror("Error in writing to file");
		goto write_error;
	}
#endif //0
write_error:
	err = ltt_buffer_put(&shared_trace_info->channel.cpu, consumed_old);

	if(err != 0) {
		if(err == -EIO) {
			perror("Reader has been pushed by the writer, last subbuffer corrupted.");
			/* FIXME : we may delete the last written buffer if we wish. */
		}
		goto get_error;
	}

get_error:
	return err;
}

/* This function is called by ltt_rw_init which has signals blocked */
static void ltt_usertrace_fast_daemon(struct ltt_trace_info *shared_trace_info,
		sigset_t oldset, pid_t l_traced_pid, pthread_t l_traced_thread)
{
	struct sigaction act;
	int ret;
	int fd_fac;
	int fd_cpu;
	char outfile_name[PATH_MAX];
	char identifier_name[PATH_MAX];


	traced_pid = l_traced_pid;
	traced_thread = l_traced_thread;

	printf("LTT ltt_usertrace_fast_daemon : init is %d, pid is %lu, traced_pid is %lu\n",
			shared_trace_info->init, getpid(), traced_pid);

	act.sa_handler = handler_sigusr1;
	act.sa_flags = 0;
	sigemptyset(&(act.sa_mask));
	sigaddset(&(act.sa_mask), SIGUSR1);
	sigaction(SIGUSR1, &act, NULL);

	act.sa_handler = handler_sigusr2;
	act.sa_flags = 0;
	sigemptyset(&(act.sa_mask));
	sigaddset(&(act.sa_mask), SIGUSR2);
	sigaction(SIGUSR2, &act, NULL);

	act.sa_handler = handler_sigalarm;
	act.sa_flags = 0;
	sigemptyset(&(act.sa_mask));
	sigaddset(&(act.sa_mask), SIGALRM);
	sigaction(SIGALRM, &act, NULL);

	/* Enable signals */
	ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
	if(ret) {
		printf("LTT Error in pthread_sigmask\n");
	}

	alarm(3);

	/* Open output files */
	umask(00000);
	ret = mkdir(LTT_USERTRACE_ROOT, 0777);
	if(ret < 0 && errno != EEXIST) {
		perror("LTT Error in creating output (mkdir)");
		exit(-1);
	}
	ret = chdir(LTT_USERTRACE_ROOT);
	if(ret < 0) {
		perror("LTT Error in creating output (chdir)");
		exit(-1);
	}
	snprintf(identifier_name, PATH_MAX-1,	"%lu.%lu.%llu",
			traced_pid, traced_thread, get_cycles());
	snprintf(outfile_name, PATH_MAX-1,	"facilities-%s", identifier_name);
	fd_fac = creat(outfile_name, 0644);

	snprintf(outfile_name, PATH_MAX-1,	"cpu-%s", identifier_name);
	fd_cpu = creat(outfile_name, 0644);
	
	
	while(1) {
		pause();
		if(traced_pid == 0) break; /* parent died */
		if(parent_exited) break;
		printf("LTT Doing a buffer switch read. pid is : %lu\n", getpid());
	
		do {
			ret = read_buffer(&shared_trace_info->channel.cpu, fd_cpu);
		} while(ret == 0);

		do {
			ret = read_buffer(&shared_trace_info->channel.facilities, fd_fac);
		} while(ret == 0);
	}

	/* Buffer force switch (flush) */
	flush_buffer(&shared_trace_info->channel.cpu);
	do {
		ret = read_buffer(&shared_trace_info->channel.cpu, fd_cpu);
	} while(ret == 0);


	flush_buffer(&shared_trace_info->channel.facilities);
	do {
		ret = read_buffer(&shared_trace_info->channel.facilities, fd_fac);
	} while(ret == 0);

	close(fd_fac);
	close(fd_cpu);
	
	/* The parent thread is dead and we have finished with the buffer */
	munmap(shared_trace_info, sizeof(*shared_trace_info));
	
	exit(0);
}


/* Reader-writer initialization */

static enum ltt_process_role { LTT_ROLE_WRITER, LTT_ROLE_READER }
	role = LTT_ROLE_WRITER;


void ltt_rw_init(void)
{
	pid_t pid;
	struct ltt_trace_info *shared_trace_info;
	int ret;
	sigset_t set, oldset;
	pid_t l_traced_pid = getpid();
	pthread_t l_traced_thread = pthread_self();

	/* parent : create the shared memory map */
	shared_trace_info = mmap(0, sizeof(*thread_trace_info),
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	memset(shared_trace_info, 0, sizeof(*shared_trace_info));
	/* Tricky semaphore : is in a shared memory space, so it's ok for a fast
	 * mutex (futex). */
	atomic_set(&shared_trace_info->channel.facilities.full, 0);
	shared_trace_info->channel.facilities.alloc_size = LTT_BUF_SIZE_FACILITIES;
	shared_trace_info->channel.facilities.subbuf_size = LTT_SUBBUF_SIZE_FACILITIES;
	atomic_set(&shared_trace_info->channel.cpu.full, 0);
	shared_trace_info->channel.cpu.alloc_size = LTT_BUF_SIZE_CPU;
	shared_trace_info->channel.cpu.subbuf_size = LTT_SUBBUF_SIZE_CPU;
	shared_trace_info->init = 1;

	/* Disable signals */
  ret = sigfillset(&set);
  if(ret) {
    printf("LTT Error in sigfillset\n");
  } 
	
	
  ret = pthread_sigmask(SIG_BLOCK, &set, &oldset);
  if(ret) {
    printf("LTT Error in pthread_sigmask\n");
  }

	pid = fork();
	if(pid > 0) {
		/* Parent */
		shared_trace_info->daemon_id = pid;
		thread_trace_info = shared_trace_info;

		/* Enable signals */
		ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
		if(ret) {
			printf("LTT Error in pthread_sigmask\n");
		}
	} else if(pid == 0) {
		/* Child */
		role = LTT_ROLE_READER;
		ltt_usertrace_fast_daemon(shared_trace_info, oldset, l_traced_pid,
					l_traced_thread);
		/* Should never return */
		exit(-1);
	} else if(pid < 0) {
		/* fork error */
		perror("LTT Error in forking ltt-usertrace-fast");
	}
}

static __thread struct _pthread_cleanup_buffer cleanup_buffer;

void ltt_thread_init(void)
{
	_pthread_cleanup_push(&cleanup_buffer, ltt_usertrace_fast_cleanup, NULL);
	ltt_rw_init();
}
	
void __attribute__((constructor)) __ltt_usertrace_fast_init(void)
{
  printf("LTT usertrace-fast init\n");

	ltt_rw_init();
}

void __attribute__((destructor)) __ltt_usertrace_fast_fini(void)
{
	if(role == LTT_ROLE_WRITER) {
	  printf("LTT usertrace-fast fini\n");
		ltt_usertrace_fast_cleanup(NULL);
	}
}

