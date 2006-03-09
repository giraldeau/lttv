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

#define inline inline __attribute__((always_inline))

#define _GNU_SOURCE
#define LTT_TRACE
#define LTT_TRACE_FAST
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
#include <sys/time.h>
#include <errno.h>

#include <asm/atomic.h>
#include <asm/timex.h>	//for get_cycles()

_syscall0(pid_t,gettid)

#include <ltt/ltt-usertrace.h>

#ifdef LTT_SHOW_DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif //LTT_SHOW_DEBUG

	
enum force_switch_mode { FORCE_ACTIVE, FORCE_FLUSH };

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
static pid_t traced_tid = 0;
static int parent_exited = 0;
static int fd_process = -1;
static char outfile_name[PATH_MAX];
static char identifier_name[PATH_MAX];

/* signal handling */
static void handler_sigusr1(int signo)
{
	dbg_printf("LTT Signal %d received : parent buffer switch.\n", signo);
}

static void handler_sigusr2(int signo)
{
	dbg_printf("LTT Signal %d received : parent exited.\n", signo);
	parent_exited = 1;
}

static void handler_sigalarm(int signo)
{
	dbg_printf("LTT Signal %d received\n", signo);

	if(getppid() != traced_pid) {
		/* Parent died */
		dbg_printf("LTT Parent %lu died, cleaning up\n", traced_pid);
		traced_pid = 0;
	}
	alarm(3);
}

/* Do a buffer switch. Don't switch if buffer is completely empty */
static void flush_buffer(struct ltt_buf *ltt_buf, enum force_switch_mode mode)
{
	uint64_t tsc;
	int offset_begin, offset_end, offset_old;
	int reserve_commit_diff;
	int consumed_old, consumed_new;
	int commit_count, reserve_count;
	int end_switch_old;

	do {
		offset_old = atomic_read(&ltt_buf->offset);
		offset_begin = offset_old;
		end_switch_old = 0;
		tsc = ltt_get_timestamp();
		if(tsc == 0) {
			/* Error in getting the timestamp : should not happen : it would
			 * mean we are called from an NMI during a write seqlock on xtime. */
			return;
		}

		if(SUBBUF_OFFSET(offset_begin, ltt_buf) != 0) {
			offset_begin = SUBBUF_ALIGN(offset_begin, ltt_buf);
			end_switch_old = 1;
		} else {
      /* we do not have to switch : buffer is empty */
      return;
    }
		if(mode == FORCE_ACTIVE)
			offset_begin += ltt_subbuf_header_len(ltt_buf);
		/* Always begin_switch in FORCE_ACTIVE mode */

		/* Test new buffer integrity */
		reserve_commit_diff = 
			atomic_read(
				&ltt_buf->reserve_count[SUBBUF_INDEX(offset_begin, ltt_buf)])
			- atomic_read(
					&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin, ltt_buf)]);
		if(reserve_commit_diff == 0) {
      /* Next buffer not corrupted. */ 
      if(mode == FORCE_ACTIVE
				&& (offset_begin-atomic_read(&ltt_buf->consumed))
											>= ltt_buf->alloc_size) {
      	/* We do not overwrite non consumed buffers and we are full : ignore
        	 switch while tracing is active. */ 
        return;
      }   
    } else { 
      /* Next subbuffer corrupted. Force pushing reader even in normal mode */
    }
			
		offset_end = offset_begin;
	} while(atomic_cmpxchg(&ltt_buf->offset, offset_old, offset_end)
							!= offset_old);


	if(mode == FORCE_ACTIVE) {
		/* Push the reader if necessary */
		do {
			consumed_old = atomic_read(&ltt_buf->consumed);
			/* If buffer is in overwrite mode, push the reader consumed count if
				 the write position has reached it and we are not at the first
				 iteration (don't push the reader farther than the writer). 
				 This operation can be done concurrently by many writers in the
				 same buffer, the writer being at the fartest write position sub-buffer
				 index in the buffer being the one which will win this loop. */
			/* If the buffer is not in overwrite mode, pushing the reader only
				 happen if a sub-buffer is corrupted */
			if((SUBBUF_TRUNC(offset_end, ltt_buf) 
					- SUBBUF_TRUNC(consumed_old, ltt_buf)) 
							>= ltt_buf->alloc_size)
				consumed_new = SUBBUF_ALIGN(consumed_old, ltt_buf);
			else {
				consumed_new = consumed_old;
				break;
			}
		} while(atomic_cmpxchg(&ltt_buf->consumed, consumed_old, consumed_new)
				!= consumed_old);

		if(consumed_old != consumed_new) {
			/* Reader pushed : we are the winner of the push, we can therefore
				 reequilibrate reserve and commit. Atomic increment of the commit
				 count permits other writers to play around with this variable
				 before us. We keep track of corrupted_subbuffers even in overwrite
				 mode :
				 we never want to write over a non completely committed sub-buffer : 
				 possible causes : the buffer size is too low compared to the unordered
				 data input, or there is a writer who died between the reserve and the
				 commit. */
			if(reserve_commit_diff) {
				/* We have to alter the sub-buffer commit count : a sub-buffer is
					 corrupted */
				atomic_add(reserve_commit_diff,
								&ltt_buf->commit_count[SUBBUF_INDEX(offset_begin, ltt_buf)]);
				atomic_inc(&ltt_buf->corrupted_subbuffers);
			}
		}
	}

	/* Always switch */

	if(end_switch_old) {
		/* old subbuffer */
		/* Concurrency safe because we are the last and only thread to alter this
			 sub-buffer. As long as it is not delivered and read, no other thread can
			 alter the offset, alter the reserve_count or call the
			 client_buffer_end_callback on this sub-buffer.
			 The only remaining threads could be the ones with pending commits. They
			 will have to do the deliver themself.
			 Not concurrency safe in overwrite mode. We detect corrupted subbuffers with
			 commit and reserve counts. We keep a corrupted sub-buffers count and push
			 the readers across these sub-buffers.
			 Not concurrency safe if a writer is stalled in a subbuffer and
			 another writer switches in, finding out it's corrupted. The result will be
			 than the old (uncommited) subbuffer will be declared corrupted, and that
			 the new subbuffer will be declared corrupted too because of the commit
			 count adjustment.
			 Offset old should never be 0. */
		ltt_buffer_end_callback(ltt_buf, tsc, offset_old,
				SUBBUF_INDEX((offset_old), ltt_buf));
		/* Setting this reserve_count will allow the sub-buffer to be delivered by
			 the last committer. */
		reserve_count = atomic_add_return((SUBBUF_OFFSET((offset_old-1),
                                                      ltt_buf) + 1),
										&ltt_buf->reserve_count[SUBBUF_INDEX((offset_old),
                                                          ltt_buf)]);
		if(reserve_count == atomic_read(
				&ltt_buf->commit_count[SUBBUF_INDEX((offset_old), ltt_buf)])) {
			ltt_deliver_callback(ltt_buf, SUBBUF_INDEX((offset_old), ltt_buf), NULL);
		}
	}
	
	if(mode == FORCE_ACTIVE) {
		/* New sub-buffer */
		/* This code can be executed unordered : writers may already have written
			 to the sub-buffer before this code gets executed, caution. */
		/* The commit makes sure that this code is executed before the deliver
			 of this sub-buffer */
		ltt_buffer_begin_callback(ltt_buf, tsc, SUBBUF_INDEX(offset_begin, ltt_buf));
		commit_count = atomic_add_return(ltt_subbuf_header_len(ltt_buf),
								 &ltt_buf->commit_count[SUBBUF_INDEX(offset_begin, ltt_buf)]);
		/* Check if the written buffer has to be delivered */
		if(commit_count	== atomic_read(
					&ltt_buf->reserve_count[SUBBUF_INDEX(offset_begin, ltt_buf)])) {
			ltt_deliver_callback(ltt_buf, SUBBUF_INDEX(offset_begin, ltt_buf), NULL);
		}
	}

}


static int open_output_files(void)
{
	int ret;
	int fd;
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
			traced_tid, traced_pid, get_cycles());
	snprintf(outfile_name, PATH_MAX-1,	"process-%s", identifier_name);

#ifndef LTT_NULL_OUTPUT_TEST
	fd = creat(outfile_name, 0644);
#else
	/* NULL test */
	ret = symlink("/dev/null", outfile_name);
	if(ret < 0) {
		perror("error in symlink");
		exit(-1);
	}
	fd = open(outfile_name, O_WRONLY);
	if(fd_process < 0) {
		perror("Error in open");
		exit(-1);
	}
#endif //LTT_NULL_OUTPUT_TEST
	return fd;
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
		if(traced_pid == 0 || parent_exited) return 0;

		ret = sem_post(&ltt_buf->writer_sem);
		if(ret < 0) {
			printf("error in sem_post");
		}
	}
}

static int read_subbuffer(struct ltt_buf *ltt_buf, int fd)
{
	unsigned int consumed_old;
	int err;
	dbg_printf("LTT read buffer\n");


	err = ltt_buffer_get(ltt_buf, &consumed_old);
	if(err != 0) {
		if(err != -EAGAIN) dbg_printf("LTT Reserving sub buffer failed\n");
		goto get_error;
	}
	if(fd_process == -1) {
		fd_process = fd = open_output_files();
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
	err = ltt_buffer_put(ltt_buf, consumed_old);

	if(err != 0) {
		if(err == -EIO) {
			dbg_printf("Reader has been pushed by the writer, last subbuffer corrupted.\n");
			/* FIXME : we may delete the last written buffer if we wish. */
		}
		goto get_error;
	}

get_error:
	return err;
}

/* This function is called by ltt_rw_init which has signals blocked */
static void ltt_usertrace_fast_daemon(struct ltt_trace_info *shared_trace_info,
		sigset_t oldset, pid_t l_traced_pid, pthread_t l_traced_tid)
{
	struct sigaction act;
	int ret;

	traced_pid = l_traced_pid;
	traced_tid = l_traced_tid;

	dbg_printf("LTT ltt_usertrace_fast_daemon : init is %d, pid is %lu, traced_pid is %lu, traced_tid is %lu\n",
			shared_trace_info->init, getpid(), traced_pid, traced_tid);

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

	alarm(3);

	while(1) {
		ret = sigsuspend(&oldset);
		if(ret != -1) {
			perror("LTT Error in sigsuspend\n");
		}
		if(traced_pid == 0) break; /* parent died */
		if(parent_exited) break;
		dbg_printf("LTT Doing a buffer switch read. pid is : %lu\n", getpid());

		do {
			ret = read_subbuffer(&shared_trace_info->channel.process, fd_process);
		} while(ret == 0);
	}
	/* The parent thread is dead and we have finished with the buffer */

	/* Buffer force switch (flush). Using FLUSH instead of ACTIVE because we know
	 * there is no writer. */
	flush_buffer(&shared_trace_info->channel.process, FORCE_FLUSH);
	do {
		ret = read_subbuffer(&shared_trace_info->channel.process, fd_process);
	} while(ret == 0);

	if(fd_process != -1)
		close(fd_process);
	
	ret = sem_destroy(&shared_trace_info->channel.process.writer_sem);
	if(ret < 0) {
		perror("error in sem_destroy");
	}
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
	pid_t l_traced_tid = gettid();

	/* parent : create the shared memory map */
	shared_trace_info = mmap(0, sizeof(*thread_trace_info),
			PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	shared_trace_info->init=0;
	shared_trace_info->filter=0;
	shared_trace_info->daemon_id=0;
	shared_trace_info->nesting=0;
	memset(&shared_trace_info->channel.process, 0,
			sizeof(shared_trace_info->channel.process));
	//Need NPTL!
	ret = sem_init(&shared_trace_info->channel.process.writer_sem, 1,
									LTT_N_SUBBUFS);
	if(ret < 0) {
		perror("error in sem_init");
	}
	shared_trace_info->channel.process.alloc_size = LTT_BUF_SIZE_PROCESS;
	shared_trace_info->channel.process.subbuf_size = LTT_SUBBUF_SIZE_PROCESS;
	shared_trace_info->channel.process.start =
						shared_trace_info->channel.process_buf;
	ltt_buffer_begin_callback(&shared_trace_info->channel.process,
			ltt_get_timestamp(), 0);
	
	shared_trace_info->init = 1;

	/* Disable signals */
  ret = sigfillset(&set);
  if(ret) {
    dbg_printf("LTT Error in sigfillset\n");
  } 
	
  ret = pthread_sigmask(SIG_BLOCK, &set, &oldset);
  if(ret) {
    dbg_printf("LTT Error in pthread_sigmask\n");
  }

	pid = fork();
	if(pid > 0) {
		/* Parent */
		shared_trace_info->daemon_id = pid;
		thread_trace_info = shared_trace_info;

		/* Enable signals */
		ret = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
		if(ret) {
			dbg_printf("LTT Error in pthread_sigmask\n");
		}
	} else if(pid == 0) {
		pid_t sid;
		/* Child */
		role = LTT_ROLE_READER;
		sid = setsid();
		//Not a good idea to renice, unless futex wait eventually implement
		//priority inheritence.
		//ret = nice(1);
		//if(ret < 0) {
		//	perror("Error in nice");
		//}
		if(sid < 0) {
			perror("Error setting sid");
		}
		ltt_usertrace_fast_daemon(shared_trace_info, oldset, l_traced_pid,
					l_traced_tid);
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
  dbg_printf("LTT usertrace-fast init\n");

	ltt_rw_init();
}

void __attribute__((destructor)) __ltt_usertrace_fast_fini(void)
{
	if(role == LTT_ROLE_WRITER) {
	  dbg_printf("LTT usertrace-fast fini\n");
		ltt_usertrace_fast_cleanup(NULL);
	}
}

