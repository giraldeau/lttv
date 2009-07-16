#define BUFSIZE 8
/* LTTng ltt-tracer.c atomic lockless buffering scheme Promela model v2
 * Created for the Spin validator.
 * Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 * October 2008
 
 * TODO : create test cases that will generate an overflow on the offset and
 * counter type. Counter types smaller than a byte should be used.
 
 * Promela only has unsigned char, no signed char.
 * Because detection of difference < 0 depends on a signed type, but we want
 * compactness, check also for the values being higher than half of the unsigned
 * char range (and consider them negative). The model, by design, does not use
 * offsets or counts higher than 127 because we would then have to use a larger
 * type (short or int).
 */
#define HALF_UCHAR (255/2)

/* NUMPROCS 4 : causes event loss with some reader timings.
 * e.g. 3 events, 1 switch, 1 event (lost, buffer full), read 1 subbuffer
 */
#define NUMPROCS 4

/* NUMPROCS 3 : does not cause event loss because buffers are big enough.
 * #define NUMPROCS 3
 * e.g. 3 events, 1 switch, read 1 subbuffer
 */

#define NUMSWITCH 1
#ifndef BUFSIZE
#define BUFSIZE 4
#endif
#define NR_SUBBUFS 2
#define SUBBUF_SIZE (BUFSIZE / NR_SUBBUFS)

/* Writer counters
*/
byte write_off = 0;
byte commit_count[NR_SUBBUFS];

/* <formal_verif> */
byte _commit_sum;
/* </formal_verif> */

/* Reader counters
*/
byte read_off = 0;

byte events_lost = 0;
byte refcount = 0;

bool deliver = 0;

//#ifdef RACE_TEST
/* buffer slot in-use bit. Detects racy use (more than a single process
 * accessing a slot at any given step).
 */
bool buffer_use[BUFSIZE];
//#endif

/* Proceed to a sub-subber switch is needed.
 * Used in a periodical timer interrupt to fill and ship the current subbuffer
 * to the reader so we can guarantee a steady flow. If a subbuffer is
 * completely empty, do not switch.
 * Also used as "finalize" operation to complete the last subbuffer after
 * all writers have finished so the last subbuffer can be read by the reader.
 */
proctype switcher()
{
  byte prev_off, new_off, tmp_commit;
  byte size;
  
cmpxchg_loop:
  atomic {
    prev_off = write_off;
    size = SUBBUF_SIZE - (prev_off % SUBBUF_SIZE);
    new_off = prev_off + size;
    if
      :: (new_off - read_off > BUFSIZE && new_off - read_off < HALF_UCHAR)
         || size == SUBBUF_SIZE ->
         refcount = refcount - 1;
         goto not_needed;
      :: else -> skip
    fi;
  }
  atomic {
    if
      :: prev_off != write_off -> goto cmpxchg_loop
      :: else -> write_off = new_off;
    fi;
  }

  atomic {
    tmp_commit = commit_count[(prev_off % BUFSIZE) / SUBBUF_SIZE] + size;
    /* <formal_verif> */
    _commit_sum = _commit_sum - commit_count[(prev_off % BUFSIZE) / SUBBUF_SIZE]
                  + tmp_commit;
    /* </formal_verif> */
    commit_count[(prev_off % BUFSIZE) / SUBBUF_SIZE] = tmp_commit;
    if
      :: (((prev_off / BUFSIZE) * BUFSIZE) / NR_SUBBUFS) + SUBBUF_SIZE -
         tmp_commit
         -> deliver = 1
      :: else
         -> skip
    fi;
    refcount = refcount - 1;
  }
not_needed:
  skip;
}

/* tracer
 * Writes 1 byte of information in the buffer at the current
 * "write_off" position and then increment the commit_count of the sub-buffer
 * the information has been written to.
 */
proctype tracer()
{
  byte size = 1;
  byte prev_off, new_off, tmp_commit;
  byte i, j;

cmpxchg_loop:
  atomic {
    prev_off = write_off;
    new_off = prev_off + size;
  }
  atomic {
    if
      :: new_off - read_off > BUFSIZE && new_off - read_off < HALF_UCHAR ->
         goto lost
      :: else -> skip
    fi;
  }
  atomic {
    if
      :: prev_off != write_off -> goto cmpxchg_loop
      :: else -> write_off = new_off;
    fi;
    i = 0;
    do
      :: i < size ->
         assert(buffer_use[(prev_off + i) % BUFSIZE] == 0);
         buffer_use[(prev_off + i) % BUFSIZE] = 1;
         i++
      :: i >= size -> break
    od;
  }

  /* writing to buffer...
  */

  atomic {
    i = 0;
    do
      :: i < size ->
         buffer_use[(prev_off + i) % BUFSIZE] = 0;
         i++
      :: i >= size -> break
    od;
    tmp_commit = commit_count[(prev_off % BUFSIZE) / SUBBUF_SIZE] + size;
    /* <formal_verif> */
    _commit_sum = _commit_sum - commit_count[(prev_off % BUFSIZE) / SUBBUF_SIZE]
                  + tmp_commit;
    /* </formal_verif> */
    commit_count[(prev_off % BUFSIZE) / SUBBUF_SIZE] = tmp_commit;
    if
      :: (((prev_off / BUFSIZE) * BUFSIZE) / NR_SUBBUFS) + SUBBUF_SIZE -
         tmp_commit
         -> deliver = 1
      :: else
         -> skip
    fi;
  }
  atomic {
    goto end;
lost:
    events_lost++;
end:
    refcount = refcount - 1;
  }
}

/* reader
 * Read the information sub-buffer per sub-buffer when available.
 *
 * Reads the information as soon as it is ready, or may be delayed by
 * an asynchronous delivery. Being modeled as a process insures all cases
 * (scheduled very quickly or very late, causing event loss) are covered.
 * Only one reader per buffer (normally ensured by a mutex). This is modeled
 * by using a single reader process.
 */
proctype reader()
{
  byte i, j;

  do
    :: (write_off / SUBBUF_SIZE) - (read_off / SUBBUF_SIZE) > 0
       && (write_off / SUBBUF_SIZE) - (read_off / SUBBUF_SIZE) < HALF_UCHAR
       && (commit_count[(read_off % BUFSIZE) / SUBBUF_SIZE]
           - SUBBUF_SIZE - (((read_off / BUFSIZE) * BUFSIZE) / NR_SUBBUFS)
           == 0) ->
       atomic {
         i = 0;
         do
           :: i < SUBBUF_SIZE ->
              assert(buffer_use[(read_off + i) % BUFSIZE] == 0);
              buffer_use[(read_off + i) % BUFSIZE] = 1;
              i++
           :: i >= SUBBUF_SIZE -> break
         od;
       }

       /* reading from buffer...
       */

       atomic {
         i = 0;
         do
           :: i < SUBBUF_SIZE ->
              buffer_use[(read_off + i) % BUFSIZE] = 0;
              i++
           :: i >= SUBBUF_SIZE -> break
         od;
         read_off = read_off + SUBBUF_SIZE;
       }
    :: read_off >= (NUMPROCS - events_lost) -> break;
  od;
}

/* Waits for all tracer and switcher processes to finish before finalizing
 * the buffer. Only after that will the reader be allowed to read the
 * last subbuffer.
 */
proctype cleaner()
{
  atomic {
    do
      :: refcount == 0 ->
         refcount = refcount + 1;
         run switcher();  /* Finalize the last sub-buffer so it can be read. */
         break;
    od;
  }
}

init {
  byte i = 0;
  byte j = 0;
  byte sum = 0;
  byte commit_sum = 0;

  atomic {
    i = 0;
    do
      :: i < NR_SUBBUFS ->
         commit_count[i] = 0;
         i++
      :: i >= NR_SUBBUFS -> break
    od;
    /* <formal_verif> */
    _commit_sum = 0;
    /* </formal_verif> */
    i = 0;
    do
      :: i < BUFSIZE ->
         buffer_use[i] = 0;
         i++
      :: i >= BUFSIZE -> break
    od;
    run reader();
    run cleaner();
    i = 0;
    do
      :: i < NUMPROCS ->
         refcount = refcount + 1;
         run tracer();
         i++
      :: i >= NUMPROCS -> break
    od;
    i = 0;
    do
      :: i < NUMSWITCH ->
         refcount = refcount + 1;
         run switcher();
         i++
      :: i >= NUMSWITCH -> break
    od;
  }
}

