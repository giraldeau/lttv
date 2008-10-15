#ifdef PEG
struct T_SRC {
	char *fl; int ln;
} T_SRC[NTRANS];

void
tr_2_src(int m, char *file, int ln)
{	T_SRC[m].fl = file;
	T_SRC[m].ln = ln;
}

void
putpeg(int n, int m)
{	printf("%5d	trans %4d ", m, n);
	printf("file %s line %3d\n",
		T_SRC[n].fl, T_SRC[n].ln);
}
#endif

void
settable(void)
{	Trans *T;
	Trans *settr(int, int, int, int, int, char *, int, int, int);

	trans = (Trans ***) emalloc(6*sizeof(Trans **));

	/* proctype 4: :init: */

	trans[4] = (Trans **) emalloc(57*sizeof(Trans *));

	T = trans[ 4][41] = settr(158,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(158,2,1,0,0,"ATOMIC", 1, 2, 0);
	trans[4][1]	= settr(118,2,7,3,3,"i = 0", 1, 2, 0);
	trans[4][8]	= settr(125,2,7,1,0,".(goto)", 1, 2, 0);
	T = trans[4][7] = settr(124,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(124,2,2,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(124,2,5,0,0,"DO", 1, 2, 0);
	trans[4][2]	= settr(119,2,7,4,4,"((i<2))", 1, 2, 0); /* m: 3 -> 7,0 */
	reached4[3] = 1;
	trans[4][3]	= settr(0,0,0,0,0,"commit_count[i] = 0",0,0,0);
	trans[4][4]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[4][5]	= settr(122,2,16,5,5,"((i>=2))", 1, 2, 0); /* m: 10 -> 16,0 */
	reached4[10] = 1;
	trans[4][6]	= settr(123,2,10,1,0,"goto :b6", 1, 2, 0); /* m: 10 -> 0,16 */
	reached4[10] = 1;
	trans[4][9]	= settr(126,2,10,1,0,"break", 1, 2, 0);
	trans[4][10]	= settr(127,2,16,6,6,"i = 0", 1, 2, 0);
	trans[4][17]	= settr(134,2,16,1,0,".(goto)", 1, 2, 0);
	T = trans[4][16] = settr(133,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(133,2,11,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(133,2,14,0,0,"DO", 1, 2, 0);
	trans[4][11]	= settr(128,2,16,7,7,"((i<4))", 1, 2, 0); /* m: 12 -> 16,0 */
	reached4[12] = 1;
	trans[4][12]	= settr(0,0,0,0,0,"buffer_use[i] = 0",0,0,0);
	trans[4][13]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[4][14]	= settr(131,2,19,8,8,"((i>=4))", 1, 2, 0);
	trans[4][15]	= settr(132,2,19,1,0,"goto :b7", 1, 2, 0);
	trans[4][18]	= settr(135,2,19,1,0,"break", 1, 2, 0);
	trans[4][19]	= settr(136,2,20,9,9,"(run reader())", 1, 2, 0);
	trans[4][20]	= settr(137,2,28,10,10,"(run cleaner())", 1, 2, 0); /* m: 21 -> 28,0 */
	reached4[21] = 1;
	trans[4][21]	= settr(0,0,0,0,0,"i = 0",0,0,0);
	trans[4][29]	= settr(146,2,28,1,0,".(goto)", 1, 2, 0);
	T = trans[4][28] = settr(145,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(145,2,22,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(145,2,26,0,0,"DO", 1, 2, 0);
	trans[4][22]	= settr(139,2,24,11,11,"((i<4))", 1, 2, 0); /* m: 23 -> 24,0 */
	reached4[23] = 1;
	trans[4][23]	= settr(0,0,0,0,0,"refcount = (refcount+1)",0,0,0);
	trans[4][24]	= settr(141,2,28,12,12,"(run tracer())", 1, 2, 0); /* m: 25 -> 28,0 */
	reached4[25] = 1;
	trans[4][25]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[4][26]	= settr(143,2,38,13,13,"((i>=4))", 1, 2, 0); /* m: 31 -> 38,0 */
	reached4[31] = 1;
	trans[4][27]	= settr(144,2,31,1,0,"goto :b8", 1, 2, 0); /* m: 31 -> 0,38 */
	reached4[31] = 1;
	trans[4][30]	= settr(147,2,31,1,0,"break", 1, 2, 0);
	trans[4][31]	= settr(148,2,38,14,14,"i = 0", 1, 2, 0);
	trans[4][39]	= settr(156,2,38,1,0,".(goto)", 1, 2, 0);
	T = trans[4][38] = settr(155,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(155,2,32,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(155,2,36,0,0,"DO", 1, 2, 0);
	trans[4][32]	= settr(149,2,34,15,15,"((i<1))", 1, 2, 0); /* m: 33 -> 34,0 */
	reached4[33] = 1;
	trans[4][33]	= settr(0,0,0,0,0,"refcount = (refcount+1)",0,0,0);
	trans[4][34]	= settr(151,2,38,16,16,"(run switcher())", 1, 2, 0); /* m: 35 -> 38,0 */
	reached4[35] = 1;
	trans[4][35]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[4][36]	= settr(153,2,40,17,17,"((i>=1))", 1, 2, 0); /* m: 37 -> 40,0 */
	reached4[37] = 1;
	trans[4][37]	= settr(154,2,40,1,0,"goto :b9", 1, 2, 0);
	trans[4][40]	= settr(157,0,55,1,0,"break", 1, 2, 0);
	T = trans[ 4][55] = settr(172,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(172,2,42,0,0,"ATOMIC", 1, 2, 0);
	trans[4][42]	= settr(159,2,50,18,18,"assert((((write_off-read_off)>=0)&&((write_off-read_off)<(255/2))))", 1, 2, 0); /* m: 43 -> 0,50 */
	reached4[43] = 1;
	trans[4][43]	= settr(0,0,0,0,0,"j = 0",0,0,0);
	trans[4][44]	= settr(0,0,0,0,0,"commit_sum = 0",0,0,0);
	trans[4][51]	= settr(168,2,50,1,0,".(goto)", 1, 2, 0);
	T = trans[4][50] = settr(167,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(167,2,45,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(167,2,48,0,0,"DO", 1, 2, 0);
	trans[4][45]	= settr(162,2,50,19,19,"((j<2))", 1, 2, 0); /* m: 46 -> 50,0 */
	reached4[46] = 1;
	trans[4][46]	= settr(0,0,0,0,0,"commit_sum = (commit_sum+commit_count[j])",0,0,0);
	trans[4][47]	= settr(0,0,0,0,0,"j = (j+1)",0,0,0);
	trans[4][48]	= settr(165,4,56,20,20,"((j>=2))", 1, 2, 0); /* m: 53 -> 56,0 */
	reached4[53] = 1;
	trans[4][49]	= settr(166,2,53,1,0,"goto :b10", 1, 2, 0); /* m: 53 -> 0,56 */
	reached4[53] = 1;
	trans[4][52]	= settr(169,2,53,1,0,"break", 1, 2, 0);
	trans[4][53]	= settr(170,4,56,21,21,"assert((((write_off-commit_sum)>=0)&&((write_off-commit_sum)<(255/2))))", 1, 2, 0); /* m: 54 -> 0,56 */
	reached4[54] = 1;
	trans[4][54]	= settr(0,0,0,0,0,"assert((((4+1)>4)||(events_lost==0)))",0,0,0);
	trans[4][56]	= settr(173,0,0,22,22,"-end-", 0, 3500, 0);

	/* proctype 3: cleaner */

	trans[3] = (Trans **) emalloc(10*sizeof(Trans *));

	T = trans[ 3][8] = settr(116,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(116,2,5,0,0,"ATOMIC", 1, 2, 0);
	trans[3][6]	= settr(114,2,5,1,0,".(goto)", 1, 2, 0);
	T = trans[3][5] = settr(113,2,0,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(113,2,1,0,0,"DO", 1, 2, 0);
	trans[3][1]	= settr(109,2,3,23,23,"((refcount==0))", 1, 2, 0); /* m: 2 -> 3,0 */
	reached3[2] = 1;
	trans[3][2]	= settr(0,0,0,0,0,"refcount = (refcount+1)",0,0,0);
	trans[3][3]	= settr(111,2,7,24,24,"(run switcher())", 1, 2, 0); /* m: 4 -> 7,0 */
	reached3[4] = 1;
	trans[3][4]	= settr(112,2,7,1,0,"goto :b5", 1, 2, 0);
	trans[3][7]	= settr(115,0,9,1,0,"break", 1, 2, 0);
	trans[3][9]	= settr(117,0,0,25,25,"-end-", 0, 3500, 0);

	/* proctype 2: reader */

	trans[2] = (Trans **) emalloc(30*sizeof(Trans *));

	trans[2][27]	= settr(106,0,26,1,0,".(goto)", 0, 2, 0);
	T = trans[2][26] = settr(105,0,0,0,0,"DO", 0, 2, 0);
	T = T->nxt	= settr(105,0,1,0,0,"DO", 0, 2, 0);
	    T->nxt	= settr(105,0,24,0,0,"DO", 0, 2, 0);
	trans[2][1]	= settr(80,0,12,26,0,"((((((write_off/(4/2))-(read_off/(4/2)))>0)&&(((write_off/(4/2))-(read_off/(4/2)))<(255/2)))&&(((commit_count[((read_off%4)/(4/2))]-(4/2))-(((read_off/4)*4)/2))==0)))", 1, 2, 0);
	T = trans[ 2][12] = settr(91,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(91,2,2,0,0,"ATOMIC", 1, 2, 0);
	trans[2][2]	= settr(81,2,9,27,27,"i = 0", 1, 2, 0);
	trans[2][10]	= settr(89,2,9,1,0,".(goto)", 1, 2, 0);
	T = trans[2][9] = settr(88,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(88,2,3,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(88,2,7,0,0,"DO", 1, 2, 0);
	trans[2][3]	= settr(82,2,9,28,28,"((i<(4/2)))", 1, 2, 0); /* m: 4 -> 9,0 */
	reached2[4] = 1;
	trans[2][4]	= settr(0,0,0,0,0,"assert((buffer_use[((read_off+i)%4)]==0))",0,0,0);
	trans[2][5]	= settr(0,0,0,0,0,"buffer_use[((read_off+i)%4)] = 1",0,0,0);
	trans[2][6]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[2][7]	= settr(86,2,11,29,29,"((i>=(4/2)))", 1, 2, 0); /* m: 8 -> 11,0 */
	reached2[8] = 1;
	trans[2][8]	= settr(87,2,11,1,0,"goto :b3", 1, 2, 0);
	trans[2][11]	= settr(90,0,23,1,0,"break", 1, 2, 0);
	T = trans[ 2][23] = settr(102,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(102,2,13,0,0,"ATOMIC", 1, 2, 0);
	trans[2][13]	= /* c */ settr(92,2,19,27,27,"i = 0", 1, 2, 0);
	trans[2][20]	= settr(99,2,19,1,0,".(goto)", 1, 2, 0);
	T = trans[2][19] = settr(98,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(98,2,14,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(98,2,17,0,0,"DO", 1, 2, 0);
	trans[2][14]	= settr(93,2,19,30,30,"((i<(4/2)))", 1, 2, 0); /* m: 15 -> 19,0 */
	reached2[15] = 1;
	trans[2][15]	= settr(0,0,0,0,0,"buffer_use[((read_off+i)%4)] = 0",0,0,0);
	trans[2][16]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[2][17]	= settr(96,2,21,31,31,"((i>=(4/2)))", 1, 2, 0);
	trans[2][18]	= settr(97,2,21,1,0,"goto :b4", 1, 2, 0);
	trans[2][21]	= settr(100,2,22,1,0,"break", 1, 2, 0);
	trans[2][22]	= settr(101,0,26,32,32,"read_off = (read_off+(4/2))", 1, 2, 0);
	trans[2][24]	= settr(103,0,29,33,0,"((read_off>=(4-events_lost)))", 1, 2, 0);
	trans[2][25]	= settr(104,0,29,1,0,"goto :b2", 0, 2, 0);
	trans[2][28]	= settr(107,0,29,1,0,"break", 0, 2, 0);
	trans[2][29]	= settr(108,0,0,34,34,"-end-", 0, 3500, 0);

	/* proctype 1: tracer */

	trans[1] = (Trans **) emalloc(51*sizeof(Trans *));

	T = trans[ 1][3] = settr(32,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(32,2,1,0,0,"ATOMIC", 1, 2, 0);
	trans[1][1]	= settr(30,4,10,35,35,"prev_off = write_off", 1, 2, 0); /* m: 2 -> 0,10 */
	reached1[2] = 1;
	trans[1][2]	= settr(0,0,0,0,0,"new_off = (prev_off+size)",0,0,0);
	T = trans[ 1][10] = settr(39,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(39,2,8,0,0,"ATOMIC", 1, 2, 0);
	T = trans[1][8] = settr(37,2,0,0,0,"IF", 1, 2, 0);
	T = T->nxt	= settr(37,2,4,0,0,"IF", 1, 2, 0);
	    T->nxt	= settr(37,2,6,0,0,"IF", 1, 2, 0);
	trans[1][4]	= settr(33,2,47,36,36,"((((new_off-read_off)>4)&&((new_off-read_off)<(255/2))))", 1, 2, 0);
	trans[1][5]	= settr(34,2,47,1,0,"goto lost", 1, 2, 0);
	trans[1][9]	= settr(38,0,27,1,0,".(goto)", 1, 2, 0);
	trans[1][6]	= settr(35,2,7,2,0,"else", 1, 2, 0);
	trans[1][7]	= settr(36,4,27,37,37,"(1)", 1, 2, 0); /* m: 9 -> 27,0 */
	reached1[9] = 1;
	T = trans[ 1][27] = settr(56,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(56,2,15,0,0,"ATOMIC", 1, 2, 0);
	T = trans[1][15] = settr(44,2,0,0,0,"IF", 1, 2, 0);
	T = T->nxt	= settr(44,2,11,0,0,"IF", 1, 2, 0);
	    T->nxt	= settr(44,2,13,0,0,"IF", 1, 2, 0);
	trans[1][11]	= settr(40,4,3,38,38,"((prev_off!=write_off))", 1, 2, 0); /* m: 12 -> 3,0 */
	reached1[12] = 1;
	trans[1][12]	= settr(41,0,3,1,0,"goto cmpxchg_loop", 1, 2, 0);
	trans[1][16]	= settr(45,2,17,1,0,".(goto)", 1, 2, 0); /* m: 17 -> 0,24 */
	reached1[17] = 1;
	trans[1][13]	= settr(42,2,14,2,0,"else", 1, 2, 0);
	trans[1][14]	= settr(43,2,24,39,39,"write_off = new_off", 1, 2, 0); /* m: 17 -> 0,24 */
	reached1[17] = 1;
	trans[1][17]	= settr(46,2,24,40,40,"i = 0", 1, 2, 0);
	trans[1][25]	= settr(54,2,24,1,0,".(goto)", 1, 2, 0);
	T = trans[1][24] = settr(53,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(53,2,18,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(53,2,22,0,0,"DO", 1, 2, 0);
	trans[1][18]	= settr(47,2,24,41,41,"((i<size))", 1, 2, 0); /* m: 19 -> 24,0 */
	reached1[19] = 1;
	trans[1][19]	= settr(0,0,0,0,0,"assert((buffer_use[((prev_off+i)%4)]==0))",0,0,0);
	trans[1][20]	= settr(0,0,0,0,0,"buffer_use[((prev_off+i)%4)] = 1",0,0,0);
	trans[1][21]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[1][22]	= settr(51,2,26,42,42,"((i>=size))", 1, 2, 0); /* m: 23 -> 26,0 */
	reached1[23] = 1;
	trans[1][23]	= settr(52,2,26,1,0,"goto :b0", 1, 2, 0);
	trans[1][26]	= settr(55,0,45,1,0,"break", 1, 2, 0);
	T = trans[ 1][45] = settr(74,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(74,2,28,0,0,"ATOMIC", 1, 2, 0);
	trans[1][28]	= settr(57,2,34,43,43,"i = 0", 1, 2, 0);
	trans[1][35]	= settr(64,2,34,1,0,".(goto)", 1, 2, 0);
	T = trans[1][34] = settr(63,2,0,0,0,"DO", 1, 2, 0);
	T = T->nxt	= settr(63,2,29,0,0,"DO", 1, 2, 0);
	    T->nxt	= settr(63,2,32,0,0,"DO", 1, 2, 0);
	trans[1][29]	= settr(58,2,34,44,44,"((i<size))", 1, 2, 0); /* m: 30 -> 34,0 */
	reached1[30] = 1;
	trans[1][30]	= settr(0,0,0,0,0,"buffer_use[((prev_off+i)%4)] = 0",0,0,0);
	trans[1][31]	= settr(0,0,0,0,0,"i = (i+1)",0,0,0);
	trans[1][32]	= settr(61,2,43,45,45,"((i>=size))", 1, 2, 0); /* m: 37 -> 43,0 */
	reached1[37] = 1;
	trans[1][33]	= settr(62,2,37,1,0,"goto :b1", 1, 2, 0); /* m: 37 -> 0,43 */
	reached1[37] = 1;
	trans[1][36]	= settr(65,2,37,1,0,"break", 1, 2, 0);
	trans[1][37]	= settr(66,2,43,46,46,"tmp_commit = (commit_count[((prev_off%4)/(4/2))]+size)", 1, 2, 0); /* m: 38 -> 0,43 */
	reached1[38] = 1;
	trans[1][38]	= settr(0,0,0,0,0,"commit_count[((prev_off%4)/(4/2))] = tmp_commit",0,0,0);
	T = trans[1][43] = settr(72,2,0,0,0,"IF", 1, 2, 0);
	T = T->nxt	= settr(72,2,39,0,0,"IF", 1, 2, 0);
	    T->nxt	= settr(72,2,41,0,0,"IF", 1, 2, 0);
	trans[1][39]	= settr(68,4,49,47,47,"((((((prev_off/4)*4)/2)+(4/2))-tmp_commit))", 1, 2, 0); /* m: 40 -> 49,0 */
	reached1[40] = 1;
	trans[1][40]	= settr(0,0,0,0,0,"deliver = 1",0,0,0);
	trans[1][44]	= settr(73,0,49,48,48,".(goto)", 1, 2, 0);
	trans[1][41]	= settr(70,2,42,2,0,"else", 1, 2, 0);
	trans[1][42]	= settr(71,4,49,49,49,"(1)", 1, 2, 0); /* m: 44 -> 49,0 */
	reached1[44] = 1;
	T = trans[ 1][49] = settr(78,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(78,2,46,0,0,"ATOMIC", 1, 2, 0);
	trans[1][46]	= settr(75,2,48,1,0,"goto end", 1, 2, 0);
	trans[1][47]	= settr(76,2,48,50,50,"events_lost = (events_lost+1)", 1, 2, 0);
	trans[1][48]	= settr(77,0,50,51,51,"refcount = (refcount-1)", 1, 2, 0);
	trans[1][50]	= settr(79,0,0,52,52,"-end-", 0, 3500, 0);

	/* proctype 0: switcher */

	trans[0] = (Trans **) emalloc(31*sizeof(Trans *));

	T = trans[ 0][11] = settr(10,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(10,2,1,0,0,"ATOMIC", 1, 2, 0);
	trans[0][1]	= settr(0,2,9,53,53,"prev_off = write_off", 1, 2, 0); /* m: 2 -> 0,9 */
	reached0[2] = 1;
	trans[0][2]	= settr(0,0,0,0,0,"size = ((4/2)-(prev_off%(4/2)))",0,0,0);
	trans[0][3]	= settr(0,0,0,0,0,"new_off = (prev_off+size)",0,0,0);
	T = trans[0][9] = settr(8,2,0,0,0,"IF", 1, 2, 0);
	T = T->nxt	= settr(8,2,4,0,0,"IF", 1, 2, 0);
	    T->nxt	= settr(8,2,7,0,0,"IF", 1, 2, 0);
	trans[0][4]	= settr(3,4,29,54,54,"(((((new_off-read_off)>4)&&((new_off-read_off)<(255/2)))||(size==(4/2))))", 1, 2, 0); /* m: 5 -> 29,0 */
	reached0[5] = 1;
	trans[0][5]	= settr(0,0,0,0,0,"refcount = (refcount-1)",0,0,0);
	trans[0][6]	= settr(5,0,29,1,0,"goto not_needed", 1, 2, 0);
	trans[0][10]	= settr(9,0,18,1,0,".(goto)", 1, 2, 0);
	trans[0][7]	= settr(6,2,8,2,0,"else", 1, 2, 0);
	trans[0][8]	= settr(7,4,18,55,55,"(1)", 1, 2, 0); /* m: 10 -> 18,0 */
	reached0[10] = 1;
	T = trans[ 0][18] = settr(17,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(17,2,16,0,0,"ATOMIC", 1, 2, 0);
	T = trans[0][16] = settr(15,2,0,0,0,"IF", 1, 2, 0);
	T = T->nxt	= settr(15,2,12,0,0,"IF", 1, 2, 0);
	    T->nxt	= settr(15,2,14,0,0,"IF", 1, 2, 0);
	trans[0][12]	= settr(11,4,11,56,56,"((prev_off!=write_off))", 1, 2, 0); /* m: 13 -> 11,0 */
	reached0[13] = 1;
	trans[0][13]	= settr(12,0,11,1,0,"goto cmpxchg_loop", 1, 2, 0);
	trans[0][17]	= settr(16,0,28,57,57,".(goto)", 1, 2, 0);
	trans[0][14]	= settr(13,2,15,2,0,"else", 1, 2, 0);
	trans[0][15]	= settr(14,4,28,58,58,"write_off = new_off", 1, 2, 0); /* m: 17 -> 0,28 */
	reached0[17] = 1;
	T = trans[ 0][28] = settr(27,2,0,0,0,"ATOMIC", 1, 2, 0);
	T->nxt	= settr(27,2,19,0,0,"ATOMIC", 1, 2, 0);
	trans[0][19]	= settr(18,2,25,59,59,"tmp_commit = (commit_count[((prev_off%4)/(4/2))]+size)", 1, 2, 0); /* m: 20 -> 0,25 */
	reached0[20] = 1;
	trans[0][20]	= settr(0,0,0,0,0,"commit_count[((prev_off%4)/(4/2))] = tmp_commit",0,0,0);
	T = trans[0][25] = settr(24,2,0,0,0,"IF", 1, 2, 0);
	T = T->nxt	= settr(24,2,21,0,0,"IF", 1, 2, 0);
	    T->nxt	= settr(24,2,23,0,0,"IF", 1, 2, 0);
	trans[0][21]	= settr(20,4,29,60,60,"((((((prev_off/4)*4)/2)+(4/2))-tmp_commit))", 1, 2, 0); /* m: 22 -> 29,0 */
	reached0[22] = 1;
	trans[0][22]	= settr(0,0,0,0,0,"deliver = 1",0,0,0);
	trans[0][26]	= settr(25,4,29,61,61,".(goto)", 1, 2, 0); /* m: 27 -> 0,29 */
	reached0[27] = 1;
	trans[0][23]	= settr(22,2,24,2,0,"else", 1, 2, 0);
	trans[0][24]	= settr(23,4,29,62,62,"(1)", 1, 2, 0); /* m: 26 -> 29,0 */
	reached0[26] = 1;
	trans[0][27]	= settr(0,0,0,0,0,"refcount = (refcount-1)",0,0,0);
	trans[0][29]	= settr(28,0,30,1,0,"(1)", 0, 2, 0);
	trans[0][30]	= settr(29,0,0,63,63,"-end-", 0, 3500, 0);
	/* np_ demon: */
	trans[_NP_] = (Trans **) emalloc(2*sizeof(Trans *));
	T = trans[_NP_][0] = settr(9997,0,1,_T5,0,"(np_)", 1,2,0);
	    T->nxt	  = settr(9998,0,0,_T2,0,"(1)",   0,2,0);
	T = trans[_NP_][1] = settr(9999,0,1,_T5,0,"(np_)", 1,2,0);
}

Trans *
settr(	int t_id, int a, int b, int c, int d,
	char *t, int g, int tpe0, int tpe1)
{	Trans *tmp = (Trans *) emalloc(sizeof(Trans));

	tmp->atom  = a&(6|32);	/* only (2|8|32) have meaning */
	if (!g) tmp->atom |= 8;	/* no global references */
	tmp->st    = b;
	tmp->tpe[0] = tpe0;
	tmp->tpe[1] = tpe1;
	tmp->tp    = t;
	tmp->t_id  = t_id;
	tmp->forw  = c;
	tmp->back  = d;
	return tmp;
}

Trans *
cpytr(Trans *a)
{	Trans *tmp = (Trans *) emalloc(sizeof(Trans));

	int i;
	tmp->atom  = a->atom;
	tmp->st    = a->st;
#ifdef HAS_UNLESS
	tmp->e_trans = a->e_trans;
	for (i = 0; i < HAS_UNLESS; i++)
		tmp->escp[i] = a->escp[i];
#endif
	tmp->tpe[0] = a->tpe[0];
	tmp->tpe[1] = a->tpe[1];
	for (i = 0; i < 6; i++)
	{	tmp->qu[i] = a->qu[i];
		tmp->ty[i] = a->ty[i];
	}
	tmp->tp    = (char *) emalloc(strlen(a->tp)+1);
	strcpy(tmp->tp, a->tp);
	tmp->t_id  = a->t_id;
	tmp->forw  = a->forw;
	tmp->back  = a->back;
	return tmp;
}

#ifndef NOREDUCE
int
srinc_set(int n)
{	if (n <= 2) return LOCAL;
	if (n <= 2+  DELTA) return Q_FULL_F; /* 's' or nfull  */
	if (n <= 2+2*DELTA) return Q_EMPT_F; /* 'r' or nempty */
	if (n <= 2+3*DELTA) return Q_EMPT_T; /* empty */
	if (n <= 2+4*DELTA) return Q_FULL_T; /* full  */
	if (n ==   5*DELTA) return GLOBAL;
	if (n ==   6*DELTA) return TIMEOUT_F;
	if (n ==   7*DELTA) return ALPHA_F;
	Uerror("cannot happen srinc_class");
	return BAD;
}
int
srunc(int n, int m)
{	switch(m) {
	case Q_FULL_F: return n-2;
	case Q_EMPT_F: return n-2-DELTA;
	case Q_EMPT_T: return n-2-2*DELTA;
	case Q_FULL_T: return n-2-3*DELTA;
	case ALPHA_F:
	case TIMEOUT_F: return 257; /* non-zero, and > MAXQ */
	}
	Uerror("cannot happen srunc");
	return 0;
}
#endif
int cnt;
#ifdef HAS_UNLESS
int
isthere(Trans *a, int b)
{	Trans *t;
	for (t = a; t; t = t->nxt)
		if (t->t_id == b)
			return 1;
	return 0;
}
#endif
#ifndef NOREDUCE
int
mark_safety(Trans *t) /* for conditional safety */
{	int g = 0, i, j, k;

	if (!t) return 0;
	if (t->qu[0])
		return (t->qu[1])?2:1;	/* marked */

	for (i = 0; i < 2; i++)
	{	j = srinc_set(t->tpe[i]);
		if (j >= GLOBAL && j != ALPHA_F)
			return -1;
		if (j != LOCAL)
		{	k = srunc(t->tpe[i], j);
			if (g == 0
			||  t->qu[0] != k
			||  t->ty[0] != j)
			{	t->qu[g] = k;
				t->ty[g] = j;
				g++;
	}	}	}
	return g;
}
#endif
void
retrans(int n, int m, int is, short srcln[], uchar reach[], uchar lstate[])
	/* process n, with m states, is=initial state */
{	Trans *T0, *T1, *T2, *T3;
	int i, k;
#ifndef NOREDUCE
	int g, h, j, aa;
#endif
#ifdef HAS_UNLESS
	int p;
#endif
	if (state_tables >= 4)
	{	printf("STEP 1 proctype %s\n", 
			procname[n]);
		for (i = 1; i < m; i++)
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
			crack(n, i, T0, srcln);
		return;
	}
	do {
		for (i = 1, cnt = 0; i < m; i++)
		{	T2 = trans[n][i];
			T1 = T2?T2->nxt:(Trans *)0;
/* prescan: */		for (T0 = T1; T0; T0 = T0->nxt)
/* choice in choice */	{	if (T0->st && trans[n][T0->st]
				&&  trans[n][T0->st]->nxt)
					break;
			}
#if 0
		if (T0)
		printf("\tstate %d / %d: choice in choice\n",
		i, T0->st);
#endif
			if (T0)
			for (T0 = T1; T0; T0 = T0->nxt)
			{	T3 = trans[n][T0->st];
				if (!T3->nxt)
				{	T2->nxt = cpytr(T0);
					T2 = T2->nxt;
					imed(T2, T0->st, n, i);
					continue;
				}
				do {	T3 = T3->nxt;
					T2->nxt = cpytr(T3);
					T2 = T2->nxt;
					imed(T2, T0->st, n, i);
				} while (T3->nxt);
				cnt++;
			}
		}
	} while (cnt);
	if (state_tables >= 3)
	{	printf("STEP 2 proctype %s\n", 
			procname[n]);
		for (i = 1; i < m; i++)
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
			crack(n, i, T0, srcln);
		return;
	}
	for (i = 1; i < m; i++)
	{	if (trans[n][i] && trans[n][i]->nxt) /* optimize */
		{	T1 = trans[n][i]->nxt;
#if 0
			printf("\t\tpull %d (%d) to %d\n",
			T1->st, T1->forw, i);
#endif
			if (!trans[n][T1->st]) continue;
			T0 = cpytr(trans[n][T1->st]);
			trans[n][i] = T0;
			reach[T1->st] = 1;
			imed(T0, T1->st, n, i);
			for (T1 = T1->nxt; T1; T1 = T1->nxt)
			{
#if 0
			printf("\t\tpull %d (%d) to %d\n",
				T1->st, T1->forw, i);
#endif
				if (!trans[n][T1->st]) continue;
				T0->nxt = cpytr(trans[n][T1->st]);
				T0 = T0->nxt;
				reach[T1->st] = 1;
				imed(T0, T1->st, n, i);
	}	}	}
	if (state_tables >= 2)
	{	printf("STEP 3 proctype %s\n", 
			procname[n]);
		for (i = 1; i < m; i++)
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
			crack(n, i, T0, srcln);
		return;
	}
#ifdef HAS_UNLESS
	for (i = 1; i < m; i++)
	{	if (!trans[n][i]) continue;
		/* check for each state i if an
		 * escape to some state p is defined
		 * if so, copy and mark p's transitions
		 * and prepend them to the transition-
		 * list of state i
		 */
	 if (!like_java) /* the default */
	 {	for (T0 = trans[n][i]; T0; T0 = T0->nxt)
		for (k = HAS_UNLESS-1; k >= 0; k--)
		{	if (p = T0->escp[k])
			for (T1 = trans[n][p]; T1; T1 = T1->nxt)
			{	if (isthere(trans[n][i], T1->t_id))
					continue;
				T2 = cpytr(T1);
				T2->e_trans = p;
				T2->nxt = trans[n][i];
				trans[n][i] = T2;
		}	}
	 } else /* outermost unless checked first */
	 {	Trans *T4;
		T4 = T3 = (Trans *) 0;
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
		for (k = HAS_UNLESS-1; k >= 0; k--)
		{	if (p = T0->escp[k])
			for (T1 = trans[n][p]; T1; T1 = T1->nxt)
			{	if (isthere(trans[n][i], T1->t_id))
					continue;
				T2 = cpytr(T1);
				T2->nxt = (Trans *) 0;
				T2->e_trans = p;
				if (T3)	T3->nxt = T2;
				else	T4 = T2;
				T3 = T2;
		}	}
		if (T4)
		{	T3->nxt = trans[n][i];
			trans[n][i] = T4;
		}
	 }
	}
#endif
#ifndef NOREDUCE
	for (i = 1; i < m; i++)
	{	if (a_cycles)
		{ /* moves through these states are visible */
	#if PROG_LAB>0 && defined(HAS_NP)
			if (progstate[n][i])
				goto degrade;
			for (T1 = trans[n][i]; T1; T1 = T1->nxt)
				if (progstate[n][T1->st])
					goto degrade;
	#endif
			if (accpstate[n][i] || visstate[n][i])
				goto degrade;
			for (T1 = trans[n][i]; T1; T1 = T1->nxt)
				if (accpstate[n][T1->st])
					goto degrade;
		}
		T1 = trans[n][i];
		if (!T1) continue;
		g = mark_safety(T1);	/* V3.3.1 */
		if (g < 0) goto degrade; /* global */
		/* check if mixing of guards preserves reduction */
		if (T1->nxt)
		{	k = 0;
			for (T0 = T1; T0; T0 = T0->nxt)
			{	if (!(T0->atom&8))
					goto degrade;
				for (aa = 0; aa < 2; aa++)
				{	j = srinc_set(T0->tpe[aa]);
					if (j >= GLOBAL && j != ALPHA_F)
						goto degrade;
					if (T0->tpe[aa]
					&&  T0->tpe[aa]
					!=  T1->tpe[0])
						k = 1;
			}	}
			/* g = 0;	V3.3.1 */
			if (k)	/* non-uniform selection */
			for (T0 = T1; T0; T0 = T0->nxt)
			for (aa = 0; aa < 2; aa++)
			{	j = srinc_set(T0->tpe[aa]);
				if (j != LOCAL)
				{	k = srunc(T0->tpe[aa], j);
					for (h = 0; h < 6; h++)
						if (T1->qu[h] == k
						&&  T1->ty[h] == j)
							break;
					if (h >= 6)
					{	T1->qu[g%6] = k;
						T1->ty[g%6] = j;
						g++;
			}	}	}
			if (g > 6)
			{	T1->qu[0] = 0;	/* turn it off */
				printf("pan: warning, line %d, ",
					srcln[i]);
			 	printf("too many stmnt types (%d)",
					g);
			  	printf(" in selection\n");
			  goto degrade;
			}
		}
		/* mark all options global if >=1 is global */
		for (T1 = trans[n][i]; T1; T1 = T1->nxt)
			if (!(T1->atom&8)) break;
		if (T1)
degrade:	for (T1 = trans[n][i]; T1; T1 = T1->nxt)
			T1->atom &= ~8;	/* mark as unsafe */
		/* can only mix 'r's or 's's if on same chan */
		/* and not mixed with other local operations */
		T1 = trans[n][i];
		if (!T1 || T1->qu[0]) continue;
		j = T1->tpe[0];
		if (T1->nxt && T1->atom&8)
		{ if (j == 5*DELTA)
		  {	printf("warning: line %d ", srcln[i]);
			printf("mixed condition ");
			printf("(defeats reduction)\n");
			goto degrade;
		  }
		  for (T0 = T1; T0; T0 = T0->nxt)
		  for (aa = 0; aa < 2; aa++)
		  if  (T0->tpe[aa] && T0->tpe[aa] != j)
		  {	printf("warning: line %d ", srcln[i]);
			printf("[%d-%d] mixed %stion ",
				T0->tpe[aa], j, 
				(j==5*DELTA)?"condi":"selec");
			printf("(defeats reduction)\n");
			printf("	'%s' <-> '%s'\n",
				T1->tp, T0->tp);
			goto degrade;
		} }
	}
#endif
	for (i = 1; i < m; i++)
	{	T2 = trans[n][i];
		if (!T2
		||  T2->nxt
		||  strncmp(T2->tp, ".(goto)", 7)
		||  !stopstate[n][i])
			continue;
		stopstate[n][T2->st] = 1;
	}
	if (state_tables)
	{	printf("proctype ");
		if (!strcmp(procname[n], ":init:"))
			printf("init\n");
		else
			printf("%s\n", procname[n]);
		for (i = 1; i < m; i++)
			reach[i] = 1;
		tagtable(n, m, is, srcln, reach);
	} else
	for (i = 1; i < m; i++)
	{	int nrelse;
		if (strcmp(procname[n], ":never:") != 0)
		{	for (T0 = trans[n][i]; T0; T0 = T0->nxt)
			{	if (T0->st == i
				&& strcmp(T0->tp, "(1)") == 0)
				{	printf("error: proctype '%s' ",
						procname[n]);
		  			printf("line %d, state %d: has un",
						srcln[i], i);
					printf("conditional self-loop\n");
					pan_exit(1);
		}	}	}
		nrelse = 0;
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
		{	if (strcmp(T0->tp, "else") == 0)
				nrelse++;
		}
		if (nrelse > 1)
		{	printf("error: proctype '%s' state",
				procname[n]);
		  	printf(" %d, inherits %d", i, nrelse);
		  	printf(" 'else' stmnts\n");
			pan_exit(1);
	}	}
	if (!state_tables && strcmp(procname[n], ":never:") == 0)
	{	int h = 0;
		for (i = 1; i < m; i++)
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
			if (T0->forw > h) h = T0->forw;
		h++;
		frm_st0 = (short *) emalloc(h * sizeof(short));
		for (i = 1; i < m; i++)
		for (T0 = trans[n][i]; T0; T0 = T0->nxt)
			frm_st0[T0->forw] = i;
	}
#ifndef LOOPSTATE
	if (state_tables)
#endif
	do_dfs(n, m, is, srcln, reach, lstate);
#ifdef T_REVERSE
	/* process n, with m states, is=initial state -- reverse list */
	if (!state_tables && strcmp(procname[n], ":never:") != 0)
	{	for (i = 1; i < m; i++)
		{	Trans *T4 = (Trans *) 0;
			T1 = (Trans *) 0;
			T2 = (Trans *) 0;
			T3 = (Trans *) 0;
			for (T0 = trans[n][i]; T0; T0 = T4)
			{	T4 = T0->nxt;
				if (strcmp(T0->tp, "else") == 0)
				{	T3 = T0;
					T0->nxt = (Trans *) 0;
				} else
				{	T0->nxt = T1;
					if (!T1) { T2 = T0; }
					T1 = T0;
			}	}
			if (T2 && T3) { T2->nxt = T3; }
			trans[n][i] = T1; /* reversed -- else at end */
	}	}
#endif
}
void
imed(Trans *T, int v, int n, int j)	/* set intermediate state */
{	progstate[n][T->st] |= progstate[n][v];
	accpstate[n][T->st] |= accpstate[n][v];
	stopstate[n][T->st] |= stopstate[n][v];
	mapstate[n][j] = T->st;
}
void
tagtable(int n, int m, int is, short srcln[], uchar reach[])
{	Trans *z;

	if (is >= m || !trans[n][is]
	||  is <= 0 || reach[is] == 0)
		return;
	reach[is] = 0;
	if (state_tables)
	for (z = trans[n][is]; z; z = z->nxt)
		crack(n, is, z, srcln);
	for (z = trans[n][is]; z; z = z->nxt)
	{
#ifdef HAS_UNLESS
		int i, j;
#endif
		tagtable(n, m, z->st, srcln, reach);
#ifdef HAS_UNLESS
		for (i = 0; i < HAS_UNLESS; i++)
		{	j = trans[n][is]->escp[i];
			if (!j) break;
			tagtable(n, m, j, srcln, reach);
		}
#endif
	}
}
void
dfs_table(int n, int m, int is, short srcln[], uchar reach[], uchar lstate[])
{	Trans *z;

	if (is >= m || is <= 0 || !trans[n][is])
		return;
	if ((reach[is] & (4|8|16)) != 0)
	{	if ((reach[is] & (8|16)) == 16)	/* on stack, not yet recorded */
		{	lstate[is] = 1;
			reach[is] |= 8; /* recorded */
			if (state_tables)
			{	printf("state %d line %d is a loopstate\n", is, srcln[is]);
		}	}
		return;
	}
	reach[is] |= (4|16);	/* visited | onstack */
	for (z = trans[n][is]; z; z = z->nxt)
	{
#ifdef HAS_UNLESS
		int i, j;
#endif
		dfs_table(n, m, z->st, srcln, reach, lstate);
#ifdef HAS_UNLESS
		for (i = 0; i < HAS_UNLESS; i++)
		{	j = trans[n][is]->escp[i];
			if (!j) break;
			dfs_table(n, m, j, srcln, reach, lstate);
		}
#endif
	}
	reach[is] &= ~16; /* no longer on stack */
}
void
do_dfs(int n, int m, int is, short srcln[], uchar reach[], uchar lstate[])
{	int i;
	dfs_table(n, m, is, srcln, reach, lstate);
	for (i = 0; i < m; i++)
		reach[i] &= ~(4|8|16);
}
void
crack(int n, int j, Trans *z, short srcln[])
{	int i;

	if (!z) return;
	printf("	state %3d -(tr %3d)-> state %3d  ",
		j, z->forw, z->st);
	printf("[id %3d tp %3d", z->t_id, z->tpe[0]);
	if (z->tpe[1]) printf(",%d", z->tpe[1]);
#ifdef HAS_UNLESS
	if (z->e_trans)
		printf(" org %3d", z->e_trans);
	else if (state_tables >= 2)
	for (i = 0; i < HAS_UNLESS; i++)
	{	if (!z->escp[i]) break;
		printf(" esc %d", z->escp[i]);
	}
#endif
	printf("]");
	printf(" [%s%s%s%s%s] line %d => ",
		z->atom&6?"A":z->atom&32?"D":"-",
		accpstate[n][j]?"a" :"-",
		stopstate[n][j]?"e" : "-",
		progstate[n][j]?"p" : "-",
		z->atom & 8 ?"L":"G",
		srcln[j]);
	for (i = 0; z->tp[i]; i++)
		if (z->tp[i] == '\n')
			printf("\\n");
		else
			putchar(z->tp[i]);
	if (z->qu[0])
	{	printf("\t[");
		for (i = 0; i < 6; i++)
			if (z->qu[i])
				printf("(%d,%d)",
				z->qu[i], z->ty[i]);
		printf("]");
	}
	printf("\n");
	fflush(stdout);
}

#ifdef VAR_RANGES
#define BYTESIZE	32	/* 2^8 : 2^3 = 256:8 = 32 */

typedef struct Vr_Ptr {
	char	*nm;
	uchar	vals[BYTESIZE];
	struct Vr_Ptr *nxt;
} Vr_Ptr;
Vr_Ptr *ranges = (Vr_Ptr *) 0;

void
logval(char *s, int v)
{	Vr_Ptr *tmp;

	if (v<0 || v > 255) return;
	for (tmp = ranges; tmp; tmp = tmp->nxt)
		if (!strcmp(tmp->nm, s))
			goto found;
	tmp = (Vr_Ptr *) emalloc(sizeof(Vr_Ptr));
	tmp->nxt = ranges;
	ranges = tmp;
	tmp->nm = s;
found:
	tmp->vals[(v)/8] |= 1<<((v)%8);
}

void
dumpval(uchar X[], int range)
{	int w, x, i, j = -1;

	for (w = i = 0; w < range; w++)
	for (x = 0; x < 8; x++, i++)
	{
from:		if ((X[w] & (1<<x)))
		{	printf("%d", i);
			j = i;
			goto upto;
	}	}
	return;
	for (w = 0; w < range; w++)
	for (x = 0; x < 8; x++, i++)
	{
upto:		if (!(X[w] & (1<<x)))
		{	if (i-1 == j)
				printf(", ");
			else
				printf("-%d, ", i-1);
			goto from;
	}	}
	if (j >= 0 && j != 255)
		printf("-255");
}

void
dumpranges(void)
{	Vr_Ptr *tmp;
	printf("\nValues assigned within ");
	printf("interval [0..255]:\n");
	for (tmp = ranges; tmp; tmp = tmp->nxt)
	{	printf("\t%s\t: ", tmp->nm);
		dumpval(tmp->vals, BYTESIZE);
		printf("\n");
	}
}
#endif
