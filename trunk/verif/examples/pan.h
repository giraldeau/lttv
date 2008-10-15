#define SpinVersion	"Spin Version 5.1.6 -- 9 May 2008"
#define PanSource	"buffer.spin"

#ifdef WIN64
#define ONE_L	((unsigned long) 1)
#define long	long long
#else
#define ONE_L	(1L)
#endif
char *TrailFile = PanSource; /* default */
char *trailfilename;
#if defined(BFS)
#ifndef SAFETY
#define SAFETY
#endif
#ifndef XUSAFE
#define XUSAFE
#endif
#endif
#ifndef uchar
#define uchar	unsigned char
#endif
#ifndef uint
#define uint	unsigned int
#endif
#define DELTA	500
#ifdef MA
	#if NCORE>1 && !defined(SEP_STATE)
	#define SEP_STATE
	#endif
#if MA==1
#undef MA
#define MA	100
#endif
#endif
#ifdef W_XPT
#if W_XPT==1
#undef W_XPT
#define W_XPT 1000000
#endif
#endif
#ifndef NFAIR
#define NFAIR	2	/* must be >= 2 */
#endif
#define HAS_CODE
#define MERGED	1
#ifdef NP	/* includes np_ demon */
#define HAS_NP	2
#define VERI	5
#define endclaim	3 /* none */
#endif
typedef struct S_F_MAP {
	char *fnm; int from; int upto;
} S_F_MAP;

#define nstates4	59	/* :init: */
#define endstate4	58
short src_ln4 [] = {
	  0, 225, 227, 228, 229, 230, 231, 231, 
	226, 233, 226, 233, 235, 236, 237, 238, 
	238, 234, 240, 234, 240, 241, 242, 244, 
	245, 246, 247, 248, 248, 243, 250, 243, 
	250, 252, 253, 254, 255, 256, 256, 251, 
	258, 251, 224, 262, 263, 264, 266, 267, 
	271, 272, 273, 273, 265, 278, 265, 278, 
	282, 260, 284,   0, };
S_F_MAP src_file4 [] = {
	{ "-", 0, 0 },
	{ "buffer.spin", 1, 58 },
	{ "-", 59, 60 }
};
uchar reached4 [] = {
	  0,   1,   1,   0,   0,   0,   1,   1, 
	  0,   1,   1,   0,   1,   0,   0,   1, 
	  1,   0,   1,   1,   0,   0,   0,   1, 
	  0,   0,   0,   1,   1,   0,   1,   1, 
	  0,   1,   0,   0,   0,   1,   1,   0, 
	  1,   1,   0,   1,   0,   0,   1,   0, 
	  0,   0,   1,   1,   0,   1,   1,   0, 
	  0,   0,   0,   0, };
uchar *loopstate4;

#define nstates3	10	/* cleaner */
#define endstate3	9
short src_ln3 [] = {
	  0, 210, 211, 212, 213, 209, 215, 209, 
	208, 216,   0, };
S_F_MAP src_file3 [] = {
	{ "-", 0, 0 },
	{ "buffer.spin", 1, 9 },
	{ "-", 10, 11 }
};
uchar reached3 [] = {
	  0,   1,   0,   0,   1,   0,   1,   1, 
	  0,   0,   0, };
uchar *loopstate3;

#define nstates2	32	/* reader */
#define endstate2	31
short src_ln2 [] = {
	  0, 169, 171, 173, 174, 175, 176, 177, 
	177, 172, 179, 172, 170, 187, 189, 190, 
	191, 192, 192, 188, 194, 188, 194, 196, 
	197, 186, 199, 199, 164, 201, 164, 201, 
	  0, };
S_F_MAP src_file2 [] = {
	{ "-", 0, 0 },
	{ "buffer.spin", 1, 31 },
	{ "-", 32, 33 }
};
uchar reached2 [] = {
	  0,   1,   1,   1,   0,   0,   0,   1, 
	  1,   0,   1,   1,   0,   1,   1,   0, 
	  0,   1,   1,   0,   1,   1,   0,   0, 
	  0,   0,   1,   1,   0,   1,   1,   0, 
	  0, };
uchar *loopstate2;

#define nstates1	51	/* tracer */
#define endstate1	50
short src_ln1 [] = {
	  0,  99, 100,  98, 104, 105, 106, 106, 
	103, 108, 102, 111, 111, 112, 112, 110, 
	114, 114, 116, 117, 118, 119, 120, 120, 
	115, 122, 115, 109, 127, 129, 130, 131, 
	132, 132, 128, 134, 128, 134, 135, 137, 
	137, 138, 138, 136, 140, 126, 142, 144, 
	146, 141, 148,   0, };
S_F_MAP src_file1 [] = {
	{ "-", 0, 0 },
	{ "buffer.spin", 1, 50 },
	{ "-", 51, 52 }
};
uchar reached1 [] = {
	  0,   1,   0,   0,   1,   1,   1,   0, 
	  1,   1,   0,   1,   1,   1,   0,   1, 
	  1,   0,   1,   0,   0,   0,   1,   1, 
	  0,   1,   1,   0,   1,   1,   0,   0, 
	  1,   1,   0,   1,   1,   0,   0,   1, 
	  0,   1,   0,   0,   1,   0,   1,   0, 
	  0,   0,   0,   0, };
uchar *loopstate1;

#define nstates0	31	/* switcher */
#define endstate0	30
short src_ln0 [] = {
	  0,  56,  57,  58,  61,  62,  63,  64, 
	 64,  59,  66,  55,  69,  69,  70,  70, 
	 68,  72,  67,  75,  76,  78,  78,  79, 
	 79,  77,  81,  81,  74,  84,  85,   0, };
S_F_MAP src_file0 [] = {
	{ "-", 0, 0 },
	{ "buffer.spin", 1, 30 },
	{ "-", 31, 32 }
};
uchar reached0 [] = {
	  0,   1,   0,   0,   1,   0,   1,   1, 
	  0,   0,   1,   0,   1,   1,   1,   0, 
	  1,   1,   0,   1,   0,   1,   0,   1, 
	  0,   0,   1,   0,   0,   1,   0,   0, };
uchar *loopstate0;
struct {
	int tp; short *src;
} src_all[] = {
	{ 4, &src_ln4[0] },
	{ 3, &src_ln3[0] },
	{ 2, &src_ln2[0] },
	{ 1, &src_ln1[0] },
	{ 0, &src_ln0[0] },
	{ 0, (short *) 0 }
};
short *frm_st0;
struct {
	char *c; char *t;
} code_lookup[] = {
	{ (char *) 0, "" }
};
#define _T5	64
#define _T2	65
#define T_ID	unsigned char
#define SYNC	0
#define ASYNC	0

#ifndef NCORE
	#ifdef DUAL_CORE
		#define NCORE	2
	#elif QUAD_CORE
		#define NCORE	4
	#else
		#define NCORE	1
	#endif
#endif
char *procname[] = {
   "switcher",
   "tracer",
   "reader",
   "cleaner",
   ":init:",
   ":np_:",
};

#define Pinit	((P4 *)this)
typedef struct P4 { /* :init: */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
	uchar i;
	uchar j;
	uchar sum;
	uchar commit_sum;
} P4;
#define Air4	(sizeof(P4) - Offsetof(P4, commit_sum) - 1*sizeof(uchar))
#define Pcleaner	((P3 *)this)
typedef struct P3 { /* cleaner */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
} P3;
#define Air3	(sizeof(P3) - 3)
#define Preader	((P2 *)this)
typedef struct P2 { /* reader */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
	uchar i;
	uchar j;
	uchar tmp_retrieve;
	uchar lwrite_off;
	uchar lcommit_count;
} P2;
#define Air2	(sizeof(P2) - Offsetof(P2, lcommit_count) - 1*sizeof(uchar))
#define Ptracer	((P1 *)this)
typedef struct P1 { /* tracer */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
	uchar size;
	uchar prev_off;
	uchar new_off;
	uchar tmp_commit;
	uchar i;
	uchar j;
} P1;
#define Air1	(sizeof(P1) - Offsetof(P1, j) - 1*sizeof(uchar))
#define Pswitcher	((P0 *)this)
typedef struct P0 { /* switcher */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
	uchar prev_off;
	uchar new_off;
	uchar tmp_commit;
	uchar size;
} P0;
#define Air0	(sizeof(P0) - Offsetof(P0, size) - 1*sizeof(uchar))
typedef struct P5 { /* np_ */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
} P5;
#define Air5	(sizeof(P5) - 3)
#if defined(BFS) && defined(REACH)
#undef REACH
#endif
#ifdef VERI
#define BASE	1
#else
#define BASE	0
#endif
typedef struct Trans {
	short atom;	/* if &2 = atomic trans; if &8 local */
#ifdef HAS_UNLESS
	short escp[HAS_UNLESS];	/* lists the escape states */
	short e_trans;	/* if set, this is an escp-trans */
#endif
	short tpe[2];	/* class of operation (for reduction) */
	short qu[6];	/* for conditional selections: qid's  */
	uchar ty[6];	/* ditto: type's */
#ifdef NIBIS
	short om;	/* completion status of preselects */
#endif
	char *tp;	/* src txt of statement */
	int st;		/* the nextstate */
	int t_id;	/* transition id, unique within proc */
	int forw;	/* index forward transition */
	int back;	/* index return  transition */
	struct Trans *nxt;
} Trans;

#define qptr(x)	(((uchar *)&now)+(int)q_offset[x])
#define pptr(x)	(((uchar *)&now)+(int)proc_offset[x])
extern uchar *Pptr(int);
#define q_sz(x)	(((Q0 *)qptr(x))->Qlen)

#ifndef VECTORSZ
#define VECTORSZ	1024           /* sv   size in bytes */
#endif

#ifdef VERBOSE
#ifndef CHECK
#define CHECK
#endif
#ifndef DEBUG
#define DEBUG
#endif
#endif
#ifdef SAFETY
#ifndef NOFAIR
#define NOFAIR
#endif
#endif
#ifdef NOREDUCE
#ifndef XUSAFE
#define XUSAFE
#endif
#if !defined(SAFETY) && !defined(MA)
#define FULLSTACK
#endif
#else
#ifdef BITSTATE
#if defined(SAFETY) && !defined(HASH64)
#define CNTRSTACK
#else
#define FULLSTACK
#endif
#else
#define FULLSTACK
#endif
#endif
#ifdef BITSTATE
#ifndef NOCOMP
#define NOCOMP
#endif
#if !defined(LC) && defined(SC)
#define LC
#endif
#endif
#if defined(COLLAPSE2) || defined(COLLAPSE3) || defined(COLLAPSE4)
/* accept the above for backward compatibility */
#define COLLAPSE
#endif
#ifdef HC
#undef HC
#define HC4
#endif
#ifdef HC0
#define HC	0
#endif
#ifdef HC1
#define HC	1
#endif
#ifdef HC2
#define HC	2
#endif
#ifdef HC3
#define HC	3
#endif
#ifdef HC4
#define HC	4
#endif
#ifdef COLLAPSE
#if NCORE>1 && !defined(SEP_STATE)
unsigned long *ncomps;	/* in shared memory */
#else
unsigned long ncomps[256+2];
#endif
#endif
#define MAXQ   	255
#define MAXPROC	255
#define WS		sizeof(void *) /* word size in bytes */
typedef struct Stack  {	 /* for queues and processes */
#if VECTORSZ>32000
	int o_delta;
	int o_offset;
	int o_skip;
	int o_delqs;
#else
	short o_delta;
	short o_offset;
	short o_skip;
	short o_delqs;
#endif
	short o_boq;
#ifndef XUSAFE
	char *o_name;
#endif
	char *body;
	struct Stack *nxt;
	struct Stack *lst;
} Stack;

typedef struct Svtack { /* for complete state vector */
#if VECTORSZ>32000
	int o_delta;
	int m_delta;
#else
	short o_delta;	 /* current size of frame */
	short m_delta;	 /* maximum size of frame */
#endif
#if SYNC
	short o_boq;
#endif
#define StackSize	(WS)
	char *body;
	struct Svtack *nxt;
	struct Svtack *lst;
} Svtack;

Trans ***trans;	/* 1 ptr per state per proctype */

struct H_el *Lstate;
int depthfound = -1;	/* loop detection */
#if VECTORSZ>32000
int proc_offset[MAXPROC];
int q_offset[MAXQ];
#else
short proc_offset[MAXPROC];
short q_offset[MAXQ];
#endif
uchar proc_skip[MAXPROC];
uchar q_skip[MAXQ];
unsigned long  vsize;	/* vector size in bytes */
#ifdef SVDUMP
int vprefix=0, svfd;		/* runtime option -pN */
#endif
char *tprefix = "trail";	/* runtime option -tsuffix */
short boq = -1;		/* blocked_on_queue status */
typedef struct State {
	uchar _nr_pr;
	uchar _nr_qs;
	uchar   _a_t;	/* cycle detection */
#ifndef NOFAIR
	uchar   _cnt[NFAIR];	/* counters, weak fairness */
#endif
#ifndef NOVSZ
#if VECTORSZ<65536
	unsigned short _vsz;
#else
	unsigned long  _vsz;
#endif
#endif
#ifdef HAS_LAST
	uchar  _last;	/* pid executed in last step */
#endif
#ifdef EVENT_TRACE
#if nstates_event<256
	uchar _event;
#else
	unsigned short _event;
#endif
#endif
	uchar buffer_use[4];
	uchar write_off;
	uchar commit_count[2];
	uchar read_off;
	uchar retrieve_count[2];
	uchar events_lost;
	uchar refcount;
	uchar sv[VECTORSZ];
} State;

#define HAS_TRACK	0
/* hidden variable: */	uchar deliver;
int _; /* a predefined write-only variable */

#define FORWARD_MOVES	"pan.m"
#define REVERSE_MOVES	"pan.b"
#define TRANSITIONS	"pan.t"
#define _NP_	5
uchar reached5[3];  /* np_ */
uchar *loopstate5;  /* np_ */
#define nstates5	3 /* np_ */
#define endstate5	2 /* np_ */

#define start5	0 /* np_ */
#define start4	42
#define start3	8
#define start2	28
#define start1	3
#define start0	11
#ifdef NP
	#define ACCEPT_LAB	1 /* at least 1 in np_ */
#else
	#define ACCEPT_LAB	0 /* user-defined accept labels */
#endif
#ifdef MEMCNT
	#ifdef MEMLIM
		#warning -DMEMLIM takes precedence over -DMEMCNT
		#undef MEMCNT
	#else
		#if MEMCNT<20
			#warning using minimal value -DMEMCNT=20 (=1MB)
			#define MEMLIM	(1)
			#undef MEMCNT
		#else
			#if MEMCNT==20
				#define MEMLIM	(1)
				#undef MEMCNT
			#else
			 #if MEMCNT>=50
			  #error excessive value for MEMCNT
			 #else
				#define MEMLIM	(1<<(MEMCNT-20))
			 #endif
			#endif
		#endif
	#endif
#endif
#if NCORE>1 && !defined(MEMLIM)
	#define MEMLIM	(2048)	/* need a default, using 2 GB */
#endif
#define PROG_LAB	0 /* progress labels */
uchar *accpstate[6];
uchar *progstate[6];
uchar *loopstate[6];
uchar *reached[6];
uchar *stopstate[6];
uchar *visstate[6];
short *mapstate[6];
#ifdef HAS_CODE
int NrStates[6];
#endif
#define NQS	0
short q_flds[1];
short q_max[1];
typedef struct Q0 {	/* generic q */
	uchar Qlen;	/* q_size */
	uchar _t;
} Q0;

/** function prototypes **/
char *emalloc(unsigned long);
char *Malloc(unsigned long);
int Boundcheck(int, int, int, int, Trans *);
int addqueue(int, int);
/* int atoi(char *); */
/* int abort(void); */
int close(int);
int delproc(int, int);
int endstate(void);
int hstore(char *, int);
#ifdef MA
int gstore(char *, int, uchar);
#endif
int q_cond(short, Trans *);
int q_full(int);
int q_len(int);
int q_zero(int);
int qrecv(int, int, int, int);
int unsend(int);
/* void *sbrk(int); */
void Uerror(char *);
void assert(int, char *, int, int, Trans *);
void c_chandump(int);
void c_globals(void);
void c_locals(int, int);
void checkcycles(void);
void crack(int, int, Trans *, short *);
void d_sfh(const char *, int);
void sfh(const char *, int);
void d_hash(uchar *, int);
void s_hash(uchar *, int);
void r_hash(uchar *, int);
void delq(int);
void do_reach(void);
void pan_exit(int);
void exit(int);
void hinit(void);
void imed(Trans *, int, int, int);
void new_state(void);
void p_restor(int);
void putpeg(int, int);
void putrail(void);
void q_restor(void);
void retrans(int, int, int, short *, uchar *, uchar *);
void settable(void);
void setq_claim(int, int, char *, int, char *);
void sv_restor(void);
void sv_save(void);
void tagtable(int, int, int, short *, uchar *);
void do_dfs(int, int, int, short *, uchar *, uchar *);
void uerror(char *);
void unrecv(int, int, int, int, int);
void usage(FILE *);
void wrap_stats(void);
#if defined(FULLSTACK) && defined(BITSTATE)
int  onstack_now(void);
void onstack_init(void);
void onstack_put(void);
void onstack_zap(void);
#endif
#ifndef XUSAFE
int q_S_check(int, int);
int q_R_check(int, int);
uchar q_claim[MAXQ+1];
char *q_name[MAXQ+1];
char *p_name[MAXPROC+1];
#endif
void qsend(int, int, int);
#define Addproc(x)	addproc(x)
#define LOCAL	1
#define Q_FULL_F	2
#define Q_EMPT_F	3
#define Q_EMPT_T	4
#define Q_FULL_T	5
#define TIMEOUT_F	6
#define GLOBAL	7
#define BAD	8
#define ALPHA_F	9
#define NTRANS	66
#ifdef PEG
long peg[NTRANS];
#endif
