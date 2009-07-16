#define SpinVersion	"Spin Version 5.1.6 -- 9 May 2008"
#define PanSource	"pan.spin"

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
#define VERI	6
#define endclaim	3 /* none */
#endif
#if !defined(NOCLAIM) && !defined NP
#define VERI	5
#define endclaim	endstate5
#endif
typedef struct S_F_MAP {
	char *fnm; int from; int upto;
} S_F_MAP;

#define nstates5	9	/* :never: */
#define nstates_claim	nstates5
#define endstate5	8
short src_ln5 [] = {
	  0, 304, 304, 305, 305, 303, 307, 308, 
	309,   0, };
S_F_MAP src_file5 [] = {
	{ "-", 0, 0 },
	{ "pan.___", 1, 8 },
	{ "-", 9, 10 }
};
#define src_claim	src_ln5
uchar reached5 [] = {
	  0,   1,   1,   1,   1,   0,   1,   1, 
	  0,   0, };
uchar *loopstate5;
#define reached_claim	reached5

#define nstates4	44	/* :init: */
#define endstate4	43
short src_ln4 [] = {
	  0, 255, 257, 258, 259, 260, 260, 256, 
	263, 256, 263, 265, 267, 268, 269, 270, 
	270, 266, 272, 266, 272, 273, 274, 276, 
	277, 278, 279, 280, 280, 275, 282, 275, 
	282, 284, 285, 286, 287, 288, 288, 283, 
	290, 283, 254, 291,   0, };
S_F_MAP src_file4 [] = {
	{ "-", 0, 0 },
	{ "pan.___", 1, 43 },
	{ "-", 44, 45 }
};
uchar reached4 [] = {
	  0,   1,   1,   0,   0,   1,   1,   0, 
	  1,   1,   0,   0,   1,   0,   0,   1, 
	  1,   0,   1,   1,   0,   0,   0,   1, 
	  0,   0,   0,   1,   1,   0,   1,   1, 
	  0,   1,   0,   0,   0,   1,   1,   0, 
	  1,   1,   0,   0,   0, };
uchar *loopstate4;

#define nstates3	10	/* cleaner */
#define endstate3	9
short src_ln3 [] = {
	  0, 240, 241, 242, 243, 239, 245, 239, 
	238, 246,   0, };
S_F_MAP src_file3 [] = {
	{ "-", 0, 0 },
	{ "pan.___", 1, 9 },
	{ "-", 10, 11 }
};
uchar reached3 [] = {
	  0,   1,   0,   0,   1,   0,   1,   1, 
	  0,   0,   0, };
uchar *loopstate3;

#define nstates2	30	/* reader */
#define endstate2	29
short src_ln2 [] = {
	  0, 203, 205, 207, 208, 209, 210, 211, 
	211, 206, 213, 206, 204, 219, 221, 222, 
	223, 224, 224, 220, 226, 220, 226, 218, 
	228, 228, 198, 230, 198, 230,   0, };
S_F_MAP src_file2 [] = {
	{ "-", 0, 0 },
	{ "pan.___", 1, 29 },
	{ "-", 30, 31 }
};
uchar reached2 [] = {
	  0,   1,   1,   1,   0,   0,   0,   1, 
	  1,   0,   1,   1,   0,   1,   1,   0, 
	  0,   1,   1,   0,   1,   1,   0,   0, 
	  1,   1,   0,   1,   1,   0,   0, };
uchar *loopstate2;

#define nstates1	52	/* tracer */
#define endstate1	51
short src_ln1 [] = {
	  0, 126, 127, 125, 131, 132, 133, 133, 
	130, 135, 129, 138, 138, 139, 139, 137, 
	141, 141, 143, 144, 145, 146, 147, 147, 
	142, 149, 142, 136, 155, 157, 158, 159, 
	160, 160, 156, 162, 156, 162, 164, 167, 
	170, 171, 172, 173, 168, 175, 154, 177, 
	179, 181, 176, 183,   0, };
S_F_MAP src_file1 [] = {
	{ "-", 0, 0 },
	{ "pan.___", 1, 51 },
	{ "-", 52, 53 }
};
uchar reached1 [] = {
	  0,   1,   0,   0,   1,   1,   1,   0, 
	  1,   1,   0,   1,   1,   1,   0,   1, 
	  1,   0,   1,   0,   0,   0,   1,   1, 
	  0,   1,   1,   0,   1,   1,   0,   0, 
	  1,   1,   0,   1,   1,   0,   0,   0, 
	  1,   0,   1,   0,   0,   1,   0,   1, 
	  0,   0,   0,   0,   0, };
uchar *loopstate1;

#define nstates0	32	/* switcher */
#define endstate0	31
short src_ln0 [] = {
	  0,  75,  76,  77,  80,  81,  82,  83, 
	 83,  78,  85,  74,  88,  88,  89,  89, 
	 87,  91,  86,  94,  96,  99, 102, 103, 
	104, 105, 100, 107, 107,  93, 110, 111, 
	  0, };
S_F_MAP src_file0 [] = {
	{ "-", 0, 0 },
	{ "pan.___", 1, 31 },
	{ "-", 32, 33 }
};
uchar reached0 [] = {
	  0,   1,   0,   0,   1,   0,   1,   1, 
	  0,   0,   1,   0,   1,   1,   1,   0, 
	  1,   1,   0,   1,   0,   0,   1,   0, 
	  1,   0,   0,   1,   0,   0,   1,   0, 
	  0, };
uchar *loopstate0;
struct {
	int tp; short *src;
} src_all[] = {
	{ 5, &src_ln5[0] },
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
#define _T5	62
#define _T2	63
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
   ":never:",
   ":np_:",
};

typedef struct P5 { /* :never: */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
} P5;
#define Air5	(sizeof(P5) - 3)
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
} P2;
#define Air2	(sizeof(P2) - Offsetof(P2, j) - 1*sizeof(uchar))
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
typedef struct P6 { /* np_ */
	unsigned _pid : 8;  /* 0..255 */
	unsigned _t   : 4; /* proctype */
	unsigned _p   : 7; /* state    */
} P6;
#define Air6	(sizeof(P6) - 3)
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
	uchar buffer_use[8];
	uchar write_off;
	uchar commit_count[2];
	uchar _commit_sum;
	uchar read_off;
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
#define _NP_	6
uchar reached6[3];  /* np_ */
uchar *loopstate6;  /* np_ */
#define nstates6	3 /* np_ */
#define endstate6	2 /* np_ */

#define start6	0 /* np_ */
#define start5	5
#define start_claim	5
#define start4	42
#define start3	8
#define start2	26
#define start1	3
#define start0	11
#ifdef NP
	#define ACCEPT_LAB	1 /* at least 1 in np_ */
#else
	#define ACCEPT_LAB	1 /* user-defined accept labels */
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
uchar *accpstate[7];
uchar *progstate[7];
uchar *loopstate[7];
uchar *reached[7];
uchar *stopstate[7];
uchar *visstate[7];
short *mapstate[7];
#ifdef HAS_CODE
int NrStates[7];
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
#define NTRANS	64
#ifdef PEG
long peg[NTRANS];
#endif
