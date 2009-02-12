	switch (t->back) {
	default: Uerror("bad return move");
	case  0: goto R999; /* nothing to undo */

		 /* PROC :never: */
;
		;
		;
		;
		;
		;
		
	case 6: /* STATE 14 */
		;
		p_restor(II);
		;
		;
		goto R999;

		 /* PROC :init: */

	case 7: /* STATE 1 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 8: /* STATE 4 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
		now.commit_count[ Index(((P4 *)this)->i, 2) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 9: /* STATE 11 */
		;
		((P4 *)this)->i = trpt->bup.ovals[2];
		now._commit_sum = trpt->bup.ovals[1];
	/* 0 */	((P4 *)this)->i = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;

	case 10: /* STATE 11 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
		now._commit_sum = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 11: /* STATE 14 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
		now.buffer_use[ Index(((P4 *)this)->i, 4) ] = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 12: /* STATE 15 */
		;
	/* 0 */	((P4 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 13: /* STATE 20 */
		;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 14: /* STATE 22 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 15: /* STATE 24 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 16: /* STATE 26 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 17: /* STATE 32 */
		;
		((P4 *)this)->i = trpt->bup.ovals[1];
	/* 0 */	((P4 *)this)->i = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 2);
		goto R999;

	case 18: /* STATE 32 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		goto R999;

	case 19: /* STATE 34 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 20: /* STATE 36 */
		;
		((P4 *)this)->i = trpt->bup.oval;
		;
		delproc(0, now._nr_pr-1);
		;
		goto R999;

	case 21: /* STATE 37 */
		;
	/* 0 */	((P4 *)this)->i = trpt->bup.oval;
		;
		;
		goto R999;

	case 22: /* STATE 43 */
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

	case 45: /* STATE 39 */
		;
		now.commit_count[ Index(((((P1 *)this)->prev_off%4)/(4/2)), 2) ] = trpt->bup.ovals[3];
		now._commit_sum = trpt->bup.ovals[2];
		((P1 *)this)->tmp_commit = trpt->bup.ovals[1];
	/* 0 */	((P1 *)this)->i = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 4);
		goto R999;

	case 46: /* STATE 39 */
		;
		now.commit_count[ Index(((((P1 *)this)->prev_off%4)/(4/2)), 2) ] = trpt->bup.ovals[2];
		now._commit_sum = trpt->bup.ovals[1];
		((P1 *)this)->tmp_commit = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;

	case 47: /* STATE 41 */
		;
		deliver = trpt->bup.ovals[2];
	/* 1 */	((P1 *)this)->tmp_commit = trpt->bup.ovals[1];
	/* 0 */	((P1 *)this)->prev_off = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;
;
		
	case 48: /* STATE 45 */
		goto R999;
;
		
	case 49: /* STATE 43 */
		goto R999;

	case 50: /* STATE 48 */
		;
		now.events_lost = trpt->bup.oval;
		;
		goto R999;

	case 51: /* STATE 49 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 52: /* STATE 51 */
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

	case 59: /* STATE 21 */
		;
		now.commit_count[ Index(((((P0 *)this)->prev_off%4)/(4/2)), 2) ] = trpt->bup.ovals[2];
		now._commit_sum = trpt->bup.ovals[1];
		((P0 *)this)->tmp_commit = trpt->bup.ovals[0];
		;
		ungrab_ints(trpt->bup.ovals, 3);
		goto R999;

	case 60: /* STATE 28 */
		;
		now.refcount = trpt->bup.ovals[3];
		deliver = trpt->bup.ovals[2];
	/* 1 */	((P0 *)this)->tmp_commit = trpt->bup.ovals[1];
	/* 0 */	((P0 *)this)->prev_off = trpt->bup.ovals[0];
		;
		;
		ungrab_ints(trpt->bup.ovals, 4);
		goto R999;

	case 61: /* STATE 28 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 62: /* STATE 28 */
		;
		now.refcount = trpt->bup.oval;
		;
		goto R999;

	case 63: /* STATE 31 */
		;
		p_restor(II);
		;
		;
		goto R999;
	}

