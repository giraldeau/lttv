#define rand	pan_rand
#if defined(HAS_CODE) && defined(VERBOSE)
	cpu_printf("Pr: %d Tr: %d\n", II, t->forw);
#endif
	switch (t->forw) {
	default: Uerror("bad forward move");
	case 0:	/* if without executable clauses */
		continue;
	case 1: /* generic 'goto' or 'skip' */
		IfNotBlocked
		_m = 3; goto P999;
	case 2: /* generic 'else' */
		IfNotBlocked
		if (trpt->o_pm&1) continue;
		_m = 3; goto P999;

		 /* PROC :never: */
	case 3: /* STATE 1 - line 301 "pan.___" - [((((4+1)<=4)&&(events_lost!=0)))] (0:0:0 - 1) */
		
#if defined(VERI) && !defined(NP)
		{	static int reported1 = 0;
			if (verbose && !reported1)
			{	printf("depth %d: Claim reached state %d (line %d)\n",
					depth, frm_st0[t->forw], src_claim[1]);
				reported1 = 1;
				fflush(stdout);
		}	}
#endif
		reached[5][1] = 1;
		if (!((((4+1)<=4)&&(((int)now.events_lost)!=0))))
			continue;
		_m = 3; goto P999; /* 0 */
	case 4: /* STATE 3 - line 302 "pan.___" - [(((4+1)<=4))] (0:0:0 - 1) */
		
#if defined(VERI) && !defined(NP)
		{	static int reported3 = 0;
			if (verbose && !reported3)
			{	printf("depth %d: Claim reached state %d (line %d)\n",
					depth, frm_st0[t->forw], src_claim[3]);
				reported3 = 1;
				fflush(stdout);
		}	}
#endif
		reached[5][3] = 1;
		if (!(((4+1)<=4)))
			continue;
		_m = 3; goto P999; /* 0 */
	case 5: /* STATE 7 - line 306 "pan.___" - [((events_lost!=0))] (0:0:0 - 1) */
		
#if defined(VERI) && !defined(NP)
		{	static int reported7 = 0;
			if (verbose && !reported7)
			{	printf("depth %d: Claim reached state %d (line %d)\n",
					depth, frm_st0[t->forw], src_claim[7]);
				reported7 = 1;
				fflush(stdout);
		}	}
#endif
		reached[5][7] = 1;
		if (!((((int)now.events_lost)!=0)))
			continue;
		_m = 3; goto P999; /* 0 */
	case 6: /* STATE 14 - line 311 "pan.___" - [-end-] (0:0:0 - 1) */
		
#if defined(VERI) && !defined(NP)
		{	static int reported14 = 0;
			if (verbose && !reported14)
			{	printf("depth %d: Claim reached state %d (line %d)\n",
					depth, frm_st0[t->forw], src_claim[14]);
				reported14 = 1;
				fflush(stdout);
		}	}
#endif
		reached[5][14] = 1;
		if (!delproc(1, II)) continue;
		_m = 3; goto P999; /* 0 */

		 /* PROC :init: */
	case 7: /* STATE 1 - line 252 "pan.___" - [i = 0] (0:0:1 - 1) */
		IfNotBlocked
		reached[4][1] = 1;
		(trpt+1)->bup.oval = ((int)((P4 *)this)->i);
		((P4 *)this)->i = 0;
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		_m = 3; goto P999; /* 0 */
	case 8: /* STATE 2 - line 254 "pan.___" - [((i<2))] (7:0:2 - 1) */
		IfNotBlocked
		reached[4][2] = 1;
		if (!((((int)((P4 *)this)->i)<2)))
			continue;
		/* merge: commit_count[i] = 0(7, 3, 7) */
		reached[4][3] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.commit_count[ Index(((int)((P4 *)this)->i), 2) ]);
		now.commit_count[ Index(((P4 *)this)->i, 2) ] = 0;
#ifdef VAR_RANGES
		logval("commit_count[:init::i]", ((int)now.commit_count[ Index(((int)((P4 *)this)->i), 2) ]));
#endif
		;
		/* merge: i = (i+1)(7, 4, 7) */
		reached[4][4] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P4 *)this)->i);
		((P4 *)this)->i = (((int)((P4 *)this)->i)+1);
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 8, 7) */
		reached[4][8] = 1;
		;
		_m = 3; goto P999; /* 3 */
	case 9: /* STATE 5 - line 257 "pan.___" - [((i>=2))] (17:0:3 - 1) */
		IfNotBlocked
		reached[4][5] = 1;
		if (!((((int)((P4 *)this)->i)>=2)))
			continue;
		/* dead 1: i */  (trpt+1)->bup.ovals = grab_ints(3);
		(trpt+1)->bup.ovals[0] = ((P4 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P4 *)this)->i = 0;
		/* merge: goto :b6(17, 6, 17) */
		reached[4][6] = 1;
		;
		/* merge: _commit_sum = 0(17, 10, 17) */
		reached[4][10] = 1;
		(trpt+1)->bup.ovals[1] = ((int)now._commit_sum);
		now._commit_sum = 0;
#ifdef VAR_RANGES
		logval("_commit_sum", ((int)now._commit_sum));
#endif
		;
		/* merge: i = 0(17, 11, 17) */
		reached[4][11] = 1;
		(trpt+1)->bup.ovals[2] = ((int)((P4 *)this)->i);
		((P4 *)this)->i = 0;
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 18, 17) */
		reached[4][18] = 1;
		;
		_m = 3; goto P999; /* 4 */
	case 10: /* STATE 10 - line 260 "pan.___" - [_commit_sum = 0] (0:17:2 - 3) */
		IfNotBlocked
		reached[4][10] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now._commit_sum);
		now._commit_sum = 0;
#ifdef VAR_RANGES
		logval("_commit_sum", ((int)now._commit_sum));
#endif
		;
		/* merge: i = 0(17, 11, 17) */
		reached[4][11] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P4 *)this)->i);
		((P4 *)this)->i = 0;
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 18, 17) */
		reached[4][18] = 1;
		;
		_m = 3; goto P999; /* 2 */
	case 11: /* STATE 12 - line 264 "pan.___" - [((i<4))] (17:0:2 - 1) */
		IfNotBlocked
		reached[4][12] = 1;
		if (!((((int)((P4 *)this)->i)<4)))
			continue;
		/* merge: buffer_use[i] = 0(17, 13, 17) */
		reached[4][13] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.buffer_use[ Index(((int)((P4 *)this)->i), 4) ]);
		now.buffer_use[ Index(((P4 *)this)->i, 4) ] = 0;
#ifdef VAR_RANGES
		logval("buffer_use[:init::i]", ((int)now.buffer_use[ Index(((int)((P4 *)this)->i), 4) ]));
#endif
		;
		/* merge: i = (i+1)(17, 14, 17) */
		reached[4][14] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P4 *)this)->i);
		((P4 *)this)->i = (((int)((P4 *)this)->i)+1);
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 18, 17) */
		reached[4][18] = 1;
		;
		_m = 3; goto P999; /* 3 */
	case 12: /* STATE 15 - line 267 "pan.___" - [((i>=4))] (0:0:1 - 1) */
		IfNotBlocked
		reached[4][15] = 1;
		if (!((((int)((P4 *)this)->i)>=4)))
			continue;
		/* dead 1: i */  (trpt+1)->bup.oval = ((P4 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P4 *)this)->i = 0;
		_m = 3; goto P999; /* 0 */
	case 13: /* STATE 20 - line 269 "pan.___" - [(run reader())] (0:0:0 - 3) */
		IfNotBlocked
		reached[4][20] = 1;
		if (!(addproc(2)))
			continue;
		_m = 3; goto P999; /* 0 */
	case 14: /* STATE 21 - line 270 "pan.___" - [(run cleaner())] (29:0:1 - 1) */
		IfNotBlocked
		reached[4][21] = 1;
		if (!(addproc(3)))
			continue;
		/* merge: i = 0(0, 22, 29) */
		reached[4][22] = 1;
		(trpt+1)->bup.oval = ((int)((P4 *)this)->i);
		((P4 *)this)->i = 0;
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 30, 29) */
		reached[4][30] = 1;
		;
		_m = 3; goto P999; /* 2 */
	case 15: /* STATE 23 - line 273 "pan.___" - [((i<4))] (25:0:1 - 1) */
		IfNotBlocked
		reached[4][23] = 1;
		if (!((((int)((P4 *)this)->i)<4)))
			continue;
		/* merge: refcount = (refcount+1)(0, 24, 25) */
		reached[4][24] = 1;
		(trpt+1)->bup.oval = ((int)now.refcount);
		now.refcount = (((int)now.refcount)+1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 1 */
	case 16: /* STATE 25 - line 275 "pan.___" - [(run tracer())] (29:0:1 - 1) */
		IfNotBlocked
		reached[4][25] = 1;
		if (!(addproc(1)))
			continue;
		/* merge: i = (i+1)(0, 26, 29) */
		reached[4][26] = 1;
		(trpt+1)->bup.oval = ((int)((P4 *)this)->i);
		((P4 *)this)->i = (((int)((P4 *)this)->i)+1);
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 30, 29) */
		reached[4][30] = 1;
		;
		_m = 3; goto P999; /* 2 */
	case 17: /* STATE 27 - line 277 "pan.___" - [((i>=4))] (39:0:2 - 1) */
		IfNotBlocked
		reached[4][27] = 1;
		if (!((((int)((P4 *)this)->i)>=4)))
			continue;
		/* dead 1: i */  (trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((P4 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P4 *)this)->i = 0;
		/* merge: goto :b8(39, 28, 39) */
		reached[4][28] = 1;
		;
		/* merge: i = 0(39, 32, 39) */
		reached[4][32] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P4 *)this)->i);
		((P4 *)this)->i = 0;
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 40, 39) */
		reached[4][40] = 1;
		;
		_m = 3; goto P999; /* 3 */
	case 18: /* STATE 32 - line 279 "pan.___" - [i = 0] (0:39:1 - 3) */
		IfNotBlocked
		reached[4][32] = 1;
		(trpt+1)->bup.oval = ((int)((P4 *)this)->i);
		((P4 *)this)->i = 0;
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 40, 39) */
		reached[4][40] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 19: /* STATE 33 - line 281 "pan.___" - [((i<1))] (35:0:1 - 1) */
		IfNotBlocked
		reached[4][33] = 1;
		if (!((((int)((P4 *)this)->i)<1)))
			continue;
		/* merge: refcount = (refcount+1)(0, 34, 35) */
		reached[4][34] = 1;
		(trpt+1)->bup.oval = ((int)now.refcount);
		now.refcount = (((int)now.refcount)+1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 1 */
	case 20: /* STATE 35 - line 283 "pan.___" - [(run switcher())] (39:0:1 - 1) */
		IfNotBlocked
		reached[4][35] = 1;
		if (!(addproc(0)))
			continue;
		/* merge: i = (i+1)(0, 36, 39) */
		reached[4][36] = 1;
		(trpt+1)->bup.oval = ((int)((P4 *)this)->i);
		((P4 *)this)->i = (((int)((P4 *)this)->i)+1);
#ifdef VAR_RANGES
		logval(":init::i", ((int)((P4 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 40, 39) */
		reached[4][40] = 1;
		;
		_m = 3; goto P999; /* 2 */
	case 21: /* STATE 37 - line 285 "pan.___" - [((i>=1))] (41:0:1 - 1) */
		IfNotBlocked
		reached[4][37] = 1;
		if (!((((int)((P4 *)this)->i)>=1)))
			continue;
		/* dead 1: i */  (trpt+1)->bup.oval = ((P4 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P4 *)this)->i = 0;
		/* merge: goto :b9(0, 38, 41) */
		reached[4][38] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 22: /* STATE 43 - line 288 "pan.___" - [-end-] (0:0:0 - 1) */
		IfNotBlocked
		reached[4][43] = 1;
		if (!delproc(1, II)) continue;
		_m = 3; goto P999; /* 0 */

		 /* PROC cleaner */
	case 23: /* STATE 1 - line 237 "pan.___" - [((refcount==0))] (3:0:1 - 1) */
		IfNotBlocked
		reached[3][1] = 1;
		if (!((((int)now.refcount)==0)))
			continue;
		/* merge: refcount = (refcount+1)(0, 2, 3) */
		reached[3][2] = 1;
		(trpt+1)->bup.oval = ((int)now.refcount);
		now.refcount = (((int)now.refcount)+1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 1 */
	case 24: /* STATE 3 - line 239 "pan.___" - [(run switcher())] (7:0:0 - 1) */
		IfNotBlocked
		reached[3][3] = 1;
		if (!(addproc(0)))
			continue;
		/* merge: goto :b5(0, 4, 7) */
		reached[3][4] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 25: /* STATE 9 - line 243 "pan.___" - [-end-] (0:0:0 - 1) */
		IfNotBlocked
		reached[3][9] = 1;
		if (!delproc(1, II)) continue;
		_m = 3; goto P999; /* 0 */

		 /* PROC reader */
	case 26: /* STATE 1 - line 200 "pan.___" - [((((((write_off/(4/2))-(read_off/(4/2)))>0)&&(((write_off/(4/2))-(read_off/(4/2)))<(255/2)))&&(((commit_count[((read_off%4)/(4/2))]-(4/2))-(((read_off/4)*4)/2))==0)))] (0:0:0 - 1) */
		IfNotBlocked
		reached[2][1] = 1;
		if (!((((((((int)now.write_off)/(4/2))-(((int)now.read_off)/(4/2)))>0)&&(((((int)now.write_off)/(4/2))-(((int)now.read_off)/(4/2)))<(255/2)))&&(((((int)now.commit_count[ Index(((((int)now.read_off)%4)/(4/2)), 2) ])-(4/2))-(((((int)now.read_off)/4)*4)/2))==0))))
			continue;
		_m = 3; goto P999; /* 0 */
	case 27: /* STATE 2 - line 202 "pan.___" - [i = 0] (0:0:1 - 1) */
		IfNotBlocked
		reached[2][2] = 1;
		(trpt+1)->bup.oval = ((int)((P2 *)this)->i);
		((P2 *)this)->i = 0;
#ifdef VAR_RANGES
		logval("reader:i", ((int)((P2 *)this)->i));
#endif
		;
		_m = 3; goto P999; /* 0 */
	case 28: /* STATE 3 - line 204 "pan.___" - [((i<(4/2)))] (9:0:2 - 1) */
		IfNotBlocked
		reached[2][3] = 1;
		if (!((((int)((P2 *)this)->i)<(4/2))))
			continue;
		/* merge: assert((buffer_use[((read_off+i)%4)]==0))(9, 4, 9) */
		reached[2][4] = 1;
		assert((((int)now.buffer_use[ Index(((((int)now.read_off)+((int)((P2 *)this)->i))%4), 4) ])==0), "(buffer_use[((read_off+i)%4)]==0)", II, tt, t);
		/* merge: buffer_use[((read_off+i)%4)] = 1(9, 5, 9) */
		reached[2][5] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.buffer_use[ Index(((((int)now.read_off)+((int)((P2 *)this)->i))%4), 4) ]);
		now.buffer_use[ Index(((now.read_off+((P2 *)this)->i)%4), 4) ] = 1;
#ifdef VAR_RANGES
		logval("buffer_use[((read_off+reader:i)%4)]", ((int)now.buffer_use[ Index(((((int)now.read_off)+((int)((P2 *)this)->i))%4), 4) ]));
#endif
		;
		/* merge: i = (i+1)(9, 6, 9) */
		reached[2][6] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P2 *)this)->i);
		((P2 *)this)->i = (((int)((P2 *)this)->i)+1);
#ifdef VAR_RANGES
		logval("reader:i", ((int)((P2 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 10, 9) */
		reached[2][10] = 1;
		;
		_m = 3; goto P999; /* 4 */
	case 29: /* STATE 7 - line 208 "pan.___" - [((i>=(4/2)))] (11:0:1 - 1) */
		IfNotBlocked
		reached[2][7] = 1;
		if (!((((int)((P2 *)this)->i)>=(4/2))))
			continue;
		/* dead 1: i */  (trpt+1)->bup.oval = ((P2 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P2 *)this)->i = 0;
		/* merge: goto :b3(0, 8, 11) */
		reached[2][8] = 1;
		;
		_m = 3; goto P999; /* 1 */
/* STATE 13 - line 216 "pan.___" - [i = 0] (0:0 - 1) same as 27 (0:0 - 1) */
	case 30: /* STATE 14 - line 218 "pan.___" - [((i<(4/2)))] (19:0:2 - 1) */
		IfNotBlocked
		reached[2][14] = 1;
		if (!((((int)((P2 *)this)->i)<(4/2))))
			continue;
		/* merge: buffer_use[((read_off+i)%4)] = 0(19, 15, 19) */
		reached[2][15] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.buffer_use[ Index(((((int)now.read_off)+((int)((P2 *)this)->i))%4), 4) ]);
		now.buffer_use[ Index(((now.read_off+((P2 *)this)->i)%4), 4) ] = 0;
#ifdef VAR_RANGES
		logval("buffer_use[((read_off+reader:i)%4)]", ((int)now.buffer_use[ Index(((((int)now.read_off)+((int)((P2 *)this)->i))%4), 4) ]));
#endif
		;
		/* merge: i = (i+1)(19, 16, 19) */
		reached[2][16] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P2 *)this)->i);
		((P2 *)this)->i = (((int)((P2 *)this)->i)+1);
#ifdef VAR_RANGES
		logval("reader:i", ((int)((P2 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 20, 19) */
		reached[2][20] = 1;
		;
		_m = 3; goto P999; /* 3 */
	case 31: /* STATE 17 - line 221 "pan.___" - [((i>=(4/2)))] (0:0:1 - 1) */
		IfNotBlocked
		reached[2][17] = 1;
		if (!((((int)((P2 *)this)->i)>=(4/2))))
			continue;
		/* dead 1: i */  (trpt+1)->bup.oval = ((P2 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P2 *)this)->i = 0;
		_m = 3; goto P999; /* 0 */
	case 32: /* STATE 22 - line 223 "pan.___" - [read_off = (read_off+(4/2))] (0:0:1 - 1) */
		IfNotBlocked
		reached[2][22] = 1;
		(trpt+1)->bup.oval = ((int)now.read_off);
		now.read_off = (((int)now.read_off)+(4/2));
#ifdef VAR_RANGES
		logval("read_off", ((int)now.read_off));
#endif
		;
		_m = 3; goto P999; /* 0 */
	case 33: /* STATE 24 - line 225 "pan.___" - [((read_off>=(4-events_lost)))] (0:0:0 - 1) */
		IfNotBlocked
		reached[2][24] = 1;
		if (!((((int)now.read_off)>=(4-((int)now.events_lost)))))
			continue;
		_m = 3; goto P999; /* 0 */
	case 34: /* STATE 29 - line 227 "pan.___" - [-end-] (0:0:0 - 3) */
		IfNotBlocked
		reached[2][29] = 1;
		if (!delproc(1, II)) continue;
		_m = 3; goto P999; /* 0 */

		 /* PROC tracer */
	case 35: /* STATE 1 - line 123 "pan.___" - [prev_off = write_off] (0:10:2 - 1) */
		IfNotBlocked
		reached[1][1] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)((P1 *)this)->prev_off);
		((P1 *)this)->prev_off = ((int)now.write_off);
#ifdef VAR_RANGES
		logval("tracer:prev_off", ((int)((P1 *)this)->prev_off));
#endif
		;
		/* merge: new_off = (prev_off+size)(10, 2, 10) */
		reached[1][2] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P1 *)this)->new_off);
		((P1 *)this)->new_off = (((int)((P1 *)this)->prev_off)+((int)((P1 *)this)->size));
#ifdef VAR_RANGES
		logval("tracer:new_off", ((int)((P1 *)this)->new_off));
#endif
		;
		_m = 3; goto P999; /* 1 */
	case 36: /* STATE 4 - line 128 "pan.___" - [((((new_off-read_off)>4)&&((new_off-read_off)<(255/2))))] (0:0:1 - 1) */
		IfNotBlocked
		reached[1][4] = 1;
		if (!((((((int)((P1 *)this)->new_off)-((int)now.read_off))>4)&&((((int)((P1 *)this)->new_off)-((int)now.read_off))<(255/2)))))
			continue;
		/* dead 1: new_off */  (trpt+1)->bup.oval = ((P1 *)this)->new_off;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P1 *)this)->new_off = 0;
		_m = 3; goto P999; /* 0 */
	case 37: /* STATE 7 - line 130 "pan.___" - [(1)] (27:0:0 - 1) */
		IfNotBlocked
		reached[1][7] = 1;
		if (!(1))
			continue;
		/* merge: .(goto)(0, 9, 27) */
		reached[1][9] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 38: /* STATE 11 - line 135 "pan.___" - [((prev_off!=write_off))] (3:0:1 - 1) */
		IfNotBlocked
		reached[1][11] = 1;
		if (!((((int)((P1 *)this)->prev_off)!=((int)now.write_off))))
			continue;
		/* dead 1: prev_off */  (trpt+1)->bup.oval = ((P1 *)this)->prev_off;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P1 *)this)->prev_off = 0;
		/* merge: goto cmpxchg_loop(0, 12, 3) */
		reached[1][12] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 39: /* STATE 14 - line 136 "pan.___" - [write_off = new_off] (0:24:2 - 1) */
		IfNotBlocked
		reached[1][14] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.write_off);
		now.write_off = ((int)((P1 *)this)->new_off);
#ifdef VAR_RANGES
		logval("write_off", ((int)now.write_off));
#endif
		;
		/* merge: .(goto)(24, 16, 24) */
		reached[1][16] = 1;
		;
		/* merge: i = 0(24, 17, 24) */
		reached[1][17] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P1 *)this)->i);
		((P1 *)this)->i = 0;
#ifdef VAR_RANGES
		logval("tracer:i", ((int)((P1 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 25, 24) */
		reached[1][25] = 1;
		;
		_m = 3; goto P999; /* 3 */
	case 40: /* STATE 17 - line 138 "pan.___" - [i = 0] (0:24:1 - 2) */
		IfNotBlocked
		reached[1][17] = 1;
		(trpt+1)->bup.oval = ((int)((P1 *)this)->i);
		((P1 *)this)->i = 0;
#ifdef VAR_RANGES
		logval("tracer:i", ((int)((P1 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 25, 24) */
		reached[1][25] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 41: /* STATE 18 - line 140 "pan.___" - [((i<size))] (24:0:2 - 1) */
		IfNotBlocked
		reached[1][18] = 1;
		if (!((((int)((P1 *)this)->i)<((int)((P1 *)this)->size))))
			continue;
		/* merge: assert((buffer_use[((prev_off+i)%4)]==0))(24, 19, 24) */
		reached[1][19] = 1;
		assert((((int)now.buffer_use[ Index(((((int)((P1 *)this)->prev_off)+((int)((P1 *)this)->i))%4), 4) ])==0), "(buffer_use[((prev_off+i)%4)]==0)", II, tt, t);
		/* merge: buffer_use[((prev_off+i)%4)] = 1(24, 20, 24) */
		reached[1][20] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.buffer_use[ Index(((((int)((P1 *)this)->prev_off)+((int)((P1 *)this)->i))%4), 4) ]);
		now.buffer_use[ Index(((((P1 *)this)->prev_off+((P1 *)this)->i)%4), 4) ] = 1;
#ifdef VAR_RANGES
		logval("buffer_use[((tracer:prev_off+tracer:i)%4)]", ((int)now.buffer_use[ Index(((((int)((P1 *)this)->prev_off)+((int)((P1 *)this)->i))%4), 4) ]));
#endif
		;
		/* merge: i = (i+1)(24, 21, 24) */
		reached[1][21] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P1 *)this)->i);
		((P1 *)this)->i = (((int)((P1 *)this)->i)+1);
#ifdef VAR_RANGES
		logval("tracer:i", ((int)((P1 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 25, 24) */
		reached[1][25] = 1;
		;
		_m = 3; goto P999; /* 4 */
	case 42: /* STATE 22 - line 144 "pan.___" - [((i>=size))] (26:0:1 - 1) */
		IfNotBlocked
		reached[1][22] = 1;
		if (!((((int)((P1 *)this)->i)>=((int)((P1 *)this)->size))))
			continue;
		/* dead 1: i */  (trpt+1)->bup.oval = ((P1 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P1 *)this)->i = 0;
		/* merge: goto :b0(0, 23, 26) */
		reached[1][23] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 43: /* STATE 28 - line 152 "pan.___" - [i = 0] (0:0:1 - 1) */
		IfNotBlocked
		reached[1][28] = 1;
		(trpt+1)->bup.oval = ((int)((P1 *)this)->i);
		((P1 *)this)->i = 0;
#ifdef VAR_RANGES
		logval("tracer:i", ((int)((P1 *)this)->i));
#endif
		;
		_m = 3; goto P999; /* 0 */
	case 44: /* STATE 29 - line 154 "pan.___" - [((i<size))] (34:0:2 - 1) */
		IfNotBlocked
		reached[1][29] = 1;
		if (!((((int)((P1 *)this)->i)<((int)((P1 *)this)->size))))
			continue;
		/* merge: buffer_use[((prev_off+i)%4)] = 0(34, 30, 34) */
		reached[1][30] = 1;
		(trpt+1)->bup.ovals = grab_ints(2);
		(trpt+1)->bup.ovals[0] = ((int)now.buffer_use[ Index(((((int)((P1 *)this)->prev_off)+((int)((P1 *)this)->i))%4), 4) ]);
		now.buffer_use[ Index(((((P1 *)this)->prev_off+((P1 *)this)->i)%4), 4) ] = 0;
#ifdef VAR_RANGES
		logval("buffer_use[((tracer:prev_off+tracer:i)%4)]", ((int)now.buffer_use[ Index(((((int)((P1 *)this)->prev_off)+((int)((P1 *)this)->i))%4), 4) ]));
#endif
		;
		/* merge: i = (i+1)(34, 31, 34) */
		reached[1][31] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P1 *)this)->i);
		((P1 *)this)->i = (((int)((P1 *)this)->i)+1);
#ifdef VAR_RANGES
		logval("tracer:i", ((int)((P1 *)this)->i));
#endif
		;
		/* merge: .(goto)(0, 35, 34) */
		reached[1][35] = 1;
		;
		_m = 3; goto P999; /* 3 */
	case 45: /* STATE 32 - line 157 "pan.___" - [((i>=size))] (44:0:4 - 1) */
		IfNotBlocked
		reached[1][32] = 1;
		if (!((((int)((P1 *)this)->i)>=((int)((P1 *)this)->size))))
			continue;
		/* dead 1: i */  (trpt+1)->bup.ovals = grab_ints(4);
		(trpt+1)->bup.ovals[0] = ((P1 *)this)->i;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P1 *)this)->i = 0;
		/* merge: goto :b1(44, 33, 44) */
		reached[1][33] = 1;
		;
		/* merge: tmp_commit = (commit_count[((prev_off%4)/(4/2))]+size)(44, 37, 44) */
		reached[1][37] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P1 *)this)->tmp_commit);
		((P1 *)this)->tmp_commit = (((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ])+((int)((P1 *)this)->size));
#ifdef VAR_RANGES
		logval("tracer:tmp_commit", ((int)((P1 *)this)->tmp_commit));
#endif
		;
		/* merge: _commit_sum = ((_commit_sum-commit_count[((prev_off%4)/(4/2))])+tmp_commit)(44, 38, 44) */
		reached[1][38] = 1;
		(trpt+1)->bup.ovals[2] = ((int)now._commit_sum);
		now._commit_sum = ((((int)now._commit_sum)-((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ]))+((int)((P1 *)this)->tmp_commit));
#ifdef VAR_RANGES
		logval("_commit_sum", ((int)now._commit_sum));
#endif
		;
		/* merge: commit_count[((prev_off%4)/(4/2))] = tmp_commit(44, 39, 44) */
		reached[1][39] = 1;
		(trpt+1)->bup.ovals[3] = ((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ]);
		now.commit_count[ Index(((((P1 *)this)->prev_off%4)/(4/2)), 2) ] = ((int)((P1 *)this)->tmp_commit);
#ifdef VAR_RANGES
		logval("commit_count[((tracer:prev_off%4)/(4/2))]", ((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ]));
#endif
		;
		_m = 3; goto P999; /* 4 */
	case 46: /* STATE 37 - line 159 "pan.___" - [tmp_commit = (commit_count[((prev_off%4)/(4/2))]+size)] (0:44:3 - 3) */
		IfNotBlocked
		reached[1][37] = 1;
		(trpt+1)->bup.ovals = grab_ints(3);
		(trpt+1)->bup.ovals[0] = ((int)((P1 *)this)->tmp_commit);
		((P1 *)this)->tmp_commit = (((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ])+((int)((P1 *)this)->size));
#ifdef VAR_RANGES
		logval("tracer:tmp_commit", ((int)((P1 *)this)->tmp_commit));
#endif
		;
		/* merge: _commit_sum = ((_commit_sum-commit_count[((prev_off%4)/(4/2))])+tmp_commit)(44, 38, 44) */
		reached[1][38] = 1;
		(trpt+1)->bup.ovals[1] = ((int)now._commit_sum);
		now._commit_sum = ((((int)now._commit_sum)-((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ]))+((int)((P1 *)this)->tmp_commit));
#ifdef VAR_RANGES
		logval("_commit_sum", ((int)now._commit_sum));
#endif
		;
		/* merge: commit_count[((prev_off%4)/(4/2))] = tmp_commit(44, 39, 44) */
		reached[1][39] = 1;
		(trpt+1)->bup.ovals[2] = ((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ]);
		now.commit_count[ Index(((((P1 *)this)->prev_off%4)/(4/2)), 2) ] = ((int)((P1 *)this)->tmp_commit);
#ifdef VAR_RANGES
		logval("commit_count[((tracer:prev_off%4)/(4/2))]", ((int)now.commit_count[ Index(((((int)((P1 *)this)->prev_off)%4)/(4/2)), 2) ]));
#endif
		;
		_m = 3; goto P999; /* 2 */
	case 47: /* STATE 40 - line 167 "pan.___" - [((((((prev_off/4)*4)/2)+(4/2))-tmp_commit))] (50:0:3 - 1) */
		IfNotBlocked
		reached[1][40] = 1;
		if (!((((((((int)((P1 *)this)->prev_off)/4)*4)/2)+(4/2))-((int)((P1 *)this)->tmp_commit))))
			continue;
		/* dead 1: prev_off */  (trpt+1)->bup.ovals = grab_ints(3);
		(trpt+1)->bup.ovals[0] = ((P1 *)this)->prev_off;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P1 *)this)->prev_off = 0;
		/* dead 1: tmp_commit */  (trpt+1)->bup.ovals[1] = ((P1 *)this)->tmp_commit;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P1 *)this)->tmp_commit = 0;
		/* merge: deliver = 1(50, 41, 50) */
		reached[1][41] = 1;
		(trpt+1)->bup.ovals[2] = ((int)deliver);
		deliver = 1;
#ifdef VAR_RANGES
		logval("deliver", ((int)deliver));
#endif
		;
		/* merge: .(goto)(50, 45, 50) */
		reached[1][45] = 1;
		;
		_m = 3; goto P999; /* 2 */
	case 48: /* STATE 45 - line 172 "pan.___" - [.(goto)] (0:50:0 - 2) */
		IfNotBlocked
		reached[1][45] = 1;
		;
		_m = 3; goto P999; /* 0 */
	case 49: /* STATE 43 - line 170 "pan.___" - [(1)] (50:0:0 - 1) */
		IfNotBlocked
		reached[1][43] = 1;
		if (!(1))
			continue;
		/* merge: .(goto)(50, 45, 50) */
		reached[1][45] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 50: /* STATE 48 - line 176 "pan.___" - [events_lost = (events_lost+1)] (0:0:1 - 2) */
		IfNotBlocked
		reached[1][48] = 1;
		(trpt+1)->bup.oval = ((int)now.events_lost);
		now.events_lost = (((int)now.events_lost)+1);
#ifdef VAR_RANGES
		logval("events_lost", ((int)now.events_lost));
#endif
		;
		_m = 3; goto P999; /* 0 */
	case 51: /* STATE 49 - line 178 "pan.___" - [refcount = (refcount-1)] (0:0:1 - 2) */
		IfNotBlocked
		reached[1][49] = 1;
		(trpt+1)->bup.oval = ((int)now.refcount);
		now.refcount = (((int)now.refcount)-1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 0 */
	case 52: /* STATE 51 - line 180 "pan.___" - [-end-] (0:0:0 - 1) */
		IfNotBlocked
		reached[1][51] = 1;
		if (!delproc(1, II)) continue;
		_m = 3; goto P999; /* 0 */

		 /* PROC switcher */
	case 53: /* STATE 1 - line 72 "pan.___" - [prev_off = write_off] (0:9:3 - 1) */
		IfNotBlocked
		reached[0][1] = 1;
		(trpt+1)->bup.ovals = grab_ints(3);
		(trpt+1)->bup.ovals[0] = ((int)((P0 *)this)->prev_off);
		((P0 *)this)->prev_off = ((int)now.write_off);
#ifdef VAR_RANGES
		logval("switcher:prev_off", ((int)((P0 *)this)->prev_off));
#endif
		;
		/* merge: size = ((4/2)-(prev_off%(4/2)))(9, 2, 9) */
		reached[0][2] = 1;
		(trpt+1)->bup.ovals[1] = ((int)((P0 *)this)->size);
		((P0 *)this)->size = ((4/2)-(((int)((P0 *)this)->prev_off)%(4/2)));
#ifdef VAR_RANGES
		logval("switcher:size", ((int)((P0 *)this)->size));
#endif
		;
		/* merge: new_off = (prev_off+size)(9, 3, 9) */
		reached[0][3] = 1;
		(trpt+1)->bup.ovals[2] = ((int)((P0 *)this)->new_off);
		((P0 *)this)->new_off = (((int)((P0 *)this)->prev_off)+((int)((P0 *)this)->size));
#ifdef VAR_RANGES
		logval("switcher:new_off", ((int)((P0 *)this)->new_off));
#endif
		;
		_m = 3; goto P999; /* 2 */
	case 54: /* STATE 4 - line 77 "pan.___" - [(((((new_off-read_off)>4)&&((new_off-read_off)<(255/2)))||(size==(4/2))))] (30:0:3 - 1) */
		IfNotBlocked
		reached[0][4] = 1;
		if (!(((((((int)((P0 *)this)->new_off)-((int)now.read_off))>4)&&((((int)((P0 *)this)->new_off)-((int)now.read_off))<(255/2)))||(((int)((P0 *)this)->size)==(4/2)))))
			continue;
		/* dead 1: new_off */  (trpt+1)->bup.ovals = grab_ints(3);
		(trpt+1)->bup.ovals[0] = ((P0 *)this)->new_off;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P0 *)this)->new_off = 0;
		/* dead 1: size */  (trpt+1)->bup.ovals[1] = ((P0 *)this)->size;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P0 *)this)->size = 0;
		/* merge: refcount = (refcount-1)(30, 5, 30) */
		reached[0][5] = 1;
		(trpt+1)->bup.ovals[2] = ((int)now.refcount);
		now.refcount = (((int)now.refcount)-1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		/* merge: goto not_needed(30, 6, 30) */
		reached[0][6] = 1;
		;
		_m = 3; goto P999; /* 2 */
	case 55: /* STATE 8 - line 80 "pan.___" - [(1)] (18:0:0 - 1) */
		IfNotBlocked
		reached[0][8] = 1;
		if (!(1))
			continue;
		/* merge: .(goto)(0, 10, 18) */
		reached[0][10] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 56: /* STATE 12 - line 85 "pan.___" - [((prev_off!=write_off))] (11:0:1 - 1) */
		IfNotBlocked
		reached[0][12] = 1;
		if (!((((int)((P0 *)this)->prev_off)!=((int)now.write_off))))
			continue;
		/* dead 1: prev_off */  (trpt+1)->bup.oval = ((P0 *)this)->prev_off;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P0 *)this)->prev_off = 0;
		/* merge: goto cmpxchg_loop(0, 13, 11) */
		reached[0][13] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 57: /* STATE 17 - line 88 "pan.___" - [.(goto)] (0:29:0 - 1) */
		IfNotBlocked
		reached[0][17] = 1;
		;
		_m = 3; goto P999; /* 0 */
	case 58: /* STATE 15 - line 86 "pan.___" - [write_off = new_off] (0:29:1 - 1) */
		IfNotBlocked
		reached[0][15] = 1;
		(trpt+1)->bup.oval = ((int)now.write_off);
		now.write_off = ((int)((P0 *)this)->new_off);
#ifdef VAR_RANGES
		logval("write_off", ((int)now.write_off));
#endif
		;
		/* merge: .(goto)(29, 17, 29) */
		reached[0][17] = 1;
		;
		_m = 3; goto P999; /* 1 */
	case 59: /* STATE 19 - line 91 "pan.___" - [tmp_commit = (commit_count[((prev_off%4)/(4/2))]+size)] (0:26:3 - 1) */
		IfNotBlocked
		reached[0][19] = 1;
		(trpt+1)->bup.ovals = grab_ints(3);
		(trpt+1)->bup.ovals[0] = ((int)((P0 *)this)->tmp_commit);
		((P0 *)this)->tmp_commit = (((int)now.commit_count[ Index(((((int)((P0 *)this)->prev_off)%4)/(4/2)), 2) ])+((int)((P0 *)this)->size));
#ifdef VAR_RANGES
		logval("switcher:tmp_commit", ((int)((P0 *)this)->tmp_commit));
#endif
		;
		/* merge: _commit_sum = ((_commit_sum-commit_count[((prev_off%4)/(4/2))])+tmp_commit)(26, 20, 26) */
		reached[0][20] = 1;
		(trpt+1)->bup.ovals[1] = ((int)now._commit_sum);
		now._commit_sum = ((((int)now._commit_sum)-((int)now.commit_count[ Index(((((int)((P0 *)this)->prev_off)%4)/(4/2)), 2) ]))+((int)((P0 *)this)->tmp_commit));
#ifdef VAR_RANGES
		logval("_commit_sum", ((int)now._commit_sum));
#endif
		;
		/* merge: commit_count[((prev_off%4)/(4/2))] = tmp_commit(26, 21, 26) */
		reached[0][21] = 1;
		(trpt+1)->bup.ovals[2] = ((int)now.commit_count[ Index(((((int)((P0 *)this)->prev_off)%4)/(4/2)), 2) ]);
		now.commit_count[ Index(((((P0 *)this)->prev_off%4)/(4/2)), 2) ] = ((int)((P0 *)this)->tmp_commit);
#ifdef VAR_RANGES
		logval("commit_count[((switcher:prev_off%4)/(4/2))]", ((int)now.commit_count[ Index(((((int)((P0 *)this)->prev_off)%4)/(4/2)), 2) ]));
#endif
		;
		_m = 3; goto P999; /* 2 */
	case 60: /* STATE 22 - line 99 "pan.___" - [((((((prev_off/4)*4)/2)+(4/2))-tmp_commit))] (30:0:4 - 1) */
		IfNotBlocked
		reached[0][22] = 1;
		if (!((((((((int)((P0 *)this)->prev_off)/4)*4)/2)+(4/2))-((int)((P0 *)this)->tmp_commit))))
			continue;
		/* dead 1: prev_off */  (trpt+1)->bup.ovals = grab_ints(4);
		(trpt+1)->bup.ovals[0] = ((P0 *)this)->prev_off;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P0 *)this)->prev_off = 0;
		/* dead 1: tmp_commit */  (trpt+1)->bup.ovals[1] = ((P0 *)this)->tmp_commit;
#ifdef HAS_CODE
		if (!readtrail)
#endif
			((P0 *)this)->tmp_commit = 0;
		/* merge: deliver = 1(30, 23, 30) */
		reached[0][23] = 1;
		(trpt+1)->bup.ovals[2] = ((int)deliver);
		deliver = 1;
#ifdef VAR_RANGES
		logval("deliver", ((int)deliver));
#endif
		;
		/* merge: .(goto)(30, 27, 30) */
		reached[0][27] = 1;
		;
		/* merge: refcount = (refcount-1)(30, 28, 30) */
		reached[0][28] = 1;
		(trpt+1)->bup.ovals[3] = ((int)now.refcount);
		now.refcount = (((int)now.refcount)-1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 3 */
	case 61: /* STATE 27 - line 104 "pan.___" - [.(goto)] (0:30:1 - 2) */
		IfNotBlocked
		reached[0][27] = 1;
		;
		/* merge: refcount = (refcount-1)(30, 28, 30) */
		reached[0][28] = 1;
		(trpt+1)->bup.oval = ((int)now.refcount);
		now.refcount = (((int)now.refcount)-1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 1 */
	case 62: /* STATE 25 - line 102 "pan.___" - [(1)] (30:0:1 - 1) */
		IfNotBlocked
		reached[0][25] = 1;
		if (!(1))
			continue;
		/* merge: .(goto)(30, 27, 30) */
		reached[0][27] = 1;
		;
		/* merge: refcount = (refcount-1)(30, 28, 30) */
		reached[0][28] = 1;
		(trpt+1)->bup.oval = ((int)now.refcount);
		now.refcount = (((int)now.refcount)-1);
#ifdef VAR_RANGES
		logval("refcount", ((int)now.refcount));
#endif
		;
		_m = 3; goto P999; /* 2 */
	case 63: /* STATE 31 - line 108 "pan.___" - [-end-] (0:0:0 - 1) */
		IfNotBlocked
		reached[0][31] = 1;
		if (!delproc(1, II)) continue;
		_m = 3; goto P999; /* 0 */
	case  _T5:	/* np_ */
		if (!((!(trpt->o_pm&4) && !(trpt->tau&128))))
			continue;
		/* else fall through */
	case  _T2:	/* true */
		_m = 3; goto P999;
#undef rand
	}

