/*
 *	BOARD.C
 *      Tenjiku Shogi by E.Werner 
 *      
 *      based on
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *	Copyright 1997 Tom Kerrigan
 *      
 */
/*
#define DEBUG
*/
#define VALUABLE_CAPTURES
#define CAPTURE_THRESHOLD  300


/*
  gen_caps() and friends should generate only such captures
  in which a piece captures a piece (or pieces) more valuable
  than a certain value and promotions to FiD.
  CAPTURE_THRESHOLD shouldn't be a const, however, but recalculated
  according to the pieces left in the game.
*/


#include "defs.h"
#include "data.h"
#include "protos.h"
#include <stdio.h>

#ifdef EZ
#include "EZ.h"
extern EZ_Item *piece_item[NUMSQUARES];
extern EZ_Widget *board;
#endif

int area_two[24]; /* keep track of generated moves */
int area_three[48];
int area_four[81];

/* init() sets the board to the initial game state */

void init()
{
	int i;
#ifdef EZ
	EZ_FreezeWidget(board);
#endif
	for (i = 0; i < NUMSQUARES; ++i) {
		color[i] = init_boardcolor[i];
		piece[i] = init_piece[i];
#ifdef EZ
		/* fprintf(stderr,"%d\n",i); */
		if ( piece_item[i] )
		  EZ_WorkAreaDeleteItem(board,piece_item[i]);
		piece_item[i] = CreatePieceItem(color[i],piece[i],
						GetXPixels(i),GetYPixels(i));
		if ( piece_item[i] ) {
		  EZ_WorkAreaInsertItem(board, piece_item[i]);
		  EZ_ConfigureItem(piece_item[i],NULL);
		}
#endif
	}
#ifdef EZ
	EZ_UnFreezeWidget(board);
#endif	
	side = LIGHT;
	xside = DARK;
	last_capture = 0;
	ply = 0;
	gen_begin[0] = 0;
	undos = 0;
	redos = 0;
	loaded_position = FALSE;
	for ( i = 0; i < PIECE_TYPES; i++ ) {
	  captured[side][i] = 0;
	  captured[xside][i] = 0;
	}
}


BOOL lost(int s)
{
	int i;
	BOOL cp, king;

	cp = FALSE; /* no cp */
	king =  FALSE; /* no king */

	for (i = 0; i < NUMSQUARES; ++i) {
	  if ( color[i] == s ) {
	    if ( piece[i] == CROWN_PRINCE ) {
	      cp = TRUE;
	    }
	    if ( piece[i] == KING ) {
	      king = TRUE;
	    }
	  }
	}
	if ( king || cp ) 
	  return FALSE;
	return TRUE; 
}


BOOL burning(int where, int sq ) {
  /* 
     sq is a (checked) square on the board.
     return TRUE if a FiD would burn an enemy
     piece on going there.
  */

  int i,n;
  if ( piece[where] != FIRE_DEMON ) return FALSE;
  if ( !hodges_FiD && suicide(sq)) return FALSE;
  if (verytame_FiD && ( color[where] != EMPTY )) return FALSE;
  if (tame_FiD && ( color[where] != EMPTY )) return FALSE;
  for (i = 0; i < 8; i++) {
    n = mailbox[mailbox256[sq] + lion_single_steps[i]];
    if ((n > -1 )&& (color[n] == xside)) return TRUE;
    if ( burn_all_FiD && (n > -1)) return TRUE;
  }
  return FALSE;
}

int eval_burned(int where, int sq ) {
  /* 
     sq is a (checked) square on the board.
     return the cumulated value of burned pieces
     if a FiD would burn be going there.
  */
  int cum = 0;
  int i,n;
  if ( piece[where] != FIRE_DEMON ) return 0;
  if ( jgs_tsa && suicide(sq)) return piece_value[piece[sq]];
  for (i = 0; i < 8; i++) {
    n = mailbox[mailbox256[sq] + lion_single_steps[i]];
    if ((n > -1 )&& (color[n] == xside)) cum += piece_value[piece[n]];
  }
  return cum;
}

#ifdef EVAL_INFLUENCE

void burn_infl(int where, int sq ) {
  /* 
     sq is a (checked) square on the board.
     if a FiD would burn be going there.
  */

  int i,n;
  if ( piece[where] != FIRE_DEMON ) return;
  if ( jgs_tsa && suicide(sq)) return;
  for (i = 0; i < 8; i++) {
    n = mailbox[mailbox256[sq] + lion_single_steps[i]];
    if (n != -1 ) influences[side][n]++;
  }
}

#endif

BOOL check_square(int sq, int *array, int array_size ) {
  int i;
  /* push only on stack if smaller depth */
  for (i = 0; i < array_size; i++ ) {
    if( sq == array[i]) 
      return FALSE; /* is already there */ 
    if( array[i] == -1 ) { /* needn't search further */
      array[i] = sq;
      return TRUE;
    }
  }
  return TRUE; /* shouldn't get here */
}


void check_area(int i, int j, int s, int *area, int array_size, int depth) {
  /* i is the old start of the step, j is the step between (square we're standing on)*/
  int n, n2;
  if ( depth <= 0 ) return;
  for ( n = 0; n < 8; n++ ) {
    n2 = mailbox[mailbox256[j] + lion_single_steps[n]];
    if ( n2 != -1 ) {
      if ( color[n2] == EMPTY ) { /* empty square */
	if ( check_square(n2, area, array_size)) { /* new square */
	  /* array movers don't promote */
	  if ( burning(i,n2) )
	    gen_push(i,n2,65);
	  else
	    gen_push(i,n2,0);
	}
	/* check next step here */
	check_area(i, n2,s,area, array_size,depth-1);
      } else if ( color[n2] == s ) ; /* own piece */
      else {    /* capture */
	/* check for very tame FiD here */
	if ( verytame_FiD && 
	     piece[i] == FIRE_DEMON ) ;
	else if ( check_square(n2, area, array_size) ) { /* new square */
	  if ( burning(i,n2) )
	    gen_push(i,n2,65);
	  else
	    gen_push(i,n2,1);
	}
      }
    }
  }
}

#ifdef EVAL_INFLUENCE

void check_area_infl(int i, int j, int s, int *area, int array_size, int depth) {
  /* i is the old start of the step, j is the step between (square we're standing on)*/
  int n, n2;
  if ( depth <= 0 ) return;
  for ( n = 0; n < 8; n++ ) {
    n2 = mailbox[mailbox256[j] + lion_single_steps[n]];
    if ( n2 != -1 ) {
      if ( color[n2] == EMPTY ) { /* empty square */
	if ( check_square(n2, area, array_size)) { /* new square */
	  influences[s][n2]++;
	  burn_infl(i,n2);
	}
	/* check next step here */
	check_area_infl(i, n2,s,area, array_size,depth-1);
      }
      /* influences[s][n2]++;*/
    }
  }
}

#endif

BOOL check_area_caps(int i, int j, int s, int *area, 
		     int array_size, int depth, int square) {
  /* i is the old start of the step, j is the step between (square we're standing on)*/
  int n, n2;
  BOOL retvalue = FALSE; 
  if ( depth <= 0 ) return FALSE;
  for ( n = 0; n < 8; n++ ) {
    n2 = mailbox[mailbox256[j] + lion_single_steps[n]];
    if ( n2 != -1 ) {
      if ( color[n2] == EMPTY ) { /* empty square, maybe FiD burns adjacent pieces? */
#ifdef VALUABLE_CAPTURES
	if ( eval_burned(i,n2) > CAPTURE_THRESHOLD )
	   gen_push(i,n2,65);
#else
	if ( burning(i,n2) ) gen_push(i,n2,65);
#endif
	retvalue = check_area_caps(i, n2,s,area, array_size,depth-1, square);
      } else if ( color[n2] == s ) ; /* own piece */
      else {    /* capture */
	/* check for very tame FiD here */
	if ( verytame_FiD && 
	     piece[i] == FIRE_DEMON ) ;
	else {
	  if ( check_square(n2, area, array_size) ) { /* new square */
#ifdef VALUABLE_CAPTURES
	    if ( piece_value[piece[n2]] > CAPTURE_THRESHOLD )
#endif
	      gen_push(i,n2,1);
	    retvalue = TRUE;
	  }
	}
      }
    }
  }
  return retvalue;
}


void special_moves( int i,  int s ) {
  /* called if move_types[ piece[i]][8] is non-none. 
     Used for area moves and the lion knight jump moves
     and the double capture moves not covered by
     the lion direction moves.
  */
  int n, n2, n3, n4;
  BOOL igui, iguip;
  move_type movetype;

#ifdef DEBUG
  if (( piece[i] == EMPTY )&&(color[i] == s)) {
    fprintf(stderr,"#Special moves: Field %d belongs to %d, although empty!\n",i,s);
    /* print_board(stderr); */
    /* piece does not belong to side */
  }
#endif
  movetype  = move_types[piece[i]][8];
  igui = FALSE;
  iguip = FALSE;
  switch ( movetype ) {
  case igui_capture: /* TODO */
    for ( n = 0; n < 8; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_single_steps[n]];
      if (( n2 != -1 ) && ( color[n2] == xside ))
	gen_push_igui( i, n2, 1);
    }
    break;
  case area2: /* goes on with lion for Colin's rules */    
    if ( ! modern_lion_hawk ){ /* two-step area move, no igui */
      area_two[0] = i; /* no igui moves */
      for ( n = 1; n < 24; n++) {
	area_two[n] = -1;
      }
      check_area(i, i, s, area_two, 24, 2 ); /* two-step area */ 
      break;
    }
  case lion: 
    /* jumps */
    for ( n = 0; n < 16; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_jumps[n]];
      if ( n2 != -1 ) {
	if ( color[n2] == EMPTY )
	  gen_push(i,n2,0);
	else if ( color[n2] == s ) ;
	else 
	  gen_push(i,n2,1);
      }
    }
    /* single steps and igui moves */
    for ( n = 0; n < 8; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_single_steps[n]];
      if ( n2 != -1 ) {
	if ( color[n2] == EMPTY ) {
	  if ( can_promote(i,n2) ) {
	    gen_push(i,n2,16);
	    if ( ! iguip ) {
	      gen_push_igui(i,n2,16);
	      iguip = TRUE;
	    }
	    gen_push(i,n2,0);
	    if ( ! igui ) {
	      gen_push_igui(i,n2,0);
	      igui = TRUE;
	    }
	  } else {
	    gen_push(i,n2,0);
	    if ( ! igui ) {
	      gen_push_igui(i,n2,0);
	      igui = TRUE;
	    }
	  }
	}
	else if ( color[n2] == s ) ;
	else {    /* single capture, igui capture,
		     double move (first capture only to save time) 
		  */
	  if ( can_promote(i,n2) ) {
	    gen_push(i,n2,17);
	    gen_push(i,n2,1);
	    gen_push_igui(i,n2,17);
	    gen_push_igui(i,n2,1);
	  } else {
	    gen_push(i,n2,1);
	    gen_push_igui(i,n2,1);
	  }
	  /* capture + step */
	  for ( n3 = 0; n3 < 8; n3++) {
	    n4 = mailbox[mailbox256[n2] + lion_single_steps[n3]];
	    if ( n4 == -1 ) continue; /* off board */
	    if ( n4 == n2 ) continue; /* have it already */
	    if ( color[n4] == s ) continue; /* own piece */
	    if ( color[n4] == EMPTY ) {
	      if ( can_promote(i,n4) ) {
		gen_push_lion_move(i,n2,n4,20);
	      }
	      gen_push_lion_move(i,n2,n4,4);
	    } else if ( color[n4] == s );
	    else { /* capture */
	      if ( can_promote(i,n4) ) {
		gen_push_lion_move(i,n2,n4,21);
	      }
	      gen_push_lion_move(i,n2,n4,5);
	    }
	  } /* for */
	}
      }
    }

    break;
  case area3: /* VGn and FiD */
      area_three[0] = i; /* no igui moves */
      for ( n = 1; n < 48; n++) {
	area_three[n] = -1;
      }
      check_area(i, i, s, &area_three[0], 48, 3 ); /* three-step area */ 
      break;
    break;
  default:
    fprintf(stderr,"#special_moves: Unknown move type %d", movetype);
    break;
  }
}

#ifdef EVAL_INFLUENCE

void special_moves_infl( int i,  int s ) {
  /* called if move_types[ piece[i]][8] is non-none. 
     Used for area moves and the lion knight jump moves
     and the double capture moves not covered by
     the lion direction moves.
  */
  int n, n2, n3, n4;
  BOOL igui, iguip;
  move_type movetype;

#ifdef DEBUG
  if (( piece[i] == EMPTY )&&(color[i] == s)) {
    fprintf(stderr,"#Special moves: Field %d belongs to %d, although empty!\n",i,s);
    /* print_board(stderr); */
    /* piece does not belong to side */
  }
#endif
  movetype  = move_types[piece[i]][8];
  igui = FALSE;
  iguip = FALSE;
  switch ( movetype ) {
  case igui_capture: /* TODO */
    for ( n = 0; n < 8; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_single_steps[n]];
      if ( n2 != -1 )
	influences[s][n2]++;
    }
    break;
  case area2: /* goes on with lion for Colin's rules */    
    if ( ! modern_lion_hawk ){ /* two-step area move, no igui */
      area_two[0] = i; /* no igui moves */
      for ( n = 1; n < 24; n++) {
	area_two[n] = -1;
      }
      check_area_infl(i, i, s, area_two, 24, 2 ); /* two-step area */ 
      break;
    }
  case lion: 
    /* jumps */
    for ( n = 0; n < 16; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_jumps[n]];
      if ( n2 != -1 ) {
	influences[s][n2]++;
      }
    }
    /* single steps and igui moves */
    for ( n = 0; n < 8; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_single_steps[n]];
      if ( n2 != -1 ) {
	influences[s][n2]++;
	if ( color[n2] != side ) {
	  for ( n3 = 0; n3 < 8; n3++) {
	    n4 = mailbox[mailbox256[n2] + lion_single_steps[n3]];
	    if ( n4 == -1 ) continue; /* off board */
	    if ( n4 == n2 ) continue; /* have it already */
	    influences[s][n4]++;
	  } /* for */
	}
      }
    }

    break;
  case area3: /* VGn and FiD */
      area_three[0] = i; /* no igui moves */
      for ( n = 1; n < 48; n++) {
	area_three[n] = -1;
      }
      check_area_infl(i, i, s, &area_three[0], 48, 3 ); /* three-step area */ 
      break;
    break;
  default:
    fprintf(stderr,"#special_moves: Unknown move type %d", movetype);
    break;
  }
}

#endif

BOOL special_moves_caps( int i,  int s, int k ) {
  /* called if move_types[ piece[i]][8] is non-none. 
     Used for area moves and the lion knight jump moves
     and the double capture moves not covered by
     the lion direction moves. 
     Returns TRUE if a move from i to k for side s is generated.
  */
  int n, n2, n3, n4;
  BOOL igui, iguip;
  BOOL retvalue;
  move_type movetype;

#ifdef DEBUG

  if (( piece[i] == EMPTY )&&(color[i] == s)) {
    fprintf(stderr,"#Special moves: Field %d belongs to %d, although empty!\n",i,s);
    /* print_board(stderr); */
    /* piece does not belong to side */
  }

#endif

  movetype  = move_types[piece[i]][8];
  igui = FALSE;
  iguip = FALSE;
  retvalue = FALSE;
  switch ( movetype ) {
  case igui_capture: /* TODO */
    for ( n = 0; n < 8; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_single_steps[n]];
      if (( n2 != -1 ) && ( color[n2] == xside )) {
#ifdef VALUABLE_CAPTURES
	if (piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
	  gen_push_igui( i, n2, 1);
	  if ( n2 == k ) retvalue = TRUE;
#ifdef VALUABLE_CAPTURES
	}
#endif
      }
    }
    break;
  case area2:
    if ( ! modern_lion_hawk ){ /* two-step area move, no igui */
      area_two[0] = i; /* no igui moves */
      for ( n = 1; n < 24; n++) {
	area_two[n] = -1;
      }
      if ( check_area_caps(i, i, s, area_two, 24, 2, k ) )
	   retvalue = TRUE; /* two-step area */ 
      break;
    }
  case lion: 
    /* jumps */
    for ( n = 0; n < 16; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_jumps[n]];
      if ( n2 != -1 ) {
	if ( color[n2] == EMPTY );
	else if ( color[n2] == s );
	else {
#ifdef VALUABLE_CAPTURES
	  if (piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
	    gen_push(i,n2,1);
	    if ( n2 == k ) retvalue = TRUE;
#ifdef VALUABLE_CAPTURES
	  }
#endif
	}
      }
    }
    /* single steps and igui moves */
    for ( n = 0; n < 8; n++ ) {
      n2 = mailbox[mailbox256[i] + lion_single_steps[n]];
      if ( n2 != -1 ) {
	if ( color[n2] == EMPTY );
	else if ( color[n2] == s );
	/*	  if ( can_promote(i,n2) ) {
	    if ( n2 == k ) retvalue = TRUE;
#ifdef VALUABLE_CAPTURES
	    if (piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
	      gen_push(i,n2,17);
	      gen_push_igui(i,n2,17);
#ifdef VALUABLE_CAPTURES
	    }
#endif
	  }
	}
	*/
	else {    /* single capture, igui capture,
		     double move (first capture only to save time) 
		  */
	  if ( can_promote(i,n2) ) {
	    if ( n2 == k ) retvalue = TRUE;
#ifdef VALUABLE_CAPTURES
	    if (piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
	      gen_push(i,n2,17);
	      gen_push(i,n2,1);
	      gen_push_igui(i,n2,17);
	      gen_push_igui(i,n2,1);
#ifdef VALUABLE_CAPTURES
	    }
#endif
	  } else {
#ifdef VALUABLE_CAPTURES
	    if (piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
	      gen_push(i,n2,1);
	      gen_push_igui(i,n2,1);
#ifdef VALUABLE_CAPTURES
	    }
#endif
	  }
	  /* capture + step */
	  for ( n3 = 0; n3 < 8; n3++) {
	    n4 = mailbox[mailbox256[n2] + lion_single_steps[n3]];
	    if ( n4 == -1 ) continue; /* off board */
	    if ( n4 == n2 ) continue; /* have it already */
	    if ( color[n4] == s ) continue; /* own piece */
	    if ( color[n4] == EMPTY ) {
	      if ( can_promote(i,n4) ) {
#ifdef VALUABLE_CAPTURES
		if (piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
		  gen_push_lion_move(i,n2,n4,21);
		  if ( n2 == k ) retvalue = TRUE;
		  if ( n4 == k ) retvalue = TRUE;
#ifdef VALUABLE_CAPTURES
		}
#endif
	      }
	    } else if ( color[n4] == s );
	    else { /* capture */
	      if ( can_promote(i,n4) ) {
#ifdef VALUABLE_CAPTURES
		if (piece_value[piece[n2]]+piece_value[piece[n4]] > CAPTURE_THRESHOLD ) {
#endif
		  gen_push_lion_move(i,n2,n4,21);
		  if ( n2 == k ) retvalue = TRUE;
		  if ( n4 == k ) retvalue = TRUE;
#ifdef VALUABLE_CAPTURES
		}
#endif
	      }
	    }
	  } /* for */
	}
      }
    }

    break;
  case area3:
    /*
      FiD 3-step area + burn 
      making this simply with a four-step
      area is not correct since it does not
      allow for *capturing* in the three-step
      range and burning in the next 
    */ 
    area_three[0] = i; /* no igui moves */
    for ( n = 1; n < 48; n++)
      area_three[n] = -1;
    if (check_area_caps(i, i, s, &area_three[0], 48, 3, k ) ) 
      retvalue = TRUE; /* three-step area */
    break;
  default:
    fprintf(stderr,"#special_moves_caps: Unknown move type %d", movetype);
    return FALSE;
    break;
  }
  return retvalue;
}


int get_steps( int i, int j, int s ) {
  /* for n-step moves and slides, returns integer for
     the for loop of gen() and friends. For lion type
     moves, pushed the moves on the stack itself and
     returns 0.
     i is the square the piece is standing on,
     j is the move direction,
     s is the side the piece belongs to
  */
  int n, n2;
  BOOL only_capture; /* for jumping slides */
  move_type movetype;
  
#ifdef DEBUG
  if (( piece[i] == EMPTY )&&(color[i] == s)) {
    fprintf(stderr,"#get_steps: Field %d belongs to %d, although empty!\n",i,s);
    /* print_board(stderr); */
    /* piece does not belong to side */
  }
#endif
  only_capture = FALSE;
  movetype  = move_types[piece[i]][j];
  if ( movetype == none ) return(0);
  switch ( movetype ) {
  case tetrarch:
    /* jump */
    n = mailbox[mailbox256[i] + 2* offset[s][piece[i]][j]];
    if ( n == -1 ) return(0); /* no need to check the triple step */
    if ( color[n] != side ) {
      if ( color[n] == EMPTY ) {
	gen_push(i, n, 0);
      } else {
	gen_push(i, n, 1);
      }
    }
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ); /* should not be possible */
    else {
      if ( piece[n] == EMPTY ) { /* triple step may be possible */
	n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	if (( n != -1 )&&( piece[n] == EMPTY )) { /* triple step may be possible */
	  n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	  if (( n != -1 )&&( color[n] != s )) {
	    if ( piece[n] == EMPTY ) {
		gen_push(i,n,0);
	    } else { /* capture */
	      gen_push(i,n,1);
	    }
	  }
	} 
      }
    }
    return 0;
  case lion:
    /* add lion moves for SEg and HF */
    /* single step and igui first */
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ) return 0; /* first part-move off board already */

    if (color[n] != EMPTY) { /* only here the double step is different
				from the jump */
      if (color[n] != s) {
	if ( can_promote(i,n) ) {
	  gen_push(i, n, 17);        /* capture with promotion */
	  gen_push_igui(i, n, 49);   /* igui capture with promotion */
	}
	gen_push(i, n, 1);           /* capture w.o. promotion */
	gen_push_igui(i, n, 33);     /* igui capture w.o. promotion */
	n2 = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]]; /* next step */
	if ( n2 == -1 ) break;        /* jump also not possible */
	if (color[n2] == s ) break;   /* don't step on own pieces */
	/* we have a double step and a capture on the first move.
	   maybe there's a capture on the second move as well. */
	if ( can_promote(i,n2) || can_promote(i,n) ) {
	  gen_push_lion_move(i,n,n2,21);
	  gen_push_lion_move(i,n,n2,5);
	}
	else {
	  gen_push_lion_move(i,n,n2,5);
	} 
      }
    } else {
      if ( can_promote(i,n) ) {
	gen_push(i, n, 16);       /* simple promotion */
	gen_push_igui(i, n, 48);  /* igui promotion */
	}
      gen_push(i, n, 0);
      gen_push_igui(i, n, 0);	
    }
    /* jump */
    n = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]];
    if ( n == -1 );
    else {
      if (color[n] != EMPTY) {
	if (color[n] != s) {
	  if ( can_promote(i,n) ) {
	    gen_push(i, n, 17);   /* capture with promotion */
	  }
	  gen_push(i, n, 1);      /* capture without promotion */
	}
      } else { /* no capture on jump */
	if ( can_promote(i,n) ) {
	  gen_push(i, n, 16);    /* promotion */
	}
	gen_push(i, n, 0);       /* normal, boring move */
      }
    }
    return(0);
  case jumpslide:
    for ( n = i;;) { 
      n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
      if (n == -1)
	break;
      if (color[n] != EMPTY) {
	if ( ! only_capture ) {
	  only_capture = TRUE;
	  if (color[n] == xside) {
	    if ( can_promote(i,n) )
	      gen_push(i, n, 17);
	    gen_push(i,n,1);
	  }
	} else { /* only capture */
	  if ( jgs_tsa && higher(i,n)) { /* tsa rule for jgs */
	    break;
	  } else {
	    if (color[n] == xside) {
	      if ( can_promote(i,n) )
		gen_push(i, n, 17);
	      gen_push(i,n,1);
	    }
	  }
	}
	if ( !jgs_tsa && higher(i,n)) /* my jgs rule */
	  break;
      } else if ( ! only_capture ) { /* EMPTY */
	if ( can_promote(i,n) )
	  gen_push(i, n, 16);
	gen_push(i, n, 0);
      }
    }
    return(0);
  case free_eagle:
#ifdef edo_style_FEg
    /* add lion moves for SEg and HF */
    /* single step and igui first */
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ) return 0; /* first part-move off board already */

    if (color[n] != EMPTY) { /* only here the double step is different
				from the jump */
      if (color[n] != s) {
	gen_push(i, n, 1);           /* capture w.o. promotion */
	gen_push_igui(i, n, 33);     /* igui capture w.o. promotion */
	n2 = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]]; /* next step */
	if ( n2 == -1 ) break;        /* jump also not possible */
	if (color[n2] == s ) break;   /* don't step on own pieces */
	/* we have a double step and a capture on the first move.
	   maybe there's a capture on the second move as well. */
	gen_push_lion_move(i,n,n2,5);
      }
    } else {
      gen_push(i, n, 0);
      gen_push_igui(i, n, 0);	
    }
    /* jump */
    n = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]];
    if ( n == -1 );
    else {
      if (color[n] != EMPTY) {
	if (color[n] != s) {
	  gen_push(i, n, 1);      /* capture without promotion */
	}
      } else { /* no capture on jump */
	gen_push(i, n, 0);       /* normal, boring move */
      }
    }
#else
    /* add jump and go on with slide */
      n = mailbox[mailbox256[i] + 2 * offset[s][piece[i]][j]];
      if ( n == -1 ) return(1); /* no sense in checking further */
      else if (color[n] != EMPTY ) {
	if ( color[n] != s ) { /* enemy piece */
	  if ( can_promote(i,n) ) gen_push(i, n, 17);
	  gen_push(i, n, 1);
	}
      } else { /* free square */
	if ( can_promote(i,n) ) gen_push(i, n, 16);
	gen_push(i, n, 0);
      } /* no break here!!! */
#endif
  case slide:
	/* check for very tame FiD here */
	if ( verytame_FiD && 
	     piece[i] == FIRE_DEMON) {
	  for ( n = i;;) { 
	    n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	    if (n == -1)
	      break;
	    if (color[n] == EMPTY) {
	      gen_push(i,n,1); /* FiD does not promote */
	    } else {
	      break; /* any non-empty square stops the FiD */
	    }
	  }
	  return(0);
	} else {
	  return(16);
	}
	break;
  case htslide:
	  for ( n = i ;;) { 
	    n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	    if (n == -1) /* off-board */
	      break;
	    if ( color[n] == EMPTY ) {
	      if ((( n > i )&&( n> i+17))||
		  ((n<i)&&(i> n+17))) 
		gen_push(i,n,1); /* HT does not promote */
	    } else {
	      if ( color[n] == xside ) {
		gen_push(i,n,1);
	      }
	      break; /* any non-empty square stops the HT */
	    }
	  }
	  return(0);
    
  case two_steps:
    return(2);
  case step:
    return(1);
  default: /* should never occur */
    fprintf(stderr,"#Unknown move_type %d: piece: %d, from: %d, to: %d\n", movetype, piece[i], i,n);
    return(0);
  }
  return(0);
}

#ifdef EVAL_INFLUENCE

int get_steps_infl( int i, int j, int s ) {
  /* for n-step moves and slides, returns integer for
     the for loop of gen() and friends. For lion type
     moves, pushed the moves on the stack itself and
     returns 0.
     i is the square the piece is standing on,
     j is the move direction,
     s is the side the piece belongs to
  */
  int n, n2;
  BOOL only_capture; /* for jumping slides */
  move_type movetype;
  
#ifdef DEBUG
  if (( piece[i] == EMPTY )&&(color[i] == s)) {
    fprintf(stderr,"#get_steps: Field %d belongs to %d, although empty!\n",i,s);
    /* print_board(stderr); */
    /* piece does not belong to side */
  }
#endif
  only_capture = FALSE;
  movetype  = move_types[piece[i]][j];
  if ( movetype == none ) return(0);
  switch ( movetype ) {
  case tetrarch:
    /* jump */
    n = mailbox[mailbox256[i] + 2* offset[s][piece[i]][j]];
    if ( n == -1 ) return(0); /* no need to check the triple step */
    influences[s][n]++;
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ); /* should not be possible */
    else {
      if ( piece[n] == EMPTY ) { /* triple step may be possible */
	n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	if (( n != -1 )&&( piece[n] == EMPTY )) { /* triple step may be possible */
	  n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	  if ( n != -1 ) {
	    influences[s][n]++;
	    }
	  }
	} 
    }
    return 0;
  case lion:
    /* add lion moves for SEg and HF */
    /* single step and igui first */
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ) return 0; /* first part-move off board already */

    if (color[n] != EMPTY) { /* only here the double step is different
				from the jump */
      influences[s][n]++;
      if (color[n] != s) {
	n2 = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]]; /* next step */
	if ( n2 == -1 ) break;        /* jump also not possible */
	influences[s][n2]++;
      }
    }
    /* jump */
    n = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]];
    if ( n == -1 );
    else {
      influences[s][n]++;
    }
    return(0);
  case jumpslide:
    for ( n = i;;) { 
      n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
      if (n == -1)
	break;
      if (color[n] != EMPTY) {
	if ( ! only_capture ) {
	  only_capture = TRUE;
	  influences[s][n]++;
	} else { /* only capture */
	  if ( jgs_tsa && higher(i,n)) { /* tsa rule for jgs */
	    break;
	  } else {
	    influences[s][n]++;
	  }
	}
	if ( !jgs_tsa && higher(i,n)) /* my jgs rule */
	  break;
      } else if ( ! only_capture ) { /* EMPTY */
	influences[s][n]++;
      }
    }
    return(0);
  case free_eagle: /* add jump and go on with slide */
      n = mailbox[mailbox256[i] + 2 * offset[s][piece[i]][j]];
      if ( n == -1 ) return(1); /* no sense in checking further */
      influences[s][n]++;
      /* no break here!!! */
  case slide:
    return(16);
  case two_steps:
    return(2);
  case step:
    return(1);
  default: /* should never occur */
    fprintf(stderr,"#Unknown move_type %d: piece: %d, from: %d, to: %d\n", movetype, piece[i], i,n);
    return(0);
  }
  return(0);
}

#endif

int get_steps_caps( int i, int j, int s ) {
  /* for n-step moves and slides, returns integer for
     the for loop of gen() and friends. For lion type
     moves, pushed the moves on the stack itself and
     returns 0.
     i is the square the piece is standing on,
     j is the move direction,
     s is the side the piece belongs to
  */
  int n, n2;
  BOOL only_capture; /* for jumping slides */
  move_type movetype;
  
#ifdef DEBUG
  if (( piece[i] == EMPTY )&&(color[i] == s)) {
    fprintf(stderr,"#get_steps: Field %d belongs to %d, although empty!\n",i,s);
    /* print_board(stderr); */
    /* piece does not belong to side */
  }
#endif
  only_capture = FALSE;
  movetype  = move_types[piece[i]][j];

  switch ( movetype ) {
  case tetrarch:
    /* jump */
    n = mailbox[mailbox256[i] + 2* offset[s][piece[i]][j]];
    if ( n == -1 ) return(0); /* no need to check the triple step */
    if ( color[n] == xside ) {
#ifdef VALUABLE_CAPTURES
      if (piece_value[piece[n]] > CAPTURE_THRESHOLD ) 
#endif
    	gen_push(i, n, 1);

    }
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ); /* should not be possible */
    else {
      if ( piece[n] == EMPTY ) { /* triple step may be possible */
	n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	if (( n != -1 )&&( piece[n] == EMPTY )) { /* triple step may be possible */
	  n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
	  if (( n != -1 )&&( color[n] == xside )) {
#ifdef VALUABLE_CAPTURES
	    if (piece_value[piece[n]] > CAPTURE_THRESHOLD )
#endif
	      gen_push(i,n,1);
	  }
	}
      } 
    }
    return 0;
    case lion:
    /* add lion moves for SEg and HF */
    /* single step and igui first */
    n = mailbox[mailbox256[i] + offset[s][piece[i]][j]];
    if ( n == -1 ) return 0; /* first part-move off board already */

    if (color[n] != EMPTY) { /* only here the double step is different
				from the jump */
      if (color[n] != s) {
	if ( can_promote(i,n) ) {
#ifdef VALUABLE_CAPTURES
	  if (piece_value[piece[n]] > CAPTURE_THRESHOLD ) {
#endif
	  gen_push(i, n, 17);        /* capture with promotion */
	  gen_push_igui(i, n, 49);   /* igui capture with promotion */
#ifdef VALUABLE_CAPTURES
	  }
#endif
	}
#ifdef VALUABLE_CAPTURES
	  if (piece_value[piece[n]] > CAPTURE_THRESHOLD ) {
#endif
	gen_push(i, n, 1);           /* capture w.o. promotion */
	gen_push_igui(i, n, 33);     /* igui capture w.o. promotion */
#ifdef VALUABLE_CAPTURES
	  }
#endif
	n2 = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]]; /* next step */
	if ( n2 == -1 ) break;        /* jump also not possible */
	if (color[n2] == s ) break;   /* don't step on own pieces */
	/* we have a double step and a capture on the first move.
	   maybe there's a capture on the second move as well. */
#ifdef VALUABLE_CAPTURES
	if (piece_value[piece[n]]+piece_value[piece[n2]] > CAPTURE_THRESHOLD ) {
#endif
	  if ( can_promote(i,n2) || can_promote(i,n) ) {
	    gen_push_lion_move(i,n,n2,21);
	    gen_push_lion_move(i,n,n2,5);
	  }
	  else {
	    gen_push_lion_move(i,n,n2,5);
	  } 
#ifdef VALUABLE_CAPTURES
	  }
#endif
      }
#ifndef VALUABLE_CAPTURES
    } else {
      if ( can_promote(i,n) ) {
	gen_push(i, n, 16);       /* simple promotion */
	gen_push_igui(i, n, 48);  /* igui promotion */
      }
#endif
      /* now don't investigate the second move, since if the
        first is not a capture, we can simply jump there */
    } 
    /* jump */
    n = mailbox[mailbox256[i] + 2*offset[s][piece[i]][j]];
    if ( n == -1 );
    else {
      if (color[n] != EMPTY) {
	if (color[n] != s) {
#ifdef VALUABLE_CAPTURES
	  if (piece_value[piece[n]] > CAPTURE_THRESHOLD ) {
#endif
	    if ( can_promote(i,n) ) {
	      gen_push(i, n, 17);   /* capture with promotion */
	    }
	    gen_push(i, n, 1);      /* capture without promotion */
#ifdef VALUABLE_CAPTURES
	  }
#endif
	} else { /* no capture on jump */
#ifndef VALUABLE_CAPTURES
	if ( can_promote(i,n) ) {
	  gen_push(i, n, 16);    /* promotion */
	}
#endif
      }
    }
    return(0);
  case jumpslide:
    for ( n = i;;) { 
      n = mailbox[mailbox256[n] + offset[s][piece[i]][j]];
      if (n == -1)
	break;
      if (color[n] != EMPTY) {
	if ( jgs_tsa && higher(i,n)) /* tsa rule for jgs */
	  break;
	only_capture = TRUE; /* from now on only captures allowed */
	if (color[n] == xside) {
#ifdef VALUABLE_CAPTURES
	  if (piece_value[piece[n]] > CAPTURE_THRESHOLD ) {
#endif
	    if ( can_promote(i,n) )
	      gen_push(i, n, 17);
	    gen_push(i,n,1);
#ifdef VALUABLE_CAPTURES
	  }
#endif
	}
	if ( !jgs_tsa && higher(i,n)) /* my jgs rule */
	  break;
#ifndef VALUABLE_CAPTURES
      } else if ( ! only_capture ) { /* color ==  EMPTY */
	if ( can_promote(i,n) )
	  gen_push(i, n, 16);
#endif
      }
    }
    return(0);
    case free_eagle: /* add jump and go on with slide */
      n = mailbox[mailbox256[i] + 2 * offset[s][piece[i]][j]];
      if ( n == -1 ) return(1); /* no sense in checking further */
      else if (color[n] != EMPTY ) {
#ifdef VALUABLE_CAPTURES
	  if (piece_value[piece[n]] > CAPTURE_THRESHOLD )
#endif
	    if ( color[n] != s ) { /* enemy piece */
	      if ( can_promote(i,n) ) gen_push(i, n, 17);
	      gen_push(i, n, 1);
	    }
#ifndef VALUABLE_CAPTURES
      } else { /* free square */
	if ( can_promote(i,n) ) gen_push(i, n, 16);
	gen_push(i, n, 0);
#endif
      } /* no break here!!! */
  case slide:
    return(16);
  case two_steps:
    return(2);
  case step:
    return(1);
  default: /* should never occur */
    fprintf(stderr,"#Unknown move_type %d: piece: %d, from: %d, to: %d\n", movetype, piece[i], i,n);
    return(0);
  }
  return(0);
  }
}


BOOL higher( int jumping, int standing ) {
  /* returns true if standing piece is of higher hierarchy than jumping piece */
  int thatpiece = piece[standing];
  int thispiece = piece[jumping];
  if (( thatpiece == KING )|| ( thatpiece == GREAT_GENERAL) || ( thatpiece == PGREAT_GENERAL)) return TRUE;
  if (( thatpiece == VICE_GENERAL ) ||( thatpiece == PVICE_GENERAL )) {
    if (( thispiece == GREAT_GENERAL )||( thispiece == PGREAT_GENERAL )) return FALSE;
    else return TRUE;
  }
  if (( thatpiece == ROOK_GENERAL )||( thatpiece == BISHOP_GENERAL )||
      ( thatpiece == PROOK_GENERAL )||( thatpiece == PBISHOP_GENERAL )) {
    if (( thispiece == VICE_GENERAL) || (thispiece == GREAT_GENERAL)||
	( thispiece == PVICE_GENERAL) || (thispiece == PGREAT_GENERAL))
      return FALSE;
    else return TRUE;
  }
  return FALSE;
}

/* gen() generates pseudo-legal moves for the current position.
   It scans the board to find friendly pieces and then determines
   what squares they attack. When it finds a piece/square
   combination, it calls gen_push to put the move on the "move
   stack." */

void gen()
{
	int i, j, n;
	int steps; /* steps into single direction */
	/* so far, we have no moves for the current ply */
	gen_end[ply] = gen_begin[ply];
	
	if ( lost(side)) return;

	for (i = 0; i < NUMSQUARES; ++i)
		if (color[i] == side) {
		  if ( move_types[piece[i]][8] != none ) /* special move piece */
		    special_moves( i, color[i] );
		  for (j = 0; j < offsets[piece[i]]; ++j) {
		    steps = get_steps(i,j, color[i]);
		    for (n = i;;) { /* stepping into a single direction */
		      if ( steps <= 0 )
			break;
		      n = mailbox[mailbox256[n] + offset[side][piece[i]][j]];
		      if (n == -1)
			break;
		      if (color[n] != EMPTY) {
			if (color[n] == xside) {
			  if ( can_promote(i,n) ) {
			    gen_push(i, n, 17);
			    if ( ! must_promote(i,n) )
			      gen_push(i, n, 1);
			  } else 
			    gen_push(i,n,1);
			}
			break;
		      } else { /* color == side + EMPTY */
			if ( can_promote(i,n) ) {
			  gen_push(i, n, 16);
			  if ( ! must_promote(i,n) ) gen_push(i, n, 0);
			} else {
			  if ( burning(i,n) )
			    gen_push(i,n,65);
			  else 
			    gen_push(i, n, 0);
			}
		      }
		      steps--;
		    }
		  }
		}

	/* the next ply's moves need to start where the current
	   ply's end */
	gen_begin[ply + 1] = gen_end[ply];
}

#ifdef EVAL_INFLUENCE

void gen_infl()
{
  int i, j, n;
  int steps; /* steps into single direction */
  /* so far, we have no moves for the current ply */

  for (i = 0; i < NUMSQUARES; ++i)
    if (color[i] == side) {
      if ( move_types[piece[i]][8] != none ) /* special move piece */
	special_moves_infl( i, color[i] );
      for (j = 0; j < offsets[piece[i]]; ++j) {
	steps = get_steps_infl(i,j, color[i]);
	for (n = i;;) { /* stepping into a single direction */
	  if ( steps <= 0 )
	    break;
	  n = mailbox[mailbox256[n] + offset[side][piece[i]][j]];
	  if (n == -1)
	    break;
	  influences[side][n]++;
	  burn_infl(i,n);
	  if (color[n] != EMPTY)
	    break;
	  steps--;
	}
      }
    }
}


#endif

/* gen_caps() is basically a copy of gen() that's modified to
   only generate capture and promote moves. It's used by the
   quiescence search. */

void gen_caps()
{
	int i, j, n;
	int steps; /* steps into single direction */
	/* so far, we have no moves for the current ply */
	gen_end[ply] = gen_begin[ply];
	for (i = 0; i < NUMSQUARES; ++i)
		if (color[i] == side) {		    
		  if ( move_types[piece[i]][8] ) { /* special move piece */
		    special_moves_caps( i, color[i], 0 );
		  }
		  for (j = 0; j < offsets[piece[i]]; ++j){
		    steps = get_steps_caps(i,j, color[i]);
		    for (n = i;;) {
		      if ( steps <= 0 )
			break;
		      n = mailbox[mailbox256[n] + offset[side][piece[i]][j]];
		      if (n == -1) /* out of board */
			break;
		      if (color[n] != EMPTY) {
#ifdef VALUABLE_CAPTURES
			if (piece_value[piece[n]] > CAPTURE_THRESHOLD ) {
#endif
			  if (color[n] == xside) { /* enemy */
			    if ( can_promote(i,n) ) {
			      gen_push(i, n, 17);
			      if ( ! must_promote(i,n) ) gen_push(i, n, 1);
			    } else
 			      gen_push(i, n, 1);
			  }
#ifdef VALUABLE_CAPTURES
			}
#endif
			break;
		      } else { /* empty square, add if promotion */
#ifndef VALUABLE_CAPTURES
			if ( can_promote(i,n) ) {
			  gen_push(i, n, 16);
			} else {
			  if ( burning(i,n) )
			    gen_push(i,n,65);
			}
#else                     
			if ( eval_burned(i,n) > CAPTURE_THRESHOLD )
			  gen_push(i,n,65);
#endif
			steps--;
		      }
		    }
		  }
		}
	gen_begin[ply + 1] = gen_end[ply];
}

BOOL can_promote( int from, int to ) {
  int idx;

  idx = can_promote_zone[side][piece[from]];

  /* printf("piece: %d, from: %d, to: %d, idx: %d\n", piece[from],from,to,idx); */

  if (( idx == 0)|| (promotion[piece[from]]== 0 )) return FALSE;
  if ( side == LIGHT ) {
    if ( ( from <= idx)  || (to <= idx) ) {
      /* printf("can LIGHT piece: %d, from: %d, to: %d, idx: %d\n", 
	 piece[from],from,to,idx); */
      return TRUE;
    }
  }  else { 
    if ( (from >= idx) || (to >= idx) ) {
      /* printf("can DARK piece: %d, from: %d, to: %d, idx: %d\n", 
	 piece[from],from,to,idx); */
      return TRUE;
    }
  }
  return FALSE;
}

BOOL must_promote( int from, int to ) {
  int idx;
  if ( piece[from] < 0 ) return FALSE;
  idx = must_promote_zone[side][piece[from]];
  if ( !idx ) return FALSE;
  if ( side == LIGHT ) {
    if ( (from < idx) || (to < idx) ) {
      /* printf("must LIGHT piece: %d, from: %d, to: %d, idx: %d\n", 
	 piece[from],from,to,idx); */
      return TRUE;
    }
  }  else { 
    if ( (from > idx) || (to > idx) ) {
      /* printf("must DARK piece: %d, from: %d, to: %d, idx: %d\n", 
	 piece[from],from,to,idx); */
      return TRUE;
    }
  }
  return FALSE;
}



/* gen_push() puts a move on the move stack, unless it's a
   promotion that needs to be handled by gen_promote().
   It also assigns a score to the move for alpha-beta move
   ordering. If the move is a capture, it uses MVV/LVA
   (Most Valuable Victim/Least Valuable Attacker). Otherwise,
   it uses the move's history heuristic value. Note that
   1,000,000 is added to a capture move's score, so it
   always gets ordered above a "normal" move. FiD moves
   always get onto the move stack with this function, so it's
   the function which checks whether adjacent pieces were burnt.
*/

void gen_push(int from, int to, int bits)
{
	gen_t *g;
	
	int i, n, k;
	
	g = &gen_dat[gen_end[ply]];
	g->m.b.bits &= 0; /* set all bits to zero */

	++gen_end[ply];

	g->score = 0;
	g->m.b.bits = (unsigned char)bits;
	g->m.b.burned_pieces = '0';

	if (( piece[from] == FIRE_DEMON ) || ( piece[from] == PFIRE_DEMON )) {
#ifdef DEBUG
	      fprintf(stderr,"b: %d%c-%d%c:", 16 - GetFile(from),
		      GetRank(from) + 'a',
		      16 - GetFile(to),
		      GetRank(to) + 'a');
#endif
	  g->m.b.bits |= 64;
	  for (i = 0; i < 8; i++) {
	    n = mailbox[mailbox256[to] + lion_single_steps[i]];
	    if ( n != -1 && (piece[n] != EMPTY) && ( color[n] == xside )) {
	      g->score += piece[n];
	      g->m.b.burned_pieces ++;
#ifdef DEBUG
	      fprintf(stderr,"%s%d%c",
		      piece_string[piece[n]],
		      16 - GetFile(n),
		      GetRank(n) +'a');

#endif
	    }
	  }
#ifdef DEBUG
	  fprintf(stderr,"\n");
#endif
	}
	if (bits & 16) {
	  i = promotion[ piece[from ] ];
	} else {
	  i = 0;
	}

	g->m.b.from = (unsigned char)from;
	g->m.b.over = (unsigned char)(EMPTY);
	g->m.b.to = (unsigned char)to;
	g->m.b.promote = i;

	if (color[to] != EMPTY)
		g->score = 100000 + (piece[to] * 10) - piece[from];
	else
		g->score = history[from][to];
	if ( suicide(to) ) { /* landing next to enemy FiD */
	  g->m.b.bits |=  2;
	  g->score -= ( 1000000 + piece[from] * 10);
	}
}

void gen_push_igui(int from, int over, int bits)
{
        int i;
	gen_t *g;
	
	g = &gen_dat[gen_end[ply]];
	g->m.b.bits &= 0; /* set all bits to zero */

	if (bits & 16) i = promotion[ piece[from ] ];
	else i = 0;

	g->m.b.promote = i;
	++gen_end[ply];
	g->m.b.from = (unsigned char)from;
	g->m.b.over = (unsigned char)(EMPTY);
	g->m.b.to = (unsigned char)over;
	g->m.b.bits = (unsigned char)( bits | 32 );
	if (color[over] != EMPTY)
		g->score = 100000 + (piece[over] * 10) - piece[from];
	else
		g->score = history[from][over];

	if ( suicide(from) ) { /* landing next to enemy FiD (just promoted WBf) */
	  g->m.b.bits |= 2;
	  g->score -= ( 100000 + piece[from] * 10);
	}
}


void gen_push_lion_move(int from, int over, int to, int bits)
{
  /* the over move is only interesting when there's a capture
     on the over field. Only then this function should be called.
     All fields are only necessary for the Ln, for the HF and
     the SEg the over field is clear.
  */
        int i;
	gen_t *g;
	
	g = &gen_dat[gen_end[ply]];
	g->m.b.bits = (unsigned char)bits;
	g->m.b.bits |= 5; /* set all bits to zero except Ln move bit
			     and capture bit */

	if (bits & 16) i = promotion[ piece[from ] ];
	else i = 0;

	g->m.b.promote = i;
	++gen_end[ply];
	g->m.b.from = (unsigned char)from;
	g->m.b.over = (unsigned char)over;
	g->m.b.to = (unsigned char)to;

	g->score = 100000 + (piece[over] * 10) - piece[from];
	if ((color[to] != EMPTY)||(color[over] != EMPTY ))
		g->score = 100000 + (piece[to] * 10) - piece[from];
	else {
	  fprintf(stderr,"#Error in gen_push_lion_move");
	  g->score = history[from][over];
	}
	if ( suicide(to) ) { /* landing next to enemy FiD */
	  g->m.b.bits |= 2;
	  g->score -= ( 100000 + piece[from] * 10);
	}
#ifdef DEBUG
	if ( suicide(to) ) {
	  fprintf(stderr,"Lion move %dx%d-%d ", from, over, to);
	} else {
	  fprintf(stderr,"Lion move %dx%d-%d* ", from, over, to);
	}
#endif
}


BOOL suicide( int sq ) {
  /* Returns true iff sq is next to enemy FiD */
  /* There should be a counter of FiDs on board to speed this up. */
  int i, n;
  if ( ! fire_demons[xside] ) return FALSE;
  for (i = 0; i < 8; i++ ) {
    n = mailbox[mailbox256[sq] + lion_single_steps[i]];
    if (( n != -1) && (color[n] == xside) && (piece[n] == FIRE_DEMON))
      return TRUE;
  }
  return FALSE;
}

/* makemove() makes a move. If the move is illegal, it
   undoes whatever it did and returns FALSE. Otherwise, it
   returns TRUE. */

BOOL makemove(move_bytes m)
{
  int i;

  if ( m.from == m.to ) { /* igui is notated differently */
    fprintf(stderr,"#makemove: illegal move: %s\n", half_move_str(m));
    return FALSE;
  }
  if (piece[(int)m.from] == EMPTY) return FALSE; /* shouldn't happen */
  else 
    m.oldpiece = piece[(int)m.from]; /* we need this information here, too, for showing
					the undos and redos and only the bits go onto the
					undo_dat and redo_dat */

  if (lost(side) || lost(xside)) return FALSE; /* no moves after end of game */

  /* tame FiD doesn't burn when capturing */
  if ( tame_FiD ) {
    if ( piece[(int)m.to] == EMPTY ) {
      for (i = 0; i < 8; i++ ) {
	hist_dat[ply].burn[i] = EMPTY;
      }
    }
  } else {
    for (i = 0; i < 8; i++ ) {
      hist_dat[ply].burn[i] = EMPTY;
    }
  }
  if ( piece[(int)m.to] == FIRE_DEMON ) /* we've captured an FiD */
    fire_demons[xside]--;

  if ( m.bits & 4 ) {
    if (!make_lion_move(m)) 
      return FALSE;
    else return TRUE;
  } else if ( m.bits & 32 ) {
    if ( !make_igui(m))
      return FALSE;
    else return TRUE;
  } else if ( m.bits & 64 ) {
    if (!make_FiD(m))
      return FALSE;
    else return TRUE;
  } else {
    if ( !make_normal_move(m))
      return FALSE;
    else
      return TRUE;
  }
  fprintf(stderr,"#makemove: should not get here\n");
  return FALSE;
}

BOOL make_lion_move( move_bytes m) {
  BOOL sth_captured = FALSE;
  /* back up information so we can take the move back later. */
  hist_dat[ply].m.b = m;
  hist_dat[ply].oldpiece = piece[(int)m.from];
  hist_dat[ply].capture = piece[(int)m.to];
  hist_dat[ply].last_capture = last_capture;
  hist_dat[ply].over = (int)m.over;

  if (( piece[(int)m.over] != EMPTY )|| (color[(int)m.over] == xside)) {
    hist_dat[ply].capture2 = piece[(int)m.over];
    pieces_count[xside] --;
    captured[xside][demote(piece[(int)m.over])]++;
    color[(int)m.over] = EMPTY;
    piece[(int)m.over] = EMPTY;
    last_capture = 0;
    sth_captured = TRUE;
  } else {
    hist_dat[ply].capture2 = EMPTY;
  }

  if ( piece[(int)m.to] != EMPTY ) { /* capture */
#ifdef DEBUG  
    if ( !(m.bits & 1))
      fprintf(stderr,"#Error in make_lion_move\n");
#endif
    pieces_count[xside] --;
    captured[xside][demote(piece[(int)m.to])]++;
    hist_dat[ply].capture = piece[(int)m.to];
    if ( m.bits & 16 )  /* promotion */
      piece[(int)m.to] = m.promote;
    else
      piece[(int)m.to] = piece[(int)m.from];
    last_capture = 0;
    sth_captured = TRUE;
  } 
  color[(int)m.to] = side;
  if ( m.bits & 16 )  /* promotion */
    piece[(int)m.to] = m.promote;
  else
    piece[(int)m.to] = piece[(int)m.from];

  if ( m.bits & 2 ) { /* suicide move */
    piece[(int)m.to] = EMPTY;
    color[(int)m.to] = EMPTY;
    captured[side][demote(piece[(int)m.from])]++;
  }

  if (!sth_captured) {
    last_capture++;
  } else {
    last_capture = 0;
  }

  piece[(int)m.from] = EMPTY;
  color[(int)m.from] = EMPTY;

#ifdef DEBUG
  if ((( piece[(int)m.from] == EMPTY )&& (color[(int)m.from] != EMPTY))||
      (( piece[(int)m.from] != EMPTY )&& (color[(int)m.from] == EMPTY)))
    fprintf(stderr,"#make_lion_move1: %s\n", move_str(m));
  if ((( piece[(int)m.over] == EMPTY )&& (color[(int)m.over] != EMPTY))||
      (( piece[(int)m.over] != EMPTY )&& (color[(int)m.over] == EMPTY)))
    fprintf(stderr,"#make_lion_move2: %s\n", move_str(m));
  if ((( piece[(int)m.to] == EMPTY )&& (color[(int)m.to] != EMPTY))||
      (( piece[(int)m.to] != EMPTY )&& (color[(int)m.to] == EMPTY)))
    fprintf(stderr,"#make_lion_move3: %s\n", move_str(m));
#endif

  /* hist_dat[ply].m.b.bits &= !(64|32); */

  ++ply;
  side ^= 1; 
  xside ^= 1; 

  return TRUE;
}

BOOL make_igui( move_bytes m ) {

  /* back up information so we can take the move back later. */

  hist_dat[ply].m.b = m;
  hist_dat[ply].oldpiece = piece[(int)m.from];
  hist_dat[ply].capture = piece[(int)m.to];
  hist_dat[ply].last_capture = last_capture;
  captured[xside][demote(piece[(int)m.to])]++;
#ifdef DEBUG  
  if ((hist_dat[ply].capture != EMPTY ) && !(m.bits & 1)) {
    /* print_board(stderr); */
    fprintf(stderr,"#Error in make_igui: Capturing %s's %s, but no bit set!\n",(side ? "Gote" : "Sente"), piece_string[hist_dat[ply].capture]);
    save_game("normal-move");
  }
#endif

  if (m.bits & 16) /* promotion */
    piece[(int)m.from] = m.promote;
  else {
    if (m.bits & 2 ) {/* suicide */
      piece[(int)m.from] = EMPTY;
      color[(int)m.from] = EMPTY;
    } else
      piece[(int)m.from] = piece[(int)m.from];
  }
  if (m.bits & 1) { /* capture */
    ++last_capture;
  }
  color[(int)m.to] = EMPTY;
  piece[(int)m.to] = EMPTY;

#ifdef DEBUG
  if ((( piece[(int)m.from] == EMPTY )&& (color[(int)m.from] != EMPTY))||
      (( piece[(int)m.from] != EMPTY )&& (color[(int)m.from] == EMPTY)))
    fprintf(stderr,"##make_igui1: %s\n", move_str(m));
  if ((( piece[(int)m.to] == EMPTY )&& (color[(int)m.to] != EMPTY))||
      (( piece[(int)m.to] != EMPTY )&& (color[(int)m.to] == EMPTY)))
    fprintf(stderr, "#make_igui2: %s\n", move_str(m));
#endif

  ++ply;
  side ^= 1; 
  xside ^= 1; 

  return TRUE;
}

BOOL make_FiD( move_bytes m ) {

  int i, n;

  /* back up information so we can take the move back later. */
  hist_dat[ply].m.b = m;
  hist_dat[ply].oldpiece = piece[(int)m.from];
  hist_dat[ply].capture = piece[(int)m.to];
  hist_dat[ply].last_capture = last_capture;
#ifdef DEBUG  
  if ((hist_dat[ply].capture != EMPTY ) && !(m.bits & 1)) {
    /* print_board(stderr); */
    fprintf(stderr,"#Error in make_normal_move: Capturing %s's %s, but no bit set!\n",(side ? "Gote" : "Sente"), piece_string[hist_dat[ply].capture]);
    save_game("normal-move");
  }
#endif
  if ( piece[(int)m.to] != EMPTY ) { /* capture */
    pieces_count[xside] --;
    hist_dat[ply].capture = piece[(int)m.to];
    captured[xside][demote(piece[(int)m.to])]++;
    /* hist_dat[ply].m.b.bits |= 1; */
  } else {
    hist_dat[ply].capture = EMPTY;
  }
  color[(int)m.to] = side;
  piece[(int)m.to] = piece[(int)m.from];

  piece[(int)m.from] = EMPTY;
  color[(int)m.from] = EMPTY;

  /* now the burning stuff */
  if ( ! tame_FiD || ( hist_dat[ply].capture == EMPTY )) {
    if ( !( m.bits & 2 ) || hodges_FiD ){ /* FiD non-suicide move */
      for (i = 0; i < 8; i++) {
	n = mailbox[mailbox256[(int)m.to] + lion_single_steps[i]];
#ifdef DDEBUG
	printf("burning %d%c\n",16-GetFile(n),GetRank(n) + 'a');
#endif
	if ((n != -1)&&( color[n] == xside )) {
	  /* hist_dat[ply].m.b.bits |= 1; */
	  hist_dat[ply].burn[i] = piece[n];
	  captured[xside][demote(piece[n])]++;
	  piece[n] = EMPTY;
	  color[n] = EMPTY;
	}
      }
    }
  }

  if ( m.bits & 2 ) { /* only the FiD burns */
    fire_demons[side]--;
    captured[side][FIRE_DEMON]++;
    piece[(int)m.to] = EMPTY;
    color[(int)m.to] = EMPTY;
  }

#ifdef DEBUG
  if ((( piece[(int)m.from] == EMPTY )&& (color[(int)m.from] != EMPTY))||
      (( piece[(int)m.from] != EMPTY )&& (color[(int)m.from] == EMPTY)))
    fprintf(stderr,"##make_normal_move1: %s\n", move_str(m));
  if ((( piece[(int)m.to] == EMPTY )&& (color[(int)m.to] != EMPTY))||
      (( piece[(int)m.to] != EMPTY )&& (color[(int)m.to] == EMPTY)))
    fprintf(stderr, "#make_normal_move2: %s\n", move_str(m));
#endif

  ++ply;
  side ^= 1; 
  xside ^= 1; 


  return TRUE;
}

BOOL make_normal_move( move_bytes m ) {
  /* back up information so we can take the move back later. */
  hist_dat[ply].m.b = m;
  hist_dat[ply].oldpiece = piece[(int)m.from];
  hist_dat[ply].capture = piece[(int)m.to];
  hist_dat[ply].last_capture = last_capture;
  hist_dat[ply].over = (int)m.over; /* should not be needed */

  if (( piece[(int)m.to] != EMPTY )&& (color[(int)m.to] == xside))
    captured[xside][demote(piece[(int)m.to])]++;

#ifdef DEBUG  
  if ((hist_dat[ply].capture != EMPTY ) && !(m.bits & 1)) {
    /* print_board(stderr); */
    fprintf(stderr,"#Error in make_normal_move: Capturing %s's %s, but no bit set!\n",(side ? "Gote" : "Sente"), piece_string[hist_dat[ply].capture]);
    save_game("normal-move");
  }
#endif
  if ( piece[(int)m.to] != EMPTY ) { /* capture */
    pieces_count[xside] --;
    hist_dat[ply].capture = piece[(int)m.to];
    /* hist_dat[ply].m.b.bits |= 1; */
  } else {
    hist_dat[ply].capture = EMPTY;
  }
  color[(int)m.to] = side;
  if (( m.bits & 16 )&&(m.promote != 0)&&(m.promote != EMPTY))  /* promotion */
    piece[(int)m.to] = m.promote;
  else
    piece[(int)m.to] = piece[(int)m.from];

  if ( m.bits & 2 ) { /* suicide move */
    piece[(int)m.to] = EMPTY;
    color[(int)m.to] = EMPTY;
    captured[side][demote(piece[(int)m.from])]++;
  }

  piece[(int)m.from] = EMPTY;
  color[(int)m.from] = EMPTY;

#ifdef DEBUG
  if ((( piece[(int)m.from] == EMPTY )&& (color[(int)m.from] != EMPTY))||
      (( piece[(int)m.from] != EMPTY )&& (color[(int)m.from] == EMPTY)))
    fprintf(stderr,"##make_normal_move1: %s\n", move_str(m));
  if ((( piece[(int)m.to] == EMPTY )&& (color[(int)m.to] != EMPTY))||
      (( piece[(int)m.to] != EMPTY )&& (color[(int)m.to] == EMPTY)))
    fprintf(stderr, "#make_normal_move2: %s\n", move_str(m));
#endif

  ++ply;
  side ^= 1; 
  xside ^= 1; 

  return TRUE;
}


/* reps() returns the number of times that the current
   position has been repeated. Thanks to John Stanback
   for this clever algorithm. */

int reps()
{
	int i;
	int b[NUMSQUARES];
	int c = 0;  /* count of squares that are different from
				   the current position */
	int r = 0;  /* number of repetitions */

	/* is a repetition impossible? */
	if (last_capture <= 3)
		return 0;

	for (i = 0; i < NUMSQUARES; ++i)
		b[i] = 0;

	/* loop through the reversible moves */
	/* this move first */
	if ( ++b[hist_dat[0].m.b.from] == 0 ) --c;
	else ++c;
	if ( --b[hist_dat[0].m.b.to] == 0 ) --c;
	else ++c;
	for (i = 0; i < last_capture; i++) {
	  if (i >= 400) break;
		if (++b[undo_dat[i].m.b.from] == 0)
			--c;
		else
			++c;
		if (--b[undo_dat[i].m.b.to] == 0)
			--c;
		else
			++c;
		if (c == 0)
			++r;
	}

	return r;
}


/* takeback() is very similar to makemove(), only backwards :)  */

void takeback()
{
  move_bytes m;
  
  side ^= 1; 
  xside ^= 1;  /* we've switched sides */
  --ply;       
  m = hist_dat[ply].m.b;
  /* fprintf( stderr,"#tb %s ", move_str(m) ); */
  last_capture = hist_dat[ply].last_capture;
  if ( m.bits & 2 ) { /* suicide move */
    captured[side][demote(hist_dat[ply].oldpiece)]--;
  }
  if ( m.bits & 4 ) /* lion double step */
    take_back_lion_move(m);
  else if ( m.bits & 32 ) /* igui */
    take_back_igui(m);
  else if ( m.bits & 64 ) /* FiD move */
    take_back_FiD(m);
  else {
    take_back_normal(m);
  }
}

void take_back_lion_move(move_bytes m) {

  piece[(int)m.from] = hist_dat[ply].oldpiece;
  color[(int)m.from] = side;
  
  last_capture = hist_dat[ply].last_capture;

  if (hist_dat[ply].capture == EMPTY) { /* no capture */
    color[(int)m.to] = EMPTY;
    piece[(int)m.to] = EMPTY;
  } else if ( m.bits & 1 ) {
    color[(int)m.to] = xside;
    piece[(int)m.to] = hist_dat[ply].capture;
    captured[xside][demote(piece[(int)m.to])]--;
  } else {
    fprintf(stderr,"#Error 1 in take_back_lion_move\n");
  }

  if (hist_dat[ply].capture2 == EMPTY) { /* no capture on over field */
    color[(int)m.over] = EMPTY;
    piece[(int)m.over] = EMPTY;
  } else if ( m.bits & 1 ) {
    color[(int)m.over] = xside;
    piece[(int)m.over] = hist_dat[ply].capture2;
    captured[xside][demote(piece[(int)m.over])]--;
  } else { /* capture2 not empty, but bit 1 not set, and bit 4
	      should not have been set */
    fprintf(stderr,"#Error 2 in take_back_lion_move\n");
  }
  return;
}

void take_back_igui(move_bytes m) {

  piece[(int)m.from] = hist_dat[ply].oldpiece;
  color[(int)m.from] = side;

  last_capture = hist_dat[ply].last_capture;


  if (hist_dat[ply].capture == EMPTY) { /* no capture */
    color[(int)m.to] = EMPTY;
    piece[(int)m.to] = EMPTY;
  } else if ( m.bits & 1 ) {
    color[(int)m.to] = xside;
    piece[(int)m.to] = hist_dat[ply].capture;
    captured[xside][demote(piece[(int)m.to])]--;
  } else {
    /* print_board(stderr); */
    fprintf(stderr,"#Error in take_back_igui: capture: %d\n", hist_dat[ply].capture);
  }
  return;
}

void take_back_FiD(move_bytes m) {
  int i, n;

  piece[(int)m.from] = hist_dat[ply].oldpiece;
  color[(int)m.from] = side;

  last_capture = hist_dat[ply].last_capture;

  for (i = 0; i < 8; i++) {
    n = mailbox[mailbox256[(int)m.to] + lion_single_steps[i]];
#ifdef DDEBUG
    printf("restoring %d%c\n",16-GetFile(n),GetRank(n) + 'a');
#endif
    if ((n != -1)&&(hist_dat[ply].burn[i] != EMPTY )) {
      piece[n] = hist_dat[ply].burn[i];
      color[n] = xside;
      captured[xside][demote(piece[n])]--;
      }
    }

  if (hist_dat[ply].capture == EMPTY) { /* no capture */
    color[(int)m.to] = EMPTY;
    piece[(int)m.to] = EMPTY;
  } else if ( m.bits & 1 ) {
    color[(int)m.to] = xside;
    piece[(int)m.to] = hist_dat[ply].capture;
    captured[xside][demote(piece[(int)m.to])]--;
  } else {
    /* print_board(stderr); */
    fprintf(stderr,"#Error in take_back_FiD\n");
  }
  return;
}

void take_back_normal(move_bytes m) { /* can be capture or not */

  piece[(int)m.from] = hist_dat[ply].oldpiece;
  color[(int)m.from] = side;
  if ((m.bits & 16) && ( hist_dat[ply].oldpiece == WATER_BUFFALO ))
     fire_demons[side]--;

  last_capture = hist_dat[ply].last_capture;


  if (hist_dat[ply].capture == EMPTY) { /* no capture */
    color[(int)m.to] = EMPTY;
    piece[(int)m.to] = EMPTY;
  } else if ( m.bits & 1 ) {
    color[(int)m.to] = xside;
    piece[(int)m.to] = hist_dat[ply].capture;
    captured[xside][demote(piece[(int)m.to])]--;
    if ( hist_dat[ply].capture == FIRE_DEMON ) {
      fire_demons[xside]++;
    }
  } else {
    /* print_board(stderr); */
    fprintf(stderr,"#Error in take_back_normal: capture: %s, %s\n", 
	    piece_string[hist_dat[ply].capture], half_move_str(m));
  }
}



void print_line( move_bytes m ) {
  int i;
  fprintf( stderr,"#takeback: " );
  for ( i= 0; i <= ply; i++ ) {
    fprintf( stderr,"# %s ", move_str( hist_dat[i].m.b ));
  } 
  fprintf( stderr, "# %s", move_str(m) );  
}


/* in_check() returns TRUE if side s is in check and FALSE
   otherwise. It just scans the board to find side s's king
   and calls attack() to see if it's being attacked. */

BOOL in_check(int s)
{
	int i;
	int cp_square, king_square;
	BOOL cp, king;

	/* return FALSE; */

	cp = FALSE; /* no cp */
	king =  FALSE; /* no king */

	for (i = 0; i < NUMSQUARES; ++i) {
	  if (color[i] == s ) {
	    if ( piece[i] == CROWN_PRINCE ) {
	      cp_square = i;
	      cp = TRUE;
	    }
	    if ( piece[i] == KING ) {
	      king_square = i;
	      king = TRUE;
	    }
	  }
	}
	if ( king && cp ) 
	  return FALSE; /* check disabled */
	if ( ! cp ) 
	  return attack(king_square, s ^ 1);
	if ( ! king )
	  return attack(cp_square, s ^ 1);

	return TRUE;  /* shouldn't get here */
}


/* attack() returns TRUE if square sq is being attacked by side
   s and FALSE otherwise. */

BOOL attack(int sq, int s)
{
  int i, j, n, oldn, idx;
  int first, sec;
  int steps; /* steps into single direction */
  int var_board[NUMSQUARES];
  int save_piece[NUMSQUARES];
  int save_color[NUMSQUARES];

  int orig_side = side;
  BOOL sides_swapped = FALSE;

  if ( s != orig_side ) {
    side ^= 1;
    xside ^= 1;
    sides_swapped = TRUE;
    gen();
  }

  /* init var_board */
  for ( i = 0; i < NUMSQUARES; i++)
    var_board[i] = 0;

  /* save current board */
  for ( i = 0; i < NUMSQUARES; i++) {
    save_piece[i] = piece[i];
    save_color[i] = color[i];
  }

  for (n = gen_begin[ply]; n < gen_end[ply]; ++n) {
    if ( gen_dat[n].m.b.to == sq ) {
      /* restore old board */
      for ( j = 0; j < NUMSQUARES; j++) {
	piece[j] = save_piece[j];
	color[j] = save_color[j];
      }
      if ( sides_swapped ) {
	side ^= 1;
	xside ^= 1;
	sides_swapped = FALSE;      
	gen();
      }
      return( TRUE );
      /* or if it's a fire demon not commiting suicide and the opponent's
	 K or CP next to it */
    } else if (( piece[gen_dat[n].m.b.from] == FIRE_DEMON) &&
		 (!suicide(gen_dat[n].m.b.to)||hodges_FiD)) {
	/* check if there's an opponent's K or CP */
      /*
      fprintf(stderr, "FiD move: %d-%d",gen_dat[n].m.b.from,gen_dat[n].m.b.to);
      */
      for (i = 0; i < 8; i++) {
	j = mailbox[mailbox256[gen_dat[n].m.b.to] + lion_single_steps[i]];
	if (j == sq ) {
	  /* restore old board */
	  /* fprintf(stderr, "found square: %d\n", j); */
	  for ( j = 0; j < NUMSQUARES; j++) {
	    piece[j] = save_piece[j];
	    color[j] = save_color[j];
	  }
	  if ( sides_swapped ) {
	    side ^= 1;
	    xside ^= 1;
	    sides_swapped = FALSE;      
	    gen();
	  }
	  return TRUE;
	}
      }
    }
  }

      
  for ( j = 0; j < NUMSQUARES; j++) {
    piece[j] = save_piece[j];
    color[j] = save_color[j];
  }

  if ( sides_swapped ) {
    side ^= 1;
    xside ^= 1;
    sides_swapped = FALSE;
    gen();
  }

  return FALSE;
}



/* gen_checks() generates pseudo-legal moves for the current position.
   It scans the board to find friendly pieces and then determines
   what squares they attack. When it finds a piece/square
   combination, it calls gen_push to put the move on the "move
   stack, provided it's a check." NOT WORKING */

void gen_checks()
{
	int i, j, n;
	int steps; /* steps into single direction */
	/* so far, we have no moves for the current ply */
	gen_end[ply] = gen_begin[ply];
	
	if ( lost(side)) return;

	for (i = 0; i < NUMSQUARES; ++i)
		if (color[i] == side) {
		  if ( move_types[piece[i]][8] != none ) /* special move piece */
		    special_moves( i, color[i] );
		  for (j = 0; j < offsets[piece[i]]; ++j) {
		    steps = get_steps(i,j, color[i]);
		    for (n = i;;) { /* stepping into a single direction */
		      if ( steps <= 0 )
			break;
		      n = mailbox[mailbox256[n] + offset[side][piece[i]][j]];
		      if (n == -1)
			break;
		      if (color[n] != EMPTY) {
			if (color[n] == xside) {
			  if ( can_promote(i,n) ) {
			    gen_push(i, n, 17);
			    if ( ! must_promote(i,n) )
			      gen_push(i, n, 1);
			  } else 
			    gen_push(i,n,1);
			}
			break;
		      } else { /* color == side + EMPTY */
			if ( can_promote(i,n) ) {
			  gen_push(i, n, 16);
			  if ( ! must_promote(i,n) ) gen_push(i, n, 0);
			} else {
			  if ( burning(i,n) )
			    gen_push(i,n,65);
			  else 
			    gen_push(i, n, 0);
			}
		      }
		      steps--;
		    }
		  }
		}

	/* the next ply's moves need to start where the current
	   ply's end */
	gen_begin[ply + 1] = gen_end[ply];
}

int demote ( int this_piece ) {
  int i;
  for ( i = 0; i < PIECE_TYPES; i++ ) {
    if ( this_piece && ( this_piece == promotion[i] ))
      return i;
  }
  return this_piece;
}
