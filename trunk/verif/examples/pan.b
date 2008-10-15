	switch (t->back) {
	default: Uerror("bad return move");
	case  0: goto R999; /* nothing to undo */

		 /* PROC :init: */

	case 3: /* STATE 1 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 4: /* STATE 4 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
		now.commit_count[ Index(((P4 *)this)->i, 2) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 5: /* STATE 10 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
	/* 0 */	((P4 *)this)->i = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 6: /* STATE 10 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 7: /* STATE 13 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
		now.buffer_use[ Index(((P4 *)this)->i, 4) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 8: /* STATE 14 */
		;
	/* 0 */	((P4 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 9: /* STATE 19 */
		;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 10: /* STATE 21 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 11: /* STATE 23 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 12: /* STATE 25 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 13: /* STATE 31 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
	/* 0 */	((P4 *)this)->i = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 14: /* STATE 31 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 15: /* STATE 33 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 16: /* STATE 35 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 17: /* STATE 36 */
		;
	/* 0 */	((P4 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 18: /* STATE 44 */
		;
		((P4 *)this)->commit_sum = trpt->bup.ovals[1];
		((P4 *)this)->j = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 19: /* STATE 47 */
		;
		((P4 *)this)->j = trpt->bup.ovals[1];
		((P4 *)this)->commit_sum = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 20: /* STATE 48 */
		;
	/* 0 */	((P4 *)this)->j = trpt->bup.oval;
		;
		;
		goto R999;
;
		
	case 21: /* STATE 53 */
		goto R999;

	case 22: /* STATE 56 */
		;
		p_restor(II);
		;
		;
		goto R999;

		 /* PROC cleaner */

	case 23: /* STATE 2 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 24: /* STATE 3 */
		;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 25: /* STATE 9 */
		;
		p_restor(II);
		;
		;
		goto R999;

		 /* PROC reader */
;
		;
		
	case 27: /* STATE 2 */
		;
		((P2 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 28: /* STATE 6 */
		;
		((P2 *)this)->i = trpt->bup.ovals[1];
		now.buffer_use[ Index(((now.read_off+((P2 *)this)->i)%4), 4) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 29: /* STATE 7 */
		;
	/* 0 */	((P2 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 30: /* STATE 16 */
		;
		((P2 *)this)->i = trpt->bup.ovals[1];
		now.buffer_use[ Index(((now.read_off+((P2 *)this)->i)%4), 4) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 31: /* STATE 17 */
		;
	/* 0 */	((P2 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 32: /* STATE 22 */
		;
		now.read_off = trpt->bup.oval;
		;
		goto R999;
;
		;
		
	case 34: /* STATE 29 */
		;
		p_restor(II);
		;
		;
		goto R999;

		 /* PROC tracer */

	case 35: /* STATE 2 */
		;
		((P1 *)this)->new_off = trpt->bup.ovals[1];
		((P1 *)this)->prev_off = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 36: /* STATE 4 */
		;
	/* 0 */	((P1 *)this)->new_off = trpt->bup.oval;
		;
		;
		goto R999;
;
		
	case 37: /* STATE 7 */
		goto R999;

	case 38: /* STATE 11 */
		;
	/* 0 */	((P1 *)this)->prev_off = trpt->bup.oval;
		;
		;
		goto R999;

	case 39: /* STATE 17 */
		;
		((P1 *)this)->i = trpt->bup.ovals[1];
		now.write_off = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 40: /* STATE 17 */
		;
		((P1 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 41: /* STATE 21 */
		;
		((P1 *)this)->i = trpt->bup.ovals[1];
		now.buffer_use[ Index(((((P1 *)this)->prev_off+((P1 *)this)->i)%4), 4) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 42: /* STATE 22 */
		;
	/* 0 */	((P1 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 43: /* STATE 28 */
		;
		((P1 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 44: /* STATE 31 */
		;
		((P1 *)this)->i = trpt->bup.ovals[1];
		now.buffer_use[ Index(((((P1 *)this)->prev_off+((P1 *)this)->i)%4), 4) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 45: /* STATE 38 */
		;
		now.commit_count[ Index(((((P1 *)this)->prev_off%4)/(4/2)), 2) ] = trpt->bup.ovals[2];
		((P1 *)this)->tmp_commit = trpt->bup.ovals[1];
	/* 0 */	((P1 *)this)->i = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;

	case 46: /* STATE 38 */
		;
		now.commit_count[ Index(((((P1 *)this)->prev_off%4)/(4/2)), 2) ] = trpt->bup.ovals[1];
		((P1 *)this)->tmp_commit = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 47: /* STATE 40 */
		;
		deliver = trpt->bup.ovals[2];
	/* 1 */	((P1 *)this)->tmp_commit = trpt->bup.ovals[1];
	/* 0 */	((P1 *)this)->prev_off = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;
;
		
	case 48: /* STATE 44 */
		goto R999;
;
		
	case 49: /* STATE 42 */
		goto R999;

	case 50: /* STATE 47 */
		;
		now.events_lost = trpt->bup.oval;
		;
		goto R999;

	case 51: /* STATE 48 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 52: /* STATE 50 */
		;
		p_restor(II);
		;
		;
		goto R999;

		 /* PROC switcher */

	case 53: /* STATE 3 */
		;
		((P0 *)this)->new_off = trpt->bup.ovals[2];
		((P0 *)this)->size = trpt->bup.ovals[1];
		((P0 *)this)->prev_off = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;

	case 54: /* STATE 5 */
		;
		now.refcount = trpt->bup.ovals[2];
	/* 1 */	((P0 *)this)->size = trpt->bup.ovals[1];
	/* 0 */	((P0 *)this)->new_off = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;
;
		
	case 55: /* STATE 8 */
		goto R999;

	case 56: /* STATE 12 */
		;
	/* 0 */	((P0 *)this)->prev_off = trpt->bup.oval;
		;
		;
		goto R999;
;
		
	case 57: /* STATE 17 */
		goto R999;

	case 58: /* STATE 15 */
		;
		now.write_off = trpt->bup.oval;
		;
		goto R999;

	case 59: /* STATE 20 */
		;
		now.commit_count[ Index(((((P0 *)this)->prev_off%4)/(4/2)), 2) ] = trpt->bup.ovals[1];
		((P0 *)this)->tmp_commit = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 60: /* STATE 27 */
		;
		now.refcount = trpt->bup.ovals[3];
		deliver = trpt->bup.ovals[2];
	/* 1 */	((P0 *)this)->tmp_commit = trpt->bup.ovals[1];
	/* 0 */	((P0 *)this)->prev_off = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 4);
		goto R999;

	case 61: /* STATE 27 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 62: /* STATE 27 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 63: /* STATE 30 */
		;
		p_restor(II);
		;
		;
		goto R999;
	}

