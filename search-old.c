/*
 *	SEARCH.C
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */


#include <stdio.h>
#include <string.h>

#ifdef EZ
#include "EZ.h"
extern EZ_Widget *pvbox;
#endif

#include "defs.h"
#include "data.h"
#include "protos.h"


/* see the beginning of think() */
#include <setjmp.h>
jmp_buf env;
BOOL stop_search;


/* think() calls search() iteratively. If quiet is FALSE,
   it prints the analysis after every iteration. */

void think(BOOL quiet)
{
	int i, j, x;
	char pvariant[256];
	/* some code that lets us longjmp back here and return
	   from think() when our time is up */
	stop_search = FALSE;
	setjmp(env);
	if (stop_search) {
		
		/* make sure to take back the line we were searching */
		while (ply)
			takeback();
		return;
	}

	start_time = get_ms();
	stop_time = start_time + max_time;

	ply = 0;
	nodes = 0;
	memset(pv, 0, sizeof(pv));
	memset(history, 0, sizeof(history));
	if (!quiet)
#ifdef EZ
	  EZ_AppendListBoxItem(pvbox,"ply      nodes       score  pv");
#else
	  printf("ply      nodes       score  pv\n");
#endif
	for (i = 1; i <= max_depth; ++i) {
		follow_pv = TRUE;
		x = search(-1000000, 1000000, i);
		if (!quiet) {
			sprintf(pvariant,"%3d  %9ld  %9ld", i, nodes, x);
			for (j = 0; j < pv_length[0]; ++j)
				sprintf(pvariant," %s", half_move_str(pv[0][j].b));
#ifdef EZ
			EZ_AppendListBoxItem(pvbox,pvariant);
#else
			printf("%s\n",pvariant);
#endif
		}
	}
}


/* search() does just that, in negamax fashion */

int search(int alpha, int beta, int depth)
{
	int i, j, x;
	BOOL c, f;

	/* we're as deep as we want to be; call quiesce() to get
	   a reasonable score and return it. For Tenjiku, you can
	almost always capture s.th. so don't use quiesce() */
	if (depth <= 0 )
	  /* return quiesce(alpha,beta, 0); */
	  return eval();
	++nodes;

	/* do some housekeeping every 1024 nodes */
	if ((nodes & 16) == 0) { /* was: (nodes & 1023) */
#ifdef EZ
	  EZ_ServiceEvents();
#endif
		checkup();
	}
	pv_length[ply] = ply;

	/* are we too deep? */
	if (ply >= HIST_STACK - 1)
		return eval();
	
#ifdef MOVELOG
	fprintf(stderr,"#Search depth %d\n",ply);
	/* print_board(stderr); */
#endif
	/* are we in check? if so, we want to search deeper */
	c = in_check(side);
	if (c)
		++depth;
	gen();
	if (follow_pv)  /* are we following the PV? */
		sort_pv();
	f = FALSE;

	/* loop through the moves */
	for (i = gen_begin[ply]; i < gen_end[ply]; ++i) {
	  sort(i);
	  if (!makemove(gen_dat[i].m.b)) {
#ifdef MOVELOG
	    print_board(stderr);
	    fprintf(stderr,"#%d: %s failed making move %s\n", depth,
		    (side ? "Sente" : "Gote"), move_str(gen_dat[i].m.b)); 

#endif
	    continue;
	  }
#ifdef MOVELOG
	  else {
	    fprintf(stderr,"#%d: %s making move %s\n%s\n", depth,
		    (side ? "Sente" : "Gote"), move_str(gen_dat[i].m.b),
		    half_move_str(gen_dat[i].m.b));
	  }
#endif

	  f = TRUE;
	  x = -search(-beta, -alpha, depth - 1);
	  takeback();
#ifdef MOVELOG
	  fprintf(stderr,"#%d: %s retracting move %s\nundo\n", depth,
		  (side ? "Gote" : "Sente"), move_str(gen_dat[i].m.b));
#endif
	  if (x > alpha) {
	    /* fprintf(stderr,"#search: %d %d %d\n",i,(int)gen_dat[i].m.b.from,(int)gen_dat[i].m.b.to);
	     */
	    /* this move caused a cutoff, so increase the history
	       value so it gets ordered high next time we can
	       search it */
	    history[(int)gen_dat[i].m.b.from][(int)gen_dat[i].m.b.to] += depth;
	    if (x >= beta)
	      return beta;
	    alpha = x;
	    
	    /* update the PV */
	    pv[ply][ply] = gen_dat[i].m;
	    for (j = ply + 1; j < pv_length[ply + 1]; ++j)
	      pv[ply][j] = pv[ply + 1][j];
	    pv_length[ply] = pv_length[ply + 1];
	  }
	}
#ifdef STACKLOG
	fprintf(stderr,"#ply: %d, i: %d, gen_begin[ply]: %d, gen_end[ply]: %d\n",
		ply, i, gen_begin[ply], gen_end[ply]);
#endif

	/* no legal moves? then we're in checkmate or stalemate */
	if (!f) {
	  return -1000000 + ply;
	}
	
	return alpha;
}



/* quiesce() is a recursive minimax search function with
   alpha-beta cutoffs. In other words, negamax. It basically
   only searches capture sequences and allows the evaluation
   function to cut the search off (and set alpha). The idea
   is to find a position where there isn't a lot going on
   so the static evaluation function will work. */

int quiesce(int alpha,int beta, int depth)
{
	int i, j, x;

	++nodes;

	/* do some housekeeping every 1024 nodes */
	if ((nodes % 500) == 0)
		checkup();

	pv_length[ply] = ply;

	/* are we too deep? */
	if (ply >= HIST_STACK - 1)
		return eval();

	/* check with the evaluation function */
	x = eval();
	if (x >= beta)
		return beta;
	if (x > alpha)
		alpha = x;

	gen_caps();
	if (follow_pv)  /* are we following the PV? */
		sort_pv();

	/* loop through the moves */
	for (i = gen_begin[ply]; i < gen_end[ply]; ++i) {
	  /* fprintf(stderr,"#%d moves to search\n", gen_end[ply]); */
		sort(i);
		if (!makemove(gen_dat[i].m.b)) {
#ifdef LOG
		  print_board(stderr);
		  computer_side = EMPTY;
		  fprintf(stderr,"#q%d: %s failed making move %s\n",depth,
			  (side ? "Sente" : "Gote"), move_str(gen_dat[i].m.b));
		  return 0;
#endif
			continue;
		} 
#ifdef LOG
		else {
		  fprintf(stderr,"#q%d: %s making move %s\n%s\n", depth,
			(side ? "Sente" : "Gote"), move_str(gen_dat[i].m.b), 
			half_move_str(gen_dat[i].m.b));
		}
#endif

		x = -quiesce(-beta, -alpha, depth+1);
		takeback();
#ifdef LOG
		fprintf(stderr,"#q%d: %s retracting move %s\nundo\n", depth,
			(side ? "Gote" : "Sente"), move_str(gen_dat[i].m.b));
#endif
		if (x > alpha) {
			if (x >= beta)
				return beta;
			alpha = x;

			/* update the PV */
			pv[ply][ply] = gen_dat[i].m;
			for (j = ply + 1; j < pv_length[ply + 1]; ++j)
				pv[ply][j] = pv[ply + 1][j];
			pv_length[ply] = pv_length[ply + 1];
		}
	}
	return alpha;
}


/* sort_pv() is called when the search function is following
   the PV (Principal Variation). It looks through the current
   ply's move list to see if the PV move is there. If so,
   it adds 10,000,000 to the move's score so it's played first
   by the search function. If not, follow_pv remains FALSE and
   search() stops calling sort_pv(). */

void sort_pv()
{
	int i;

	follow_pv = FALSE;
	for(i = gen_begin[ply]; i < gen_end[ply]; ++i)
		if (gen_dat[i].m.u == pv[0][ply].u) {
			follow_pv = TRUE;
			gen_dat[i].score += 10000000;
			return;
		}
}


/* sort() searches the current ply's move list from 'from'
   to the end to find the move with the highest score. Then it
   swaps that move and the 'from' move so the move with the
   highest score gets searched next, and hopefully produces
   a cutoff. */

void sort(int from)
{
	int i;
	long bs;  /* best score */
	int bi;  /* best i */
	gen_t g;

	bs = -1;
	bi = from;
	for (i = from; i < gen_end[ply]; ++i)
		if (gen_dat[i].score > bs) {
			bs = gen_dat[i].score;
			bi = i;
		}
	g = gen_dat[from];
	gen_dat[from] = gen_dat[bi];
	gen_dat[bi] = g;
}


/* checkup() is called once in a while during the search. */

void checkup()
{
	/* is the engine's time up? if so, longjmp back to the
	   beginning of think() */
#ifdef EZ
  if (move_now || (get_ms() >= stop_time)) { 
    move_now = FALSE;
#else
  if ( kbhit() || (get_ms() >= stop_time)) {
#endif
    stop_search = TRUE;
    longjmp(env, 1);
  }
}
