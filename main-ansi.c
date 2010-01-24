/*
 *	MAIN.C
 *      E.Werner 2000, 2010
 *      derived from
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */

unsigned char tenjiku_version[] = { "0.55" };

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

unsigned char *side_name[2] = { "Sente", "Gote" };
int computer_side;
char TeXoutputpath[1024] = "exports";
char savegamepath[1024] = "savegames";
char positionpath[1024] = "positions";
char positionfile[1024];

BOOL search_quiesce = TRUE;
BOOL jgs_tsa = FALSE; /* tsa rule by default off */
BOOL modern_lion_hawk = TRUE; /* tsa rule by default off */
BOOL hodges_FiD = FALSE;
BOOL tame_FiD = FALSE; /* experimental: 
			  FiD only burns when it doesn't capture */
BOOL verytame_FiD = FALSE; /* experimental: 
			  FiD only burns, cannot capture */
BOOL burn_all_FiD= FALSE; /* experimental NOT IMPLEMENTED
			     FiD burns also friendly pieces */
BOOL show_hypothetical_moves = TRUE;
/* shows hypothetical moves (possible on empty board) */

BOOL kanji = TRUE; /* kanji output for terminal */
/* get_ms() returns the milliseconds elapsed since midnight,
   January 1, 1970. */
BOOL fullkanji = FALSE;

BOOL loaded_position = FALSE; /*necessary to provide information to save_game - not yet implemented */

#include <sys/timeb.h>

#include <readline/readline.h>
/* A static variable for holding the line. */
static char *line_read = (char *)NULL;

/* Read a string, and return a pointer to it.
   Returns NULL on EOF. */
char *rl_gets() {
       /* If the buffer has already been allocated,
          return the memory to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = (char *)NULL;
    }
  
  /* Get a line from the user. */
  line_read = readline ("Enter command (? for help)> ");
  
  /* If the line has any text in it,
          save it on the history. */
  if (line_read && *line_read)
    add_history (line_read);
  
  return (line_read);
}


long get_ms()
{
	struct timeb timebuffer;
	ftime(&timebuffer);
	return (timebuffer.time * 1000) + timebuffer.millitm;
}


/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int main(int argc,  char **argv)
{
	char *s = "                                                                                              ";
	int from, over, to, i;
	int from_file, to_file;
	int over_file;
       
	char from_rank, over_rank, to_rank, igui, igui2, prom, *last_move_ptr;
	char last_move[15], before_last_move[1024], last_move_buf[128]; /* to prevent overflowing from nonsense */
	BOOL found, promote, found_in_book;

	printf("\n");
	printf("Tenjiku Shogi %s by E.Werner\n", tenjiku_version);
	printf("   derived from\n");
	printf("Tom Kerrigan's Simple Chess Program (TSCP)\n");
	printf("version 1.52, 6/29/00\n");
	printf("\n");
	printf("\"help\" or \"?\" displays a list of commands.\n");
	printf("\n");

	init();
	book = book_init();
	gen();
	computer_side = EMPTY;
	max_time = 1000000;
	read_init_file();
	process_arguments(argc, argv);
#ifdef CENTIPLY
	max_depth = 400;
#else
	max_depth = 4;
#endif
	strcpy(before_last_move,"");
	for (;;) {

	  found = FALSE;
	  promote = FALSE;
		if ((side == computer_side)|| (computer_side == BOTH )) {  /* computer's turn */
		  /* have a look at the opening book first */
		  if ( have_book) {
		    found_in_book = in_book(before_last_move, last_move_buf);
		    if ( found_in_book ) {
		      if ( 9 == sscanf(last_move_buf,"%d%c%c%d%c%c%d%c%c",&from_file, &from_rank, &igui,
				       &over_file, &over_rank, &igui2, &to_file, &to_rank, &prom) ) {
			if ( prom == '+' ) promote = TRUE;
		      }
		      if ( 8 == sscanf(last_move_buf,"%d%c%c%d%c%c%d%c",&from_file, &from_rank, &igui,
				       &over_file, &over_rank, &igui2, &to_file, &to_rank) ) {
			from = 16 - from_file;
			from += 16 * (from_rank - 'a');
			over = 16 - over_file;
			over += 16 * (over_rank - 'a');		  
			to = 16 - to_file;
			to += 16 * (to_rank - 'a');
		      
			/* loop through the moves to see if it's legal */
			found = FALSE;
			for (i = 0; i < gen_end[ply]; ++i) {
			  if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
			    continue;
			  if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
			    continue;
			  if (gen_dat[i].m.b.bits & 32)
			    continue;
			  if ( ! (gen_dat[i].m.b.bits & 4))
			    continue;
			  if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to  && gen_dat[i].m.b.over == over) {
			    found = TRUE;
			    break;
			  }
			} 
		      } else {
			promote = FALSE;
			if ( 6 == sscanf(last_move_buf,"%d%c%c%d%c%c",&from_file, &from_rank, &igui,
					 &to_file, &to_rank, &prom) ) 
			  if ( prom == '+' ) promote = TRUE;
			if ( 5 == sscanf(last_move_buf,"%d%c%c%d%c",&from_file, &from_rank, &igui,
					 &to_file, &to_rank) ) {
			  from = 16 - from_file;
			  from += 16 * (from_rank - 'a');
			  to = 16 - to_file;
			  to += 16 * (to_rank - 'a');
			
			  found = FALSE;
			  for (i = 0; i < gen_end[ply]; ++i) {
			    if ( gen_dat[i].m.b.bits & 4) continue;
			    if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
			      continue;
			    if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
			      continue;
			    if ( igui != '!' && (gen_dat[i].m.b.bits & 32))
			      continue;
			    if ( igui == '!' && ! (gen_dat[i].m.b.bits & 32))
			      continue;
			    if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) {
			      found = TRUE;
			      break;
			    }
			  }
			}
		      }
		      if (!found || !makemove(gen_dat[i].m.b)) {
			printf("Illegal move %s! Error in book!\n", last_move_buf);
			fflush(stdout);
			have_book = FALSE; /* should be already set, but ... */
		      } else {
			redos=0; /* new line, so clear old redo stack */
			print_board(stdout);
			/* printf("Computer's move: %s\n", last_move_buf);*/
			printf("Computer's move: %s\n", half_move_str(gen_dat[i].m.b));
		      }
		    } else {
		      have_book = FALSE; /* should be already set, but ... */
		      continue;
		    }/* if (last_move ); */
		  } else {
		    /* think about the move and make it */

		    think(FALSE);
		    if ( !pv[0][0].u ) {
		      printf("%s loses\n", side_name[side] );
		      computer_side = EMPTY;
		      continue;
		    }
		    strcpy(last_move,move_str(pv[0][0].b));
		    makemove(pv[0][0].b);
		    redos=0;  /* new line, so clear old redo stack */
		    print_board(stdout);
		    /* printf("Computer's move: %s\n", last_move);*/
		    printf("Computer's move: %s\n", half_move_str(pv[0][0].b));
		  }
		  fflush(stdout);
		  /* save the "backup" data because it will be overwritten */
		  undo_dat[undos] = hist_dat[0];
		  ++undos;
		    
		  ply = 0;
		  gen();
		  continue;
		}
		/* get user input */
		/* printf("%s> ", argv[0]);
		fflush(stdout);
		scanf("%s", s);
		*/
		s = rl_gets();
		if (!strcmp(s, "quiesce")) {
		  if ( search_quiesce ) {
		    search_quiesce = FALSE;
		    printf("No quiesce search\n");
		  } else {
		    search_quiesce = TRUE;
		    printf("quiesce search on\n");
		  }
			continue;
		}
		if (!strcmp(s, "auto")) {
		  computer_side = BOTH;
		  continue;
		}
 		if (!strcmp(s, "save")|| !strcmp(s,"s")) {
		  save_game("");
		  continue;
		}
 		if (!strcmp(s, "savepos")|| !strcmp(s,"spos")) {
		  save_position("");
		  continue;
		}
 		if (!strcmp(s, "loadpos")|| !strcmp(s,"lpos")) {
		  load_position(NULL);
		  print_board( stdout );
		  continue;
		}
		if (!strcmp(s, "load")|| !strcmp(s,"l")) {
		  load_game("");
		  continue;
		}
		if (!strcmp(s, "loadold")|| !strcmp(s,"l")) {
		  load_old_game("");
		  continue;
		}
		if (!strcmp(s, "setup")) {
		  setup_board();
		  print_board( stdout );
		  fflush(stdout);
		  continue;
		}
		if (!strcmp(s, "ascii")) {
		  kanji=FALSE;
		  fullkanji = FALSE;
		  print_board( stdout );
		  continue;
		}
		if (!strcmp(s, "kanji")) {
		  kanji=TRUE;
		  fullkanji = FALSE;
		  print_board( stdout );
		  continue;
		}
		if (!strcmp(s, "fullkanji")) {
		  fullkanji=TRUE;
		  kanji = FALSE;
		  print_board( stdout );
		  continue;
		}
		if (!strcmp(s, "tsa")) {
		  printf("Jumping Generals jump and jump-capture according to hierarchy (TSA).\n");
		  jgs_tsa = TRUE;
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "ew")) {
		  printf("Jumping Generals jump-capture anything (Eduard Werner).\n");
		  fflush(stdout);
		  have_book = FALSE;
		  jgs_tsa = FALSE;
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "classic")) {
		  printf("Lion Hawk as two-step area mover (TSA).\n");
		  fflush(stdout);
		  modern_lion_hawk = FALSE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "modern")) {
		  printf("Lion Hawk with Lion Power (Colin Paul Adams).\n");
		  fflush(stdout);
		  modern_lion_hawk = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "hodges")) {
		  printf("Fire Demon suicide also burns enemy pieces (George Hodges).\n");
		  hodges_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}

		if (!strcmp(s, "hypo")) {
		  printf("m(oves) shows hypothetical moves, too\n");
		  show_hypothetical_moves = TRUE;
		  continue;
		}
		if (!strcmp(s, "nohypo")) {
		  printf("m(oves) doesn't show hypothetical moves\n");
		  show_hypothetical_moves = FALSE;
		  continue;
		}		

		if (!strcmp(s, "burn")) {
		  printf("Fire Demon also burns friendly pieces (Eduard Werner).\n");
		  hodges_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "noburn")) {
		  printf("Fire Demon doesn't burn friendly pieces (Eduard Werner).\n");
		  hodges_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "tame")) {
		  printf("Fire Demon only burns if it doesn't capture.\n");
		  tame_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "verytame")) {
		  printf("Fire Demon only burns, but cannot capture.\n");
		  verytame_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "notame")) {
		  printf("Fire Demon burns also on capture.\n");
		  tame_FiD = FALSE;
		  verytame_FiD = FALSE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "hodges")) {
		  printf("Fire Demon suicide also burns enemy pieces (George Hodges).\n");
		  hodges_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "fid")) {
		  printf("Fire Demon suicide does not burn anything (TSA).\n");
		  fflush(stdout);
		  hodges_FiD = FALSE;
		  tame_FiD = FALSE;
		  verytame_FiD = FALSE;
		  book = book_init();
		  init();
		  gen();
		  continue;
		}
		if (!strcmp(s, "on")) {
			computer_side = side;
			continue;
		}
		if (!strcmp(s, "off")) {
			computer_side = EMPTY;
			continue;
		}
		if (!strcmp(s, "moves")|| !strcmp(s,"m")) {
		  show_moves();
		  continue;
		}
		if (!strcmp(s, "inf") || !strcmp(s,"i")) {
		  show_influence();
		  continue;
		}
		if (!strcmp(s, "check")) {
		  show_checks();
		  continue;
		}
		if (!strcmp(s, "st")) {
			scanf("%d", &max_time);
			max_time *= 1000;
			max_depth = 100000;
			continue;
		}
		if (!strcmp(s, "sd")) {
		  scanf("%d", &max_depth);
#ifdef CENTIPLY
		  max_depth *= 100;
#endif
			max_time = 1000000;
			continue;
		}
		if (!strcmp(s, "goto") || !strcmp(s,"g")) {
		  goto_position();
		  print_board(stdout);
		  continue;
		}
		if (!strcmp(s, "TeX") || !strcmp(s,"tex")) {
		  export_TeX();
		  continue;
		}
		if (!strcmp(s, "undo") || !strcmp(s,"u")) {
		  if (!undos) {
		    fprintf(stderr, "No more undos\n");
				continue;
		  }
		  computer_side = EMPTY;
		  undo_move();
		  print_board(stdout);
		  continue;
		}
		if (!strcmp(s, "redo") || !strcmp(s,"r")) {
		  if (!redos) {
		    fprintf(stderr, "No more redos\n");
				continue;
		  }
		  computer_side = EMPTY;
		  redo_move();
		  gen();
		  print_board(stdout);
		  continue;
		}
		if (!strcmp(s, "new")) {
			computer_side = EMPTY;
			init();
			strcpy(before_last_move,"");
			book = book_init();
			gen();
			continue;
		}
		if (!strcmp(s, "d")) {
			print_board(stdout);
			continue;
		}
		if (!strcmp(s, "bye") || !strcmp(s, "quit")) {
			printf("Share and enjoy!\n");
			exit(0);
		}
		if (!strcmp(s, "create")) {
		  book_create("tenjiku.book.in");
		  book = book_init();
			continue;
		}
		if (!strcmp(s, "hint")) {
			think(TRUE);
			if (!pv[0][0].u)
				continue;
			printf("Hint: %s\n", move_str(pv[0][0].b));
			fflush(stdout);
			continue;
		}
		if (!strcmp(s, "help") || !strcmp(s,"?")) {
		        printf("------- CURRENT RULES  ----------\n");
			printf("Fire Demon suicide move burns enemy pieces: %s\n",(hodges_FiD ? "on" : "off"));
			printf("Fire Demon also burns friendly pieces: %s\n",(burn_all_FiD ? "on" : "off"));
			printf("Fire Demon only burns when it doesn't capture: %s\n",(tame_FiD ? "on" : "off"));
			printf("Fire Demon only burns, but cannot capture: %s\n",(verytame_FiD ? "on" : "off"));
			printf("Modern LHk with Lion power: %s\n",(modern_lion_hawk ? "on" : "off"));
			printf("Jumping Generals jump-capture anything: %s\n",(jgs_tsa ? "off" : "on"));
		        printf("------- COMPUTER PLAYER ---------\n");
			printf("auto - computer plays both sides\n");		  
			printf("on - computer plays for the side to move\n");
			printf("off - computer stops playing\n");
			printf("st n - search for n seconds per move\n");
			printf("sd n - search n plies deep per move\n");
			printf("quiesce - toggle quiesce search\n");
			printf("------- PLAYING THE GAME --------\n");
			printf("new - starts a new game\n");
			printf("Enter moves, e.g., 2f-2e, 3e-3f+, 4b!4c, 4bx3c-3d, 7k\n");
			printf("m(oves) - ask for a square and shows moves of the piece\n");
			printf("hypo - m(oves) shows hypothetical moves, too\n");
			printf("nohypo - m(oves) doesn't show hypothetical moves\n");
			printf("i(nf) - ask for a square and shows pieces that can move there\n");
			/* printf("check - shows possible checks\n"); */
			printf("u(ndo) - takes back a move\n");
			printf("r(edo) - puts back a move\n");
			printf("g(oto) - goto position after move nr\n");
			printf("------- RULES VARIANTS ----------\n");
			printf("classic - starts a new game with TSA LHk (default)\n");
			printf("modern - starts a new game with Modern Lion Hawk (C.P.Adams)\n");
			printf("ew - starts a new game with changed JGns rule (E.Werner)\n");
			printf("tsa - starts a new game with TSA JGns (default)\n");
			printf("hodges - starts a new game, FiD suicide burns enemy pieces (G.Hodges)\n");
			printf("burn - starts a new game, FiD burns also friendly pieces (E.Werner)\n");
			printf("tame - (notame) starts a new game, FiD only burns when it doesn't capture (E.Werner)\n");
			printf("verytame - (notame) starts a new game, FiD only burns, but doesn't capture (E.Werner)\n");
			printf("fid - starts a new game, only FiD suicide only burns own FiD (TSA rule, default)\n");
			printf("------------ I/O ----------------\n");
			printf("create - create a new opening file - please read the comments in book.dat!\n");
			printf("d - display the board\n");
			printf("ascii - display simple ascii board\n");
			printf("kanji - display kanji board with one kanji per piece\n");
			printf("fullkanji - display kanji board with two kanjis per piece\n");
			printf("l(oad) - load a game file\n");
			/* printf("replay - load a game file and replay game\n"); */
			printf("s(ave) - save the current game to a file\n");
			printf("lpos - load game position files\n");
			printf("spos - save position to game position files\n");
			printf("tex or TeX - export position to LaTeX file\n");
			printf("bye/quit  - exit the program\n");
			fflush(stdout);
			continue;
		}

		/* maybe the user entered a move? */
		promote = FALSE;

		if ( 9 == sscanf(s,"%d%c%c%d%c%c%d%c%c",&from_file, &from_rank, &igui,
				 &over_file, &over_rank, &igui2, &to_file, &to_rank, &prom) ) {
		  if ( prom == '+' ) promote = TRUE;
		}
		if ( 8 == sscanf(s,"%d%c%c%d%c%c%d%c",&from_file, &from_rank, &igui,
				 &over_file, &over_rank, &igui2, &to_file, &to_rank) ) {
		  from = 16 - from_file;
		  from += 16 * (from_rank - 'a');
		  over = 16 - over_file;
		  over += 16 * (over_rank - 'a');		  
		  to = 16 - to_file;
		  to += 16 * (to_rank - 'a');

		  /* loop through the moves to see if it's legal */
		  found = FALSE;
		  for (i = 0; i < gen_end[ply]; ++i) {
		    if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
		      continue;
		    if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
		      continue;
		    if (gen_dat[i].m.b.bits & 32)
		      continue;
		    if ( ! (gen_dat[i].m.b.bits & 4))
		      continue;
		    if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to  && gen_dat[i].m.b.over == over) {
		      found = TRUE;
		      break;
		    }
		  } 
		} else {
		  promote = FALSE;
		  if ( 6 == sscanf(s,"%d%c%c%d%c%c",&from_file, &from_rank, &igui,
				   &to_file, &to_rank, &prom) ) 
		    if ( prom == '+' ) promote = TRUE;
		  if ( 5 == sscanf(s,"%d%c%c%d%c",&from_file, &from_rank, &igui,
				   &to_file, &to_rank) ) {
		    from = 16 - from_file;
		    from += 16 * (from_rank - 'a');
		    to = 16 - to_file;
		    to += 16 * (to_rank - 'a');
		    
		    found = FALSE;
		    for (i = 0; i < gen_end[ply]; ++i) {
		      if ( gen_dat[i].m.b.bits & 4) continue;
		      if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
			continue;
		      if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
			continue;
		      if ( igui != '!' && (gen_dat[i].m.b.bits & 32))
			continue;
		      if ( igui == '!' && ! (gen_dat[i].m.b.bits & 32))
			continue;
		      if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) {
			found = TRUE;
			break;
		      }
		    } /* for */
		  } /* if 5== sscanf */
		  else {
		    promote = FALSE;
		    if (( 3 == sscanf(s, "%d%c%c", &from_file, &from_rank, &prom ))||
			( 2 == sscanf(s, "%d%c", &from_file, &from_rank, 0 ))){
		      /* short notation like 
			 3d move or capture on 3d
			 7l+ move piece on or to 7l with promotion
			 3dx capture something with piece on 3d
			 would be nice to have sth like HFxRGn as well
		      */
		      i = identifymove(from_file, from_rank, prom);
		      if (i > 0 ) { found = TRUE; }
		    }
		  } /* else */
		  if (!found || !makemove(gen_dat[i].m.b)) {
		    printf("Illegal move.\n");
		    fflush(stdout);
		  } else {
		    /* save the "backup" data because it will be overwritten */
		    strcpy(before_last_move,s);
		    undo_dat[undos] = hist_dat[0];
		    ++undos;
		    redos=0;  /* new line, so clear old redo stack */
		    print_board(stdout);
		    ply = 0;
		    gen();
		  }
		}
	}
}


/* move_str returns a string with move m in coordinate notation */

unsigned char *move_str(move_bytes m)
{
  static unsigned char str[25];
  char igui, igui2 = '-';

  if (m.bits & 32) /* igui move */
    igui = '!';
  else 
    if (m.bits & 1) igui = 'x';
    else igui = '-';
  if (m.bits & 4) { /* lion double move */
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%s%d%c%c%d%c%c%d%c+", piece_string[piece[(int)m.from]],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a');
    else  
      sprintf(str, "%s%d%c%c%d%c%c%d%c", piece_string[piece[(int)m.from]],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a');
  } else {
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%s%d%c%c%d%c+",  piece_string[piece[(int)m.from]],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a');
    else
    sprintf(str, "%s%d%c%c%d%c",  piece_string[piece[(int)m.from]],
	    16 - GetFile(m.from),
	    GetRank(m.from) + 'a',
	    igui,
	    16 - GetFile(m.to),
	    GetRank(m.to) + 'a');

  }
  return str;
}

/* half_move_str returns a string with move m in coordinate notation w.o. piece */

unsigned char *half_move_str(move_bytes m)
{
  static unsigned char str[15];
  char igui, igui2 = '-';
  if (m.bits & 32) /* igui move */
    igui = '!';
  else 
    if (m.bits & 1) igui = 'x';
    else igui = '-';
  if (m.bits & 4) {
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%d%c%c%d%c%c%d%c+",
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a');
    else  
      sprintf(str, "%d%c%c%d%c%c%d%c",
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a');
  } else {
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%d%c%c%d%c+",
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a');
    else
    sprintf(str, "%d%c%c%d%c",
	    16 - GetFile(m.from),
	    GetRank(m.from) + 'a',
	    igui,
	    16 - GetFile(m.to),
	    GetRank(m.to) + 'a');

  }
  return str;
}


/* print_board prints the board */

void print_board( FILE *fd ) {
  if ( fullkanji )
    full_kanji_print_board( fd );
  else if ( kanji ) {
    kanji_print_board( fd );
  } else {
    ascii_print_board( fd );
  }
  if ( lost( side )) {
    fprintf( stdout, "You have lost the game!\n");
  } else if ( in_check( side )) {
    fprintf( stdout, "You're in CHECK!\n");
  }

}


void ascii_print_board( FILE *fd )
{
  
  int i;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */
  /* get the last move, should be in undo_dat[undos] */
  if ( undos ) {
    from_square = undo_dat[undos-1].m.b.from;
    to_square = undo_dat[undos-1].m.b.to;
  } else {
    from_square = -1;
    to_square = -1;
  }
  if ( redos ) {
    next_from = redo_dat[redos-1].m.b.from;
    next_to = redo_dat[redos-1].m.b.to;
  } else {
    next_from = -1;
    next_to = -1;
  }

  fprintf( fd,"\e[0m\n\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n");
  fprintf( fd,"  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    switch (color[i]) {
    case EMPTY:
      if (( i == from_square )||( i == to_square )) {
	fprintf( fd, "|\e[43m   \e[0m");
      } else {
	fprintf( fd,"|   ");
      }
      break;
    case LIGHT:
      if (( i == from_square )||( i == to_square )) {
	fprintf( fd,"|\e[43m\e[1m%s\e[0m", piece_string[piece[i]]);
      } else {
	fprintf( fd,"|\e[1m%s\e[0m", piece_string[piece[i]]);
      }
      break;
    case DARK:
      if (( i == from_square )||( i == to_square )) {
	fprintf( fd,"|\e[43m%s\e[0m", piece_string[piece[i]]);
      } else {
	fprintf( fd,"|%s", piece_string[piece[i]]);
      }
      break;
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      fprintf( fd,"| %c\n  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  fprintf( fd,"| p\n  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");

  if ( undos ) {
    if ( undo_dat[undos-1].m.b.over != EMPTY ) {
      fprintf( fd,"%s to move (last move %d: %s from %d%c over %d%c to %d%c)\n", 
	       side_name[side],undos, piece_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(undo_dat[undos-1].m.b.from), undo_dat[undos-1].m.b.from/16 + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.over),  undo_dat[undos-1].m.b.over/16 + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.to), undo_dat[undos-1].m.b.to/16 + 'a');
    } else {      
      fprintf( fd,"%s to move (last move %d: %s from %d%c to %d%c)\n", 
	       side_name[side],undos, piece_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(undo_dat[undos-1].m.b.from), GetRank(undo_dat[undos-1].m.b.from) + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.to),  GetRank(undo_dat[undos-1].m.b.to) + 'a');
    }
  } else {
    fprintf( fd,"%s to move\n", 
	     side_name[side]);
  }

  if ( redos ) {
    if ( redo_dat[redos-1].m.b.over != EMPTY ) {
      fprintf( fd,"\t(next move %d: %s from %d%c over %d%c to %d%c)", 
	       undos+1, piece_string[redo_dat[redos-1].oldpiece],
	       16 - GetFile(redo_dat[redos-1].m.b.from), redo_dat[redos-1].m.b.from/16 + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.over),  redo_dat[redos-1].m.b.over/16 + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.to), redo_dat[redos-1].m.b.to/16 + 'a');
    } else {      
      fprintf( fd,"\t(next move %d: %s from %d%c to %d%c)", 
	       undos+1, piece_string[redo_dat[redos-1].oldpiece],
	       16 - GetFile(redo_dat[redos-1].m.b.from), GetRank(redo_dat[redos-1].m.b.from) + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.to),  GetRank(redo_dat[redos-1].m.b.to) + 'a');
    }
  }
  fprintf( fd,"\n");
  fflush(fd);
}

void kanji_print_board( FILE *fd )
{
  
  int i;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */
  /* get the last move, should be in undo_dat[undos] */
  if ( undos ) {
    from_square = undo_dat[undos-1].m.b.from;
    to_square = undo_dat[undos-1].m.b.to;
  } else {
    from_square = -1;
    to_square = -1;
  }
  if ( redos ) {
    next_from = redo_dat[redos-1].m.b.from;
    next_to = redo_dat[redos-1].m.b.to;
  } else {
    next_from = -1;
    next_to = -1;
  }

  fprintf( fd,"\e[0m\n\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n");
  fprintf( fd,"  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    switch (color[i]) {
    case EMPTY:
      if (( i == from_square )||( i == to_square )) {
	if ( i %RANKS == 0 )
	  fprintf( fd, "║\e[43m   \e[0m");
	else
	  fprintf( fd, "│\e[43m   \e[0m");
      } else {
	if ( i % RANKS == 0 )
	  fprintf( fd,"║   ");
	else
	  fprintf( fd,"│   ");
      }
      break;
    case LIGHT:
      if (( i == from_square )||( i == to_square )) {
	if ( i %RANKS == 0 )
	  fprintf( fd,"║\e[43m\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
	else
	  fprintf( fd,"│\e[43m\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
      } else {
	if ( i %RANKS == 0 )
	  fprintf( fd,"║\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
	else
	  fprintf( fd,"│\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
      }
      break;
    case DARK:
      if (( i == from_square )||( i == to_square )) {
	if ( i %RANKS == 0 )
	  fprintf( fd,"║\e[43m%s\e[0m", kanji_piece_string[piece[i]]);
	else
	  fprintf( fd,"│\e[43m%s\e[0m", kanji_piece_string[piece[i]]);
      } else {
	if ( i %RANKS == 0 )
	  fprintf( fd,"║%s", kanji_piece_string[piece[i]]);
	else
	  fprintf( fd,"│%s", kanji_piece_string[piece[i]]);
      }
      break;
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  fprintf( fd,"║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");

  if ( undos ) {
    if ( undo_dat[undos-1].m.b.over != EMPTY ) {
      fprintf( fd,"%s to move (last move %d: %s from %d%c over %d%c to %d%c)\n", 
	       side_name[side],undos, kanji_piece_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(undo_dat[undos-1].m.b.from), undo_dat[undos-1].m.b.from/16 + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.over),  undo_dat[undos-1].m.b.over/16 + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.to), undo_dat[undos-1].m.b.to/16 + 'a');
    } else {      
      fprintf( fd,"%s to move (last move %d: %s from %d%c to %d%c)\n", 
	       side_name[side],undos, kanji_piece_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(undo_dat[undos-1].m.b.from), GetRank(undo_dat[undos-1].m.b.from) + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.to),  GetRank(undo_dat[undos-1].m.b.to) + 'a');
    }
  } else {
    fprintf( fd,"%s to move\n", 
	     side_name[side]);
  }

  if ( redos ) {
    if ( redo_dat[redos-1].m.b.over != EMPTY ) {
      fprintf( fd,"\t(next move %d: %s from %d%c over %d%c to %d%c)", 
	       undos+1, kanji_piece_string[redo_dat[redos-1].oldpiece],
	       16 - GetFile(redo_dat[redos-1].m.b.from), redo_dat[redos-1].m.b.from/16 + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.over),  redo_dat[redos-1].m.b.over/16 + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.to), redo_dat[redos-1].m.b.to/16 + 'a');
    } else {      
      fprintf( fd,"\t(next move %d: %s from %d%c to %d%c)", 
	       undos+1, kanji_piece_string[redo_dat[redos-1].oldpiece],
	       16 - GetFile(redo_dat[redos-1].m.b.from), GetRank(redo_dat[redos-1].m.b.from) + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.to),  GetRank(redo_dat[redos-1].m.b.to) + 'a');
    }
  }
  fprintf( fd,"\n");
  fflush(fd);
}

void full_kanji_print_board( FILE *fd )
{
  
  int i;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */
  BOOL same_rank = FALSE; /* have to pass every rank twice for the two kanjis */
  /* get the last move, should be in undo_dat[undos] */
  if ( undos ) {
    from_square = undo_dat[undos-1].m.b.from;
    to_square = undo_dat[undos-1].m.b.to;
  } else {
    from_square = -1;
    to_square = -1;
  }
  if ( redos ) {
    next_from = redo_dat[redos-1].m.b.from;
    next_to = redo_dat[redos-1].m.b.to;
  } else {
    next_from = -1;
    next_to = -1;
  }

  fprintf( fd,"\e[0m\n\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n");
  fprintf( fd,"  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    switch (color[i]) {
    case EMPTY:
      if (( i == from_square )||( i == to_square )) {
	if ( i %RANKS == 0 )
	  fprintf( fd, "║\e[43m   \e[0m");
	else
	  fprintf( fd, "│\e[43m   \e[0m");
      } else {
	if ( i % RANKS == 0 )
	  fprintf( fd,"║   ");
	else
	  fprintf( fd,"│   ");
      }
      break;
    case LIGHT:
      if (( i == from_square )||( i == to_square )) {
	if ( i %RANKS == 0 )
	  if ( !same_rank )
	    fprintf( fd,"║\e[43m↑\e[1m%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"║\e[43m↑\e[1m%s\e[0m", lower_kanji_string[piece[i]]);
	else
	  if ( !same_rank ) 
	    fprintf( fd,"│\e[43m↑\e[1m%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m↑\e[1m%s\e[0m", lower_kanji_string[piece[i]]);
      } else {
	if ( i %RANKS == 0 )
	  if ( !same_rank )
	    fprintf( fd,"║\e[1m↑%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"║\e[1m↑%s\e[0m", lower_kanji_string[piece[i]]);
	else
	  if ( !same_rank )
	    fprintf( fd,"│\e[1m↑%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[1m↑%s\e[0m", lower_kanji_string[piece[i]]);
      }
      break;
    case DARK:
      if (( i == from_square )||( i == to_square )) {
	if ( i %RANKS == 0 )
	  if ( !same_rank )
	    fprintf( fd,"║\e[43m⇩%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"║\e[43m⇩%s\e[0m", lower_kanji_string[piece[i]]);
	else
	  if ( !same_rank ) 
	    fprintf( fd,"│\e[43m⇩%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m⇩%s\e[0m", lower_kanji_string[piece[i]]);
 
     } else {
	if ( i %RANKS == 0 )
	  if ( !same_rank )
	    fprintf( fd,"║⇩%s", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"║⇩%s", lower_kanji_string[piece[i]]);
	else
	  if ( !same_rank )
	    fprintf( fd,"│⇩%s", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"│⇩%s", lower_kanji_string[piece[i]]);
      }
      break;
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE ) {
      if ( same_rank ) {
	fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	same_rank = FALSE;
      } else {
	fprintf( fd, "║\n  ");
	i-=16;
	same_rank = TRUE;
      }
    }
    if (( i == LASTSQUARE )&& !same_rank) {
	fprintf( fd, "║\n  ");
	i-=16;
	same_rank = TRUE;	
    }
  }
  fprintf( fd,"║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");

  if ( undos ) {
    if ( undo_dat[undos-1].m.b.over != EMPTY ) {
      fprintf( fd,"%s to move (last move %d: %s%s from %d%c over %d%c to %d%c)\n", 
	       side_name[side],undos, upper_kanji_string[undo_dat[undos-1].oldpiece],
	       lower_kanji_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(undo_dat[undos-1].m.b.from), undo_dat[undos-1].m.b.from/16 + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.over),  undo_dat[undos-1].m.b.over/16 + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.to), undo_dat[undos-1].m.b.to/16 + 'a');
    } else {      
      fprintf( fd,"%s to move (last move %d: %s%s from %d%c to %d%c)\n", 
	       side_name[side],undos, 
	       upper_kanji_string[undo_dat[undos-1].oldpiece],
	       lower_kanji_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(undo_dat[undos-1].m.b.from), GetRank(undo_dat[undos-1].m.b.from) + 'a', 
	       16 - GetFile(undo_dat[undos-1].m.b.to),  GetRank(undo_dat[undos-1].m.b.to) + 'a');
    }
  } else {
    fprintf( fd,"%s to move\n", 
	     side_name[side]);
  }

  if ( redos ) {
    if ( redo_dat[redos-1].m.b.over != EMPTY ) {
      fprintf( fd,"\t(next move %d: %s%s from %d%c over %d%c to %d%c)", 
	       undos+1,
	       upper_kanji_string[undo_dat[undos-1].oldpiece],
	       lower_kanji_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(redo_dat[redos-1].m.b.from), redo_dat[redos-1].m.b.from/16 + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.over),  redo_dat[redos-1].m.b.over/16 + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.to), redo_dat[redos-1].m.b.to/16 + 'a');
    } else {      
      fprintf( fd,"\t(next move %d: %s%s from %d%c to %d%c)", 
	       undos+1,
	       upper_kanji_string[undo_dat[undos-1].oldpiece],
	       lower_kanji_string[undo_dat[undos-1].oldpiece],
	       16 - GetFile(redo_dat[redos-1].m.b.from), GetRank(redo_dat[redos-1].m.b.from) + 'a', 
	       16 - GetFile(redo_dat[redos-1].m.b.to),  GetRank(redo_dat[redos-1].m.b.to) + 'a');
    }
  }
  fprintf( fd,"\n");
  fflush(fd);
}



void load_game( void ) {
  unsigned char fn[1024], fullfn[1024];
  printf("Enter load file name: ");
  fflush(stdout);
  scanf("%s",fn);
  sprintf(fullfn,"%s/%s",savegamepath,fn);
  really_load_game( fullfn );
}

void really_load_game( char *fn ) {
  /* loads game file and init files */
  unsigned char s[25], piece[3];
  char hist_s[25];
  char setup_file[1024];
  int from, over, to, i, move_nr;
  int from_file, to_file;
  int over_file;
  int line = 0;
  char from_rank, over_rank, to_rank, igui, igui2, prom;
  BOOL promote, found;
  FILE *fh;
  fh = fopen(fn,"rb");
  if (! fh) {
    printf("%s: No such file!\n", fn );
    fflush(stdout);
    return;
  }
  do {    
    /* fscanf(fh,"%s\n",s); */
    fgets(s,25,fh);
    line++;
    if (!strcmp(s, "ascii")) {
      kanji=FALSE;
      fullkanji=FALSE;
      continue;
    }
    if (!strcmp(s, "kanji")) {
      kanji=TRUE;
      fullkanji=FALSE;
      continue;
    }
    if (!strcmp(s, "fullkanji")) {
      fullkanji=TRUE;
      kanji = FALSE;
      continue;
    }


    if (!strcmp(s, "tsa")) {
      printf("Jumping Generals according to TSA leaflet.\n");
      fflush(stdout);
      jgs_tsa = TRUE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "ew")) {
      printf("Jumping Generals according to E.Werner.\n");
      fflush(stdout);
      jgs_tsa = FALSE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "classic")) {
      printf("Lion Hawk according to TSA leaflet.\n");
      fflush(stdout);
      modern_lion_hawk = FALSE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "modern")) {
      printf("Lion Hawk according to Colin Paul Adams.\n");
      fflush(stdout);
      modern_lion_hawk = TRUE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "burn")) {
      printf("Fire Demon also burns friendly pieces (Eduard Werner).\n");
      hodges_FiD = TRUE;
      book = book_init();
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "noburn")) {
      printf("Fire Demon doesn't burn friendly pieces (Eduard Werner).\n");
      hodges_FiD = TRUE;
		  book = book_init();
		  init();
		  gen();
		  continue;
    }
    if (!strcmp(s, "tame")) {
      printf("Fire Demon only burns if it doesn't capture.\n");
      tame_FiD = TRUE;
      book = book_init();
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "verytame")) {
      printf("Fire Demon only burns, but cannot capture.\n");
      verytame_FiD = TRUE;
      book = book_init();
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "notame")) {
      printf("Fire Demon burns also on capture.\n");
      tame_FiD = FALSE;
      verytame_FiD = FALSE;
      book = book_init();
      init();
      gen();
      continue;
    }
    promote = FALSE;
    if ( 10 == sscanf(s,"%d.%s%d%c%c%d%c%c%d%c",&move_nr, piece, &from_file, &from_rank, &igui,
		     &over_file, &over_rank, &igui2, &to_file, &to_rank) ) {
      if ( 11 == sscanf(s,"%d.%s%d%c%c%d%c%c%d%c%c", &move_nr, piece, &from_file, &from_rank, &igui,
		     &over_file, &over_rank, &igui2, &to_file, &to_rank, &prom) ) {
		  if ( prom == '+' ) promote = TRUE;
      }
      /* add the move to the history */
      sprintf(hist_s,"%d%c%c%d%c%c%d%c",from_file, from_rank,igui,
	      over_file,over_rank,igui2,to_file,to_rank);
      add_history(hist_s);
	
      from = 16 - from_file;
      from += 16 * (from_rank - 'a');
      over = 16 - over_file;
      over += 16 * (over_rank - 'a');		  
      to = 16 - to_file;
      to += 16 * (to_rank - 'a');

		  /* loop through the moves to see if it's legal */
      found = FALSE;
      for (i = 0; i < gen_end[ply]; ++i) {
	if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
	  continue;
	if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
	  continue;
	if (gen_dat[i].m.b.bits & 32)
	  continue;
	if ( ! (gen_dat[i].m.b.bits & 4))
	  continue;
	if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to  && gen_dat[i].m.b.over == over) {
	  found = TRUE;
	  break;
	}
      } 
    } else if ( 7 == sscanf(s,"%d.%s%d%c%c%d%c",&move_nr, piece, &from_file, &from_rank, &igui,
			    &to_file, &to_rank) ) {
      if ( 8 == sscanf(s,"%d.%s%d%c%c%d%c%c",&move_nr, piece, &from_file, &from_rank, &igui,
		       &to_file, &to_rank, &prom) ) {
	if ( prom == '+' ) promote = TRUE;
      }
      sprintf(hist_s,"%d%c%c%d%c",from_file, from_rank,
	      igui,to_file,to_rank);
      add_history(hist_s);

      from = 16 - from_file;
      from += 16 * (from_rank - 'a');
      to = 16 - to_file;
      to += 16 * (to_rank - 'a');
#ifdef DEBUG
      fprintf(stderr, "test\n");
#endif
      found = FALSE;
      for (i = 0; i < gen_end[ply]; ++i) {
	/* if ( gen_dat[i].m.b.bits & 4) continue; */
	if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
	  continue;
	if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
	  continue;
	if ( igui != '!' && (gen_dat[i].m.b.bits & 32))
	  continue;
	if ( igui == '!' && ! (gen_dat[i].m.b.bits & 32))
	  continue;
	if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) {
	  found = TRUE;
	  break;
	}
      }
    } else if ( sscanf(s, "setup %s\n", setup_file )) {
      load_position( setup_file );
      continue;
    } 
	
    if (!found || !makemove(gen_dat[i].m.b)) {
      printf("Illegal move on line %d.\n", line);
      print_board(stdout);
      fflush(stdout);
      return;
    } else {
      /* save the "backup" data because it will be overwritten */
      undo_dat[undos] = hist_dat[0];
      ++undos;
      ply = 0;
      gen();
    }

  } while (!feof(fh));
  printf("%d plies\n", undos );

  fclose(fh);
  print_board(stdout);
  fflush(stdout);
}

void load_old_game( void ) {
  unsigned char fullfn[1024], fn[1024], s[15];
  int from, over, to, i, move_nr;
  int from_file, to_file;
  int over_file;
  int line = 0;
  char from_rank, over_rank, to_rank, igui, igui2, prom;
  BOOL promote, found;
  FILE *fh;
  printf("Enter load file name: ");
  fflush(stdout);
  scanf("%s", fn);
  sprintf(fullfn,"%s/%s",savegamepath, fn);
  fh = fopen(fullfn,"rb");
  if (! fh) {
    printf("No such file!\n");
    fflush(stdout);
    return;
  }
  do {    
    fscanf(fh,"%s\n",s);
    line++;
    if (!strcmp(s, "tsa")) {
      printf("Jumping Generals according to TSA leaflet.\n");
      fflush(stdout);
      jgs_tsa = TRUE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "ew")) {
      printf("Jumping Generals according to E.Werner.\n");
      fflush(stdout);
      jgs_tsa = FALSE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "classic")) {
      printf("Lion Hawk according to TSA leaflet.\n");
      fflush(stdout);
      modern_lion_hawk = FALSE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "modern")) {
      printf("Lion Hawk according to Colin Paul Adams.\n");
      fflush(stdout);
      modern_lion_hawk = TRUE;
      init();
      gen();
      continue;
    }
    if (!strcmp(s, "undo")) {
      undo_move();
      continue;
    }

    promote = FALSE;
    if ( 9 == sscanf(s,"%d.%d%c%c%d%c%c%d%c",&move_nr, &from_file, &from_rank, &igui,
		     &over_file, &over_rank, &igui2, &to_file, &to_rank) ) {
      if ( 10 == sscanf(s,"%d.%d%c%c%d%c%c%d%c%c", &move_nr, &from_file, &from_rank, &igui,
		     &over_file, &over_rank, &igui2, &to_file, &to_rank, &prom) ) {
		  if ( prom == '+' ) promote = TRUE;
      }
		
      from = 16 - from_file;
      from += 16 * (from_rank - 'a');
      over = 16 - over_file;
      over += 16 * (over_rank - 'a');		  
      to = 16 - to_file;
      to += 16 * (to_rank - 'a');

		  /* loop through the moves to see if it's legal */
      found = FALSE;
      for (i = 0; i < gen_end[ply]; ++i) {
	if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
	  continue;
	if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
	  continue;
	if (gen_dat[i].m.b.bits & 32)
	  continue;
	if ( ! (gen_dat[i].m.b.bits & 4))
	  continue;
	if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to  && gen_dat[i].m.b.over == over) {
	  found = TRUE;
	  break;
	}
      } 
    } else if ( 6 == sscanf(s,"%d.%d%c%c%d%c",&move_nr, &from_file, &from_rank, &igui,
			    &to_file, &to_rank) ) {
      if ( 7 == sscanf(s,"%d.%d%c%c%d%c%c",&move_nr, &from_file, &from_rank, &igui,
		       &to_file, &to_rank, &prom) ) {
	if ( prom == '+' ) promote = TRUE;
      }
      from = 16 - from_file;
      from += 16 * (from_rank - 'a');
      to = 16 - to_file;
      to += 16 * (to_rank - 'a');
#ifdef DEBUG
      fprintf(stderr, "test\n");
#endif
      found = FALSE;
      for (i = 0; i < gen_end[ply]; ++i) {
	/* if ( gen_dat[i].m.b.bits & 4) continue; */
	if ( promote && ! ( gen_dat[i].m.b.bits & 16 )) 
	  continue;
	if  ( !promote && (gen_dat[i].m.b.bits & 16)) 
	  continue;
	if ( igui != '!' && (gen_dat[i].m.b.bits & 32))
	  continue;
	if ( igui == '!' && ! (gen_dat[i].m.b.bits & 32))
	  continue;
	if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) {
	  found = TRUE;
	  break;
	}
      }
    }
	
    if (!found || !makemove(gen_dat[i].m.b)) {
      printf("Illegal move on line %d.\n", line);
      print_board(stdout);
      fflush(stdout);
      return;
    } else {
      /* save the "backup" data because it will be overwritten */
      undo_dat[undos] = hist_dat[0];
      ++undos;
      ply = 0;
      gen();
    }

  } while (!feof(fh));
  printf("%d plies\n", undos );

  fclose(fh);
  print_board(stdout);
  fflush(stdout);
}


void save_game( char *filename ) {
  unsigned char fn[1024], fullfn[1024];
  int i;
  FILE *fh;
  if (strlen(filename) < 1) {
    printf("Enter save file name: ");
    fflush(stdout);
    scanf("%s",fn);
    sprintf(fullfn,"%s/%s",savegamepath,fn);
    fh = fopen(fullfn,"rb");
  } else {
    strcpy(fn, filename);
    fh = fopen(fn,"rb");
  }

  if (fh) {
    printf("File already exists!\n");
    fclose(fh);
    fflush(stdout);
    return;
  }
  fh = fopen(fullfn,"wb");
  printf("%d\n",undos);
  if ( loaded_position ) {
    fprintf(fh,"setup %s\n", positionfile );
  }
  for (i=0;i < undos; i++) {
    fprintf(fh,"%d. %s %s\n",i+1,piece_string[undo_dat[i].oldpiece], half_move_str(undo_dat[i].m.b));
  }
  fclose(fh);
}

int identifymove( int file, char rank, char promotion_or_capture ) {
  /* short notation like 
     3d move or capture on 3d
     7l+ move piece on or to 7l with promotion
     3dx capture something with piece on 3d
     would be nice to have sth like HFxRGn as well
  */

  int i, from_or_to, other_file;
  char other_rank, promotion_char, capture_char;
  BOOL promotion = FALSE, capture = FALSE;


  if (((file < 1)&&(file > FILES))||
      ((rank < 'a')&&(rank > 'p')))
    {
      printf("Not a square!\n");
      fflush(stdout);
      return(-1);
    } /**/

  /* if promotion or capture stuff */ 
  if (promotion_or_capture == '+') {
    promotion = TRUE;
  } else if (promotion_or_capture == 'x') {
    capture = TRUE;
  }
  /*    */
  from_or_to = RANKS - file;
  from_or_to += FILES * (rank - 'a');

  for (i = 0; i < gen_end[ply]; ++i) {
    if ( gen_dat[i].m.b.from == from_or_to ) { /* friendly piece found, should have only one move */
      /* check if capture or prom is ok */
      if ( promotion && ! ( gen_dat[i].m.b.bits & 16 )) 
	continue;
      if  ( !promotion && ( gen_dat[i].m.b.bits & 16 )) 
	continue;
      if ( capture && (piece[gen_dat[i].m.b.to] == EMPTY ) )
	continue;
      return(i);
    } else if ( gen_dat[i].m.b.to == from_or_to ) { /* opponent piece or empty square found */
      /* check if capture or prom is ok */
      if ( promotion && ! ( gen_dat[i].m.b.bits & 16 )) 
	continue;
      else if  ( !promotion && ( gen_dat[i].m.b.bits & 16 )) 
	continue;
      else
	return(i);
    }
  }
  return( -1 );
}

void goto_position( void ) {
  /* asks for move number and displays 
     position after that move */
  
  int move_nr;

  printf("Which move number? ");
  scanf("%d", &move_nr );
  
  if (( move_nr < 0 ) || ( move_nr > undos + redos )) {
    printf("No such move number.\n");
    return;
  }
  if (move_nr == undos ) return;
  if ( move_nr < undos ) { /* we have to go back */
    while ( move_nr < undos )
      undo_move();
  } else { /* we have to go forward */
    while ( move_nr > undos )
      redo_move();
  }

}

void show_moves( void ) {
  /* shows moves of piece on square */
  /* should return number of possible
     moves
  */
  char rank;
  int i, j, k, n, from, file;
  BOOL same_rank = FALSE;
  BOOL faked_piece = FALSE;
  BOOL i_am_a_fire_demon = FALSE;
  BOOL swapped_sides = FALSE;
  int save_piece[NUMSQUARES];
  int save_color[NUMSQUARES];
  move_possibilities var_board[NUMSQUARES];

  int orig_side = side, orig_xside = xside;

  printf("Which piece (enter square)? ");
  scanf("%d%c",&file,&rank);
  if (((file < 1)&&(file > FILES))||
      ((rank < 'a')&&(rank > 'p')))
    {
      printf("Not a square!\n");
      fflush(stdout);
      return;
    }

  /* index of the current square */
  from = RANKS - file;
  from += FILES * (rank - 'a');

  if (( piece[from] == FIRE_DEMON ) || ! hodges_FiD )
    i_am_a_fire_demon = TRUE; /* only Hodges-style FiD will be 
				 burned by another FiD */

  /* check if piece belong to current player */
  if ( color[from] == xside ) { /* no it doesn't, we have to swap */
    xside ^= 1;
    side ^= 1;
    swapped_sides = TRUE;
  }


  /* init var_board */
  for ( i = 0; i < NUMSQUARES; i++) {
    var_board[i] = NO_MOVE;
    if ( i == from ){
	var_board[i]= ALREADY_THERE;
    }
  }

  gen();
  /* normal moves first */
  for (i = 0; i < gen_end[ply]; ++i) {
    if ( gen_dat[i].m.b.from == from ) {
      /* printf("%s, ", move_str(gen_dat[i].m.b)); */
      if ( suicide(gen_dat[i].m.b.to) ) {
	var_board[gen_dat[i].m.b.to] = SUICIDE; /* suicide move */
      } else {
	var_board[gen_dat[i].m.b.to] = NORMAL; /* move possible */
      }
    }
  }

  /* for hypothetical moves we need an empty board for the gen_stuff */
  /* save current board and clear it */
  if ( show_hypothetical_moves ) {
    for ( i = 0; i < NUMSQUARES; i++) {
      save_piece[i] = piece[i];
      save_color[i] = color[i];
      if (( i != from ) && ( piece[i] != KING )) { /*don't clear this one */
	piece[i] = EMPTY;
	color[i] = EMPTY;
      } 
    }
    gen();

  /* hypothetical moves now (on empty board) */
    for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.from == from ) {
	/* printf("%s, ", move_str(gen_dat[i].m.b)); */
	if ( var_board[gen_dat[i].m.b.to] == NO_MOVE )
	  var_board[gen_dat[i].m.b.to] = HYPOTHETICAL; /* hypothetical move */
      }
    }
  
  /* restore the board */
    for ( i = 0; i < NUMSQUARES; i++) {
      piece[i] = save_piece[i];
      color[i] = save_color[i];
    }
    gen();
  }
/* change NORMAL to DANGER if opponent can move on var_board */

  for ( i = 0; i < NUMSQUARES; i++)
    if ( var_board[i] == NORMAL ) {
      if ( piece[i] == EMPTY ) {
	piece[i] = PAWN; /* fake a pawn and see whether opponent can capture */
	color[i] = side;
	faked_piece = TRUE;
      } else { /* since it's a move, only opponent piece can be here */
	color[i] = side;
      }

      side ^= 1;
      xside ^= 1;

      gen();

      for (j = 0; j < gen_end[ply]; ++j) {
	if ( gen_dat[j].m.b.to == i ) { 
	  var_board[i] = DANGER;
	}
	/* check whether Fire Demon can burn */
	if ( !i_am_a_fire_demon ) { /* otherwise  I won't get hurt, 
				       hodges_FiD already checked */
	  if ( piece[gen_dat[j].m.b.from] == FIRE_DEMON ) {
	    for (k = 0; k < 8; k++) {
	      n = mailbox[mailbox256[(int)gen_dat[j].m.b.to] + lion_single_steps[k]];
#ifdef DDEBUG
	      printf("burning %d%c\n",16-GetFile(n),GetRank(n) + 'a');
#endif
	      if ( n == i ) {
		var_board[i] = DANGER;
	      }
	    }
	  }
	}
      }
      if (faked_piece ) { /* remove the faked piece */
	piece[i] = EMPTY;
	color[i] = EMPTY;
	faked_piece = FALSE;
      } else {
	color[i] = side; /* not xside as sides have been swapped :-) 
			    now this looks bug-prone*/
      }

      side ^= 1;
      xside ^= 1;
    }

  if ( swapped_sides ) {
    side ^= 1;
    xside ^= 1;
    gen();
  }
  side = orig_side;
  xside = orig_xside;
  gen();


  printf("\n All moves.\n");
  /* now print the variant board */
  if ( fullkanji ) {
        printf(  "\e[0m\n\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n");
    printf(  "  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf( "║%s   \e[0m", color_string[var_board[i]]);
	  else
	    printf( "│%s   \e[0m", color_string[var_board[i]]);
	  break;
	case LIGHT:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║%s\e[1m %s\e[0m", color_string[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "║%s\e[1m %s\e[0m", color_string[var_board[i]], lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│%s\e[1m %s\e[0m", color_string[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "│%s\e[1m %s\e[0m", color_string[var_board[i]], lower_kanji_string[piece[i]]);
	  break;
	case DARK:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║%s %s\e[0m", color_string[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "║%s %s\e[0m", color_string[var_board[i]], lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│%s %s\e[0m", color_string[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "│%s %s\e[0m", color_string[var_board[i]], lower_kanji_string[piece[i]]);
	  break;
	}
	if ((i + 1) % RANKS == 0 && i != LASTSQUARE ) {
	  if ( same_rank ) {
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	    same_rank = FALSE;
	  } else {
	    printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;
	  }
	}
	if (( i == LASTSQUARE )&& !same_rank) {
	  printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;	
	}
      } else { /* if var_board[i] */
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf( "║   ");
	  else
	    printf( "│   ");
	  break;
	case LIGHT:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║ \e[1m%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "║ \e[1m%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│ \e[1m%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "│ \e[1m%s\e[0m", lower_kanji_string[piece[i]]);
	  break;
	case DARK:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║ %s", upper_kanji_string[piece[i]]);
	    else
	      printf( "║ %s", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│ %s", upper_kanji_string[piece[i]]);
	    else
	      printf( "│ %s", lower_kanji_string[piece[i]]);
	  break;
	}
	if ((i + 1) % RANKS == 0 && i != LASTSQUARE ) {
	  if ( same_rank ) {
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	    same_rank = FALSE;
	  } else {
	    printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;
	  }
	}
	if (( i == LASTSQUARE )&& !same_rank) {
	  printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;	
	}
      }
    }
    printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  } else if ( kanji ) {
    printf("\n\n   16  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1\n");
    printf(  "  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf( "║%s   \e[0m", color_string[var_board[i]]);
	    else
	    printf( "│%s   \e[0m", color_string[var_board[i]]);
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║%s\e[1m%s\e[0m", 
		   color_string[var_board[i]], kanji_piece_string[piece[i]]);
	  else
	    printf("|%s\e[1m%s\e[0m", 
		   color_string[var_board[i]], kanji_piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s%s\e[0m", 
		   color_string[var_board[i]], kanji_piece_string[piece[i]]);
	  else
	    printf("|%s%s\e[0m", color_string[var_board[i]], 
		   kanji_piece_string[piece[i]]);
	  break;	
	}
      } else {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf("║   ");
	  else
	    printf("|   ");
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    printf("|\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s", kanji_piece_string[piece[i]]);
	  else
	    printf("|%s", kanji_piece_string[piece[i]]);
	  break;
	}
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
    printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  } else { /* ascii board */
    printf("\n\n   16  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1\n");
    printf("  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  printf("|%s   \e[0m", color_string[var_board[i]]); 
	  break;
	case LIGHT:
	  printf("|%s\e[1m%s\e[0m", color_string[var_board[i]], piece_string[piece[i]]);
	  break;
	case DARK:
	  printf("|%s%s\e[0m", color_string[var_board[i]], piece_string[piece[i]]);
	  break;	
	}
      } else {
	switch (color[i]) {
	case EMPTY:
	  printf("|   ");
	  break;
	case LIGHT:
	  printf("|\e[1m%s\e[0m", piece_string[piece[i]]);
	  break;
	case DARK:
	  printf("|%s", piece_string[piece[i]]);
	  break;
	}
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	printf("| %c\n  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
    printf("| p\n  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n   16  15  14  13  12  11  10   9    8    7    6    5    4    3    2    1\n\n");
  }   
  fflush(stdout);
}


void show_checks( void ) {
  /* find the moves which would put the opponent into check */
  /* not very useful as such, but a good step into the direction
     of a mate problem solver */
  int i;
  gen_checks();
  for (i = 0; i <= ply; i++ ) {
   fprintf( stderr,"# %s ", move_str( hist_dat[i].m.b ));
  }
}

void show_influence( void ) {
  /* shows pieces that can move on square */
  /* must fake a piece for an empty square
     to correctly reflect JGs, furthermore
     must remove the defenders and regenerate
     to correctly reflect stacked pieces
     until theres no more influences found.
     Same thing must be done for other side.
  */
  char rank;
  int i, k, n, to, file, level;
  BOOL found_influence = FALSE;
  BOOL same_rank = FALSE;
  int var_board[NUMSQUARES];
  int save_piece[NUMSQUARES];
  int save_color[NUMSQUARES];
  /* to work around lurking bugs */
  int orig_side = side, orig_xside = xside;

  printf("Which square? ");
  scanf("%d%c",&file,&rank);
  if (((file < 1)&&(file > 16))||
      ((rank < 'a')&&(rank > 'p')))
    {
      printf("Not a square!\n");
      fflush(stdout);
      return;
    }

  /* init var_board */
  for ( i = 0; i < NUMSQUARES; i++)
    var_board[i] = 0;

  /* save current board */
  for ( i = 0; i < NUMSQUARES; i++) {
    save_piece[i] = piece[i];
    save_color[i] = color[i];
  }

  to = 16 - file;
  to += 16 * (rank - 'a');

  level = 1;

  /* look at own  pieces that can walk there */

  /* blank out piece */
  if ( piece[to] != EMPTY ) {
    piece[to] = EMPTY;
    color[to] = EMPTY;
    gen();
  }

  
  for (i = 0; i < gen_end[ply]; ++i) { /* level is 1 here */
      if ( gen_dat[i].m.b.to == to ) {
	printf("%s, ", move_str(gen_dat[i].m.b));
	var_board[gen_dat[i].m.b.from] = level;
      } else {	/* can a fire demon burn something on the square */
	if ( piece[ gen_dat[i].m.b.from ] == FIRE_DEMON ) {
	  /* only fire demons burn */
	  /* is the square next to "to"? */
	  for (k = 0; k < 8; k++) {
	    n = mailbox[mailbox256[(int)gen_dat[i].m.b.to] + lion_single_steps[k]];
#ifdef DDEBUG
	    printf("burning %d%c\n",16-GetFile(n),GetRank(n) + 'a');
#endif
	    if ( n == to ) { /* FiD can burn on "to" if not ... */
	      /* is there an enemy FiD around "to" which might prevent burning? */
	      if ( suicide( gen_dat[i].m.b.to )) {
		if ( hodges_FiD ) { /* still burns */
		  if ( ! var_board[ gen_dat[i].m.b.from ] ) {
		    var_board[ gen_dat[i].m.b.from ] = 3;
		    printf("%s, ", move_str(gen_dat[i].m.b));
		  /* enemy FiD not important */
		  }
		}
	      } else {
		if ( ! var_board[ gen_dat[i].m.b.from ] ) 
		  var_board[ gen_dat[i].m.b.from ] = 3;
		printf("%s, ", move_str(gen_dat[i].m.b));
	      }
	    }
	  }
	}
      }
  }
  printf("\n");
  
  /* look at enemy pieces that can walk there */

  side  ^= 1;
  xside ^= 1;

  gen();
  
  for (i = 0; i < gen_end[ply]; ++i) { /* level is 1 here */
      if ( gen_dat[i].m.b.to == to ) {
	printf("%s, ", move_str(gen_dat[i].m.b));
	var_board[gen_dat[i].m.b.from] = level;
      } else {	/* can a fire demon burn something on the square */
	if ( piece[ gen_dat[i].m.b.from ] == FIRE_DEMON ) {
	  /* only fire demons burn */
	  /* is the square next to "to"? */
	  for (k = 0; k < 8; k++) {
	    n = mailbox[mailbox256[(int)gen_dat[i].m.b.to] + lion_single_steps[k]];
#ifdef DDEBUG
	    printf("burning %d%c\n",16-GetFile(n),GetRank(n) + 'a');
#endif
	    if ( n == to ) { /* FiD can burn on "to" if not ... */
	      /* is there an enemy FiD around "to" which might prevent burning? */
	      if ( suicide( gen_dat[i].m.b.to )) {
		if ( hodges_FiD ) { /* still burns */
		  if ( ! var_board[ gen_dat[i].m.b.from ] ) {
		    var_board[ gen_dat[i].m.b.from ] = 3;
		    printf("%s, ", move_str(gen_dat[i].m.b));
		  /* enemy FiD not important */
		  }
		}
	      } else {
		if ( ! var_board[ gen_dat[i].m.b.from ] ) 
		  var_board[ gen_dat[i].m.b.from ] = 3;
		printf("%s, ", move_str(gen_dat[i].m.b));
	      }
	    }
	  }
	}
      }
  }
  printf("\n");


  side ^=  1;
  xside ^= 1;


  if ( piece[to] == EMPTY ) { 
    /* must fake a piece here for JGs */
    piece[to] = PAWN;
    color[to] = xside;
  } 

  level = 2;

  /* parse and remove the pieces so far until
     there's no more influences.
  */

  do {
    gen(); /* regenerate */
    found_influence = FALSE;
    for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	/* printf("%s, ", half_move_str(gen_dat[i].m.b)); */
	piece[gen_dat[i].m.b.from] = EMPTY;
	color[gen_dat[i].m.b.from] = EMPTY;
	found_influence = TRUE;
	if ( var_board[gen_dat[i].m.b.from] == 0 ) {
	  var_board[gen_dat[i].m.b.from] = level;
	}
      }
    }


    side ^=  1;
    xside ^= 1;
    color[to] = xside;

    gen(); /* regenerate */

    for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	/* printf("%s, ", half_move_str(gen_dat[i].m.b)); */
	piece[gen_dat[i].m.b.from] = EMPTY;
	color[gen_dat[i].m.b.from] = EMPTY;
	found_influence = TRUE;
	if ( var_board[gen_dat[i].m.b.from] == 0 ) {
	  var_board[gen_dat[i].m.b.from] = level;
	}
      }
    }

    side ^=  1;
    xside ^= 1;
    color[to] = xside;

    level++;
    printf("\n");
  } while ( found_influence );

  printf("\n All moves.\n");



  /* restore current board */

  for ( i = 0; i < NUMSQUARES; i++) {
     piece[i] = save_piece[i];
     color[i] = save_color[i];
     side = orig_side;
     xside = orig_xside;
  }
  gen();

  /* now print the variant board */
  if ( fullkanji ) {
    printf(  "\e[0m\n\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n");
    printf(  "  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf( "║%s   \e[0m",inf_color[var_board[i]]);
	  else
	    printf( "│%s   \e[0m",inf_color[var_board[i]]);
	  break;
	case LIGHT:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║%s\e[1m %s\e[0m",inf_color[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "║%s\e[1m %s\e[0m",inf_color[var_board[i]], lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│%s\e[1m %s\e[0m",inf_color[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "│%s\e[1m %s\e[0m",inf_color[var_board[i]], lower_kanji_string[piece[i]]);
	  break;
	case DARK:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║%s %s\e[0m",inf_color[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "║%s %s\e[0m",inf_color[var_board[i]], lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│%s %s\e[0m",inf_color[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "│%s %s\e[0m",inf_color[var_board[i]], lower_kanji_string[piece[i]]);
	  break;
	}
	if ((i + 1) % RANKS == 0 && i != LASTSQUARE ) {
	  if ( same_rank ) {
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	    same_rank = FALSE;
	  } else {
	    printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;
	  }
	}
	if (( i == LASTSQUARE )&& !same_rank) {
	  printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;	
	}
      } else {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf( "║   ",inf_color[var_board[i]]);
	  else
	    printf( "│   ",inf_color[var_board[i]]);
	  break;
	case LIGHT:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║ \e[1m%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "║ \e[1m%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│ \e[1m%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "│ \e[1m%s\e[0m", lower_kanji_string[piece[i]]);
	  break;
	case DARK:
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      printf( "║ %s", upper_kanji_string[piece[i]]);
	    else
	      printf( "║ %s", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│ %s", upper_kanji_string[piece[i]]);
	    else
	      printf( "│ %s", lower_kanji_string[piece[i]]);
	  break;
	}
	if ((i + 1) % RANKS == 0 && i != LASTSQUARE ) {
	  if ( same_rank ) {
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	    same_rank = FALSE;
	  } else {
	    printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;
	  }
	}
	if (( i == LASTSQUARE )&& !same_rank) {
	  printf(  "║\n  ");
	  i-=16;
	  same_rank = TRUE;	
	}
      }
    }
    printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  } else if ( kanji ) {
    printf("\n\n   16  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1\n");
    printf(  "  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf( "║%s   \e[0m",inf_color[var_board[i]]);
	  else
	    printf( "│%s   \e[0m",inf_color[var_board[i]]);
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║%s\e[1m%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  else
	    printf("|%s\e[1m%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  else
	    printf("|%s%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  break;	
	}
      } else {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf("║   ");
	  else
	    printf("|   ");
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    printf("|\e[1m%s\e[0m", kanji_piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s", kanji_piece_string[piece[i]]);
	  else
	    printf("|%s", kanji_piece_string[piece[i]]);
	  break;
	}
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
    printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  } else { /* ascii board */
    printf("\e[0m\n\n   16  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1\n");
    printf("  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  printf("|\e[42m   \e[0m"); 
	  break;
	case LIGHT:
	  printf("|\e[45m\e[1m%s\e[0m", piece_string[piece[i]]);
	  break;
	case DARK:
	  printf("|\e[45m%s\e[0m", piece_string[piece[i]]);
	  break;	
	}
      }
      else {
	switch (color[i]) {
	case EMPTY:
	  printf("|   ");
	  break;
	case LIGHT:
	  printf("|\e[1m%s\e[0m", piece_string[piece[i]]);
	  break;
	case DARK:
	  printf("|%s", piece_string[piece[i]]);
	  break;
	}
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	printf("| %c\n  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
    printf("| p\n  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  }
  fflush(stdout);

  gen();
}

void export_TeX( void ) {
  unsigned char outputname[1024], path[1024];
  FILE *TeX;
  int TeX_line = 1450;
  int i;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */

  fprintf(stdout, "Enter name for TeX file: ");
  scanf("%s",outputname);
  sprintf(path, "%s/%s",TeXoutputpath,outputname);
  if ( !(TeX=fopen(path,"wb"))) {
    fprintf(stderr, "Cannot open %s/%s\n", TeXoutputpath,outputname);
    return;
  }

  /* get the last move, should be in undo_dat[undos] */
  if ( undos ) { /* not used yet */
    from_square = undo_dat[undos-1].m.b.from;
    to_square = undo_dat[undos-1].m.b.to;
  } else {
    from_square = -1;
    to_square = -1;
  }
  if ( redos ) {
    next_from = redo_dat[redos-1].m.b.from;
    next_to = redo_dat[redos-1].m.b.to;
  } else {
    next_from = -1;
    next_to = -1;
  }

  /* TeX preamble */
  fprintf( TeX ,"\\documentclass[12pt]{article}\n\\usepackage{colordvi}\n\\usepackage{shogi}\n\\pagestyle{empty}\n\\begin{document}\n\\tenjikudiag{%%\n\\multiputlist(40,1550)(85,0){");
  /* filling in the data */
    for (i = 0; i < NUMSQUARES; ++i) {
      switch (color[i]) {
      case EMPTY:
	fprintf( TeX,","); 
	break;
      case LIGHT:
	fprintf( TeX,"\\%s,", TeX_light_piece_string[piece[i]]);
	break;
      case DARK:
	fprintf( TeX,"\\%s,", TeX_dark_piece_string[piece[i]]);
	break;	
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE) {
	fprintf( TeX,"}\n\\multiputlist(40,%d)(85,0){",TeX_line);
	TeX_line -= 100;
      }
    }

  /* clearing up */
  fprintf( TeX ,"}}\n\\end{document}");
  fclose(TeX);
}

void undo_move( void ) {
  if (!undos)
    return;
  --undos;
  hist_dat[0] = undo_dat[undos];
  redo_dat[redos++] = hist_dat[0];
  ply = 1;
  takeback();
  gen();
}

void redo_move( void ) {
  if (!redos) return;
  --redos;
  /* has to be put back onto undo_dat as well
     as taken down from redo_dat */
  hist_dat[0] = redo_dat[redos];
  undo_dat[undos++] = hist_dat[0];
  ply = 1;
  makemove(hist_dat[0].m.b);
}

void process_arguments(int argc, char **argv) {
  char path[1024];
  /* fprintf(stdout,"%d arguments", argc); */
  if (argc == 2 ) {
    /* fprintf(stderr,"%s\n",argv[0]);
    fprintf(stderr,"%s\n",argv[1]);
    fprintf(stderr,"%s\n",argv[2]); */
    sprintf(path,"%s/%s",savegamepath,argv[1]);
    really_load_game(path);
  }
  return;
}

void read_init_file( void ) {
  really_load_game("tenjiku.init");
}


void setup_board( void ) {
  int i, sq, piece_nr, file;
  char which_side[1], rank;

  printf("Which square? ");
  scanf("%d%c",&file,&rank);
  if (((file < 1)&&(file > FILES))||
      ((rank < 'a')&&(rank > 'p')))
    {
      printf("Not a square!\n");
      fflush(stdout);
      return;
    }
  sq = 16 - file;
  sq += 16 * (rank - 'a');


  for (i=0; i< PIECE_TYPES; i++) {
    if (fullkanji)
      fprintf(stdout, "%d: %s%s, ", i, 
	      upper_kanji_string[i],lower_kanji_string[i]); 
    else if (kanji)
      fprintf(stdout, "%d: %s, ", i, kanji_piece_string[i]); 
    else
      fprintf(stdout, "%d: %s, ", i, piece_string[i]); 
  }

  fprintf( stdout, " higher no to clear square. Which piece? (no[bw]) ");
  scanf("%d%s", &piece_nr, which_side);
  if (( piece_nr >= 0 ) && (piece_nr <= PIECE_TYPES)) {
    piece[sq] = piece_nr;
    if ( !strcmp(which_side,"b")) {
      color[sq] = LIGHT;
    } else {
      color[sq] = DARK;
    }
  } else if (piece_nr > PIECE_TYPES ){
    color[sq] = EMPTY;
    piece[sq] = EMPTY;
  } else{
    fprintf(stderr,"Invalid input: side: %s, piece %d", which_side, piece_nr);
  }
  gen();
}


void load_position( char *defaultinputname ) {
  int i;
  FILE *frompiece, *fromcolor;
  char inputname[1024];
  char piecefile[1024], colorfile[1024];

  if ( ! defaultinputname ) {
    fprintf(stdout, "Enter name for position file (w.o. extension): ");
    scanf("%s",inputname);
  } else {
    strcpy(inputname, defaultinputname);
  }

  sprintf(piecefile, "%s/%s.pieces",positionpath,inputname); 
  sprintf(colorfile, "%s/%s.colors",positionpath,inputname); 
  if ( !(frompiece=fopen(piecefile,"rb"))) {
    fprintf(stderr, "Cannot open %s\n", piecefile);
    return;
  }
  if ( !(fromcolor=fopen(colorfile,"rb"))) {
    fprintf(stderr, "Cannot open %s\n", colorfile);
    return;
  }
  for (i = 0; i < NUMSQUARES; i++) {
    fscanf( frompiece, "%d\n", &piece[i]);
    fscanf( fromcolor, "%d\n", &color[i]);
  }
  fclose( frompiece );
  fclose( fromcolor );
  loaded_position = TRUE;
  strcpy(positionfile, inputname);
}


void save_position( void ) {
  int i;
  FILE *outpiece, *outcolor;
  char outputname[1024];
  char piecefile[1024], colorfile[1024];

  fprintf(stdout, "Enter name for position file: ");
  scanf("%s",outputname);
  sprintf(piecefile, "%s/%s.pieces",positionpath,outputname);
  sprintf(colorfile, "%s/%s.colors",positionpath,outputname);
  if ( !(outpiece=fopen(piecefile,"wb"))) {
    fprintf(stderr, "Cannot open %s\n", piecefile);
    return;
  }
  if ( !(outcolor=fopen(colorfile,"wb"))) {
    fprintf(stderr, "Cannot open %s\n", colorfile);
    return;
  }

  for (i = 0; i < NUMSQUARES; i++) {
    fprintf(outpiece,"%d\n",piece[i]);
    fprintf(outcolor,"%d\n",color[i]);
  }
  fclose( outpiece );
  fclose( outcolor );
}

