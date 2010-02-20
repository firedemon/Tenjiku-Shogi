
/*
 *	MAIN.C
 *      E.Werner 2000, 2010
 *      derived from
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */

unsigned char tenjiku_version[] = { "0.70" };
/* #define DEBUG
 */ 
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "defs.h"
#include "data.h"
#include "protos.h"

#ifdef NETWORKING

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

MYSQL *conn;

char local_player[128];
char network_player[128];
char game_id[20] = "";

#endif

unsigned char *side_name[2] = { "Sente", "Gote" };
int computer_side, network_side;
char TeXoutputpath[1024] = "exports";
char savegamepath[1024] = "savegames";
char positionpath[1024] = "positions";
char positionfile[1024];
char global_last_move[64];

FILE *read_pipe = NULL;
FILE *write_pipe = NULL;

#ifdef NETWORKING

BOOL looking_for_game = FALSE;
BOOL accepting_game = FALSE;

#endif

BOOL rotated = FALSE; /* board is not rotated */
BOOL annotate = TRUE;
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

BOOL full_captures = FALSE;

BOOL loaded_position = FALSE; /*necessary to provide information to save_game - not yet implemented */

BOOL networked_game = FALSE;

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
	int from, over, to, i, idx;
	int from_file, to_file;
	int over_file;
	char net_move[5]; /* should be sufficient */
       
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
	printf("Compiled-in options:\n");
#ifdef japanese_FiD
	printf("FiD slides horizontally, not vertically\n");
#else
	printf("FiD slides vertically, not horizontally\n");
#endif
#ifdef edo_style_Feg
	printf("FEg jumps to second square orthogonally\n");
#else
	printf("FEd has igui and double capture along slide directions\n");
#endif
#ifdef japanese_HT
	printf("HT jumps into all directions, triple-steps orthogonally and slides vertically\n");
#else
	printf("HT jumps and triple-steps orthogonally\n");
#endif


	init();
	book = book_init();
	gen();
	computer_side = EMPTY;
	network_side = EMPTY;
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
		} else  {
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
		  printf("Computer's move: %s\n", move_str(gen_dat[i].m.b));
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
	      printf("Computer's move: %s\n", move_str(pv[0][0].b));
	    }
	    fflush(stdout);
	    /* save the "backup" data because it will be overwritten */
	    undo_dat[undos] = hist_dat[0];
	    ++undos;
	    
	    ply = 0;
	    gen();
	    continue;
	  } else if ( networked_game && (side == network_side )) {
		  idx = get_network_move(); /* look for incoming move */
		  if ( !makemove(gen_dat[idx].m.b)) {
		    printf("Illegal move.\n");
		    fflush(stdout);
		  } else {
		    undo_dat[undos] = hist_dat[0];
		    ++undos;
		    redos=0;  /* new line, so clear old redo stack */
		    print_board(stdout);
		    ply = 0;
		    gen();
		  }
	  } else {
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
	    if (!strcmp(s, "accept")) {
	      connect_db("localhost");
	      continue;
	    }
	    if (!strcmp(s, "connect")) {
	      connect_db("server");
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
	    if (!strcmp(s, "diff")) {
	      if ( full_captures )
		full_captures = FALSE;
	      else
		full_captures = TRUE;
	      print_board( stdout );
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
	    if (!strcmp(s, "rotate") || !strcmp(s,"R")) {
	      if ( !rotated ) 
		rotated = TRUE;
	      else
		rotated = FALSE;
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
	      /* gen(); */
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
	      if ( conn ) { /* are we connected */
		printf("Closing connection to database ...\n");
		mysql_close(conn);
	      }
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
	      printf("diff - toggle all captures or diff list\n");
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
	      printf("------------ Display ----------------\n");
	      printf("create - create a new opening file - please read the comments in book.dat!\n");
	      printf("d - display the board again\n");
	      printf("rotate/R - rotate the board and display\n");
	      printf("ascii - display ascii board\n");
	      printf("kanji - display kanji board with one kanji per piece\n");
	      printf("fullkanji - display kanji board with two kanjis per piece\n");
	      printf("------------ File I/O ----------------\n");
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
#ifdef DEBUG
		  fprintf(stderr,"move found: %d %d %d: ", from, over, to);
		  fprintf(stderr,"%d%cx%d%c-%d%c\n", from_file, from_rank, over_file, over_rank, to_file, to_rank);
#endif
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
	      } else { /* if 5== sscanf */
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
	    }
	    if (!found || !makemove(gen_dat[i].m.b)) {
	      printf("Illegal move.\n");
	      fflush(stdout);
	    } else {
	      /* save the "backup" data because it will be overwritten */
	      if ( networked_game ) { 
		printf("sending move idx (2): %d", i);
		sprintf(net_move,"%d",i);
		send_network_move(net_move);
	      }
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




unsigned char *ascii_move_str(move_bytes m)
{
  static unsigned char str[25];
  char igui, igui2 = '-';
  char burn_self = '*';
  
  if (!(m.bits & 2 ) )
    burn_self = ' ';

  if (m.bits & 32) /* igui move */
    igui = '!';
  else 
    if (m.bits & 1) igui = 'x';
    else igui = '-';
  if (m.bits & 4) { /* lion double move */
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%s%d%c%c%d%c%c%d%c+%c", unpadded_piece_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a',burn_self);
    else  
      sprintf(str, "%s%d%c%c%d%c%c%d%c%c", unpadded_piece_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a',burn_self);
  } else { /* FiD will go here */
    if (m.bits & 16)  { /* promotion, it's not an FiD */
      sprintf(str, "%s%d%c%c%d%c+%c",  unpadded_piece_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a', burn_self);
    } else {
      if ( m.burned_pieces == '0' ) {
	sprintf(str, "%s%d%c%c%d%c%c",  unpadded_piece_string[m.oldpiece],
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',burn_self);
      } else { /* cannot be a suicide move, unless hodges */
	sprintf(str, "%s%d%c%c%d%c!%c",  unpadded_piece_string[m.oldpiece],
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',m.burned_pieces);
      }
    }
  }
  return str;
}

unsigned char *kanji_move_str(move_bytes m)
{
  static unsigned char str[25];
  char igui, igui2 = '-';
  char burn_self = '*';
  
  if ( !(m.bits & 2 ) )
    burn_self = ' ';

  if (m.bits & 32) /* igui move */
    igui = '!';
  else 
    if (m.bits & 1) igui = 'x';
    else igui = '-';
  if (m.bits & 4) { /* lion double move */
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%s%d%c%c%d%c%c%d%c+%c", kanji_piece_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a', burn_self);
    else  
      sprintf(str, "%s%d%c%c%d%c%c%d%c", kanji_piece_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a',burn_self);
  } else {
    if (m.bits & 16)  { /* promotion, it's not an FiD */
      sprintf(str, "%s%d%c%c%d%c+%c",  kanji_piece_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a', burn_self);
    } else {
      if ( m.burned_pieces == '0' ) {
	sprintf(str, "%s%d%c%c%d%c%c",  kanji_piece_string[m.oldpiece],
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',burn_self);
      } else { /* cannot be a suicide move, unless hodges */
	sprintf(str, "%s%d%c%c%d%c!%c",  kanji_piece_string[m.oldpiece],
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',m.burned_pieces);
      }
    }
  }
  return str;
}

unsigned char *fullkanji_move_str(move_bytes m)
{
  static unsigned char str[25];
  char igui, igui2 = '-';
  char burn_self = '*';
  
  if ( !(m.bits & 2 ) )
    burn_self = ' ';

  if (m.bits & 32) /* igui move */
    igui = '!';
  else 
    if (m.bits & 1) igui = 'x';
    else igui = '-';
  if (m.bits & 4) { /* lion double move */
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%s%s%d%c%c%d%c%c%d%c+%c", upper_kanji_string[m.oldpiece],  
	      lower_kanji_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a',burn_self);
    else  
      sprintf(str, "%s%s%d%c%c%d%c%c%d%c%c", upper_kanji_string[m.oldpiece],
	      lower_kanji_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a',burn_self);
  } else {
    if (m.bits & 16)  { /* promotion */
      sprintf(str, "%s%s%d%c%c%d%c+%c",  upper_kanji_string[m.oldpiece],
	      lower_kanji_string[m.oldpiece],
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a',burn_self);
    } else {
      if ( m.burned_pieces == '0' ) {
	sprintf(str, "%s%s%d%c%c%d%c%c",  upper_kanji_string[m.oldpiece],
		lower_kanji_string[m.oldpiece],
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',burn_self);
      } else { /* cannot be a suicide move, unless hodges */
	sprintf(str, "%s%s%d%c%c%d%c!%c",  upper_kanji_string[m.oldpiece],
		lower_kanji_string[m.oldpiece],
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',m.burned_pieces);
      }
    }
  }
  return str;
}

/* move_str returns a string with move m in coordinate notation */

unsigned char *move_str(move_bytes m) {
  if ( m.oldpiece == EMPTY ) {
    fprintf(stderr, "move_str called w empty square!\n");
    return("error");
    }

  if (fullkanji) {
    return( fullkanji_move_str( m ));
  } else if ( kanji ) {
    return( kanji_move_str( m ));
  } else {
      return( ascii_move_str( m ));
  }
}

/* half_move_str returns a string with move m in coordinate notation w.o. piece */

unsigned char *half_move_str(move_bytes m)
{
  static unsigned char str[15];
  char igui, igui2 = '-';
  char burn_self = '*';

  if (!(m.bits & 2 ) )
    burn_self = ' ';
  if (m.bits & 32) /* igui move */
    igui = '!';
  else 
    if (m.bits & 1) igui = 'x';
    else igui = '-';
  if (m.bits & 4) {
    if (m.bits & 16)  /* promotion */
      sprintf(str, "%d%c%c%d%c%c%d%c+%c",
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a', burn_self);
    else  
      sprintf(str, "%d%c%c%d%c%c%d%c%c",
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.over),
	      GetRank(m.over) + 'a',
	      igui2,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a', burn_self);
  } else { /* FiD will go here */
    if (m.bits & 16)  /* promotion, cannot be a FiD */
      sprintf(str, "%d%c%c%d%c+%c",
	      16 - GetFile(m.from),
	      GetRank(m.from) + 'a',
	      igui,
	      16 - GetFile(m.to),
	      GetRank(m.to) + 'a', burn_self);
    else {
      if ( m.burned_pieces == '0' ) {
	sprintf(str, "%d%c%c%d%c%c",
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',burn_self);
      } else {
	sprintf(str, "%d%c%c%d%c!%c",
		16 - GetFile(m.from),
		GetRank(m.from) + 'a',
		igui,
		16 - GetFile(m.to),
		GetRank(m.to) + 'a',m.burned_pieces);
      }
    }
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
  if ( full_captures ) {
    print_full_captures();
  } else {
    print_diff_captures();
  }
  if ( lost( side )) {
    fprintf( stdout, "You have lost the game!\n");
  } else if ( in_check( side )) {
    fprintf( stdout, "You're in CHECK!\n");
  }
}


void print_full_captures( void ) {
  int i;
  printf("Captured (all): Sente: ");
  for ( i = 0; i < PIECE_TYPES; i++ ) 
    if (captured[LIGHT][i])
      if ( captured[LIGHT][i] == 1 ) {
	if ( kanji || fullkanji ) 
	  printf("%s%s, ", upper_kanji_string[i], lower_kanji_string[i]);
	else
	  printf("%s, ", piece_string[i]);
      } else {
	if ( kanji || fullkanji ) 
	  printf("%s%s(%d), ", upper_kanji_string[i], lower_kanji_string[i],captured[LIGHT][i]);
	else
	  printf("%s(%d), ", piece_string[i],captured[LIGHT][i]);
      }
  printf("\n          Gote: ");
  for ( i = 0; i < PIECE_TYPES; i++ ) 
    if (captured[DARK][i])
      if ( captured[DARK][i] == 1 ) {
	if ( kanji || fullkanji ) 
	  printf("%s%s, ", upper_kanji_string[i], lower_kanji_string[i]);
	else
	  printf("%s, ", piece_string[i]);
      } else {
	if ( kanji || fullkanji ) 
	  printf("%s%s(%d), ", upper_kanji_string[i], lower_kanji_string[i],captured[DARK][i]);
	else
	  printf("%s(%d), ", piece_string[i],captured[DARK][i]);
      }
   printf("\n");
}


void print_diff_captures( void ) { /* generates a diff list for printing captures */
  int diff_captures[2][PIECE_TYPES];
  int i;
  for ( i = 0; i < PIECE_TYPES; i++ ) {
    diff_captures[0][i] = captured[0][i];
    diff_captures[1][i] = captured[1][i];
    while ( diff_captures[0][i] && diff_captures[1][i] ) {
      diff_captures[0][i]--;
      diff_captures[1][i]--;
    }
  }
  printf("Captured (diff): Sente: ");
  for ( i = 0; i < PIECE_TYPES; i++ ) 
    if (diff_captures[LIGHT][i])
      if ( diff_captures[LIGHT][i] == 1 ) {
	if ( kanji || fullkanji ) 
	  printf("%s%s, ", upper_kanji_string[i], lower_kanji_string[i]);
	else
	  printf("%s, ", piece_string[i]);
      } else {
	if ( kanji || fullkanji ) 
	  printf("%s%s(%d), ", upper_kanji_string[i], lower_kanji_string[i],diff_captures[LIGHT][i]);
	else
	  printf("%s(%d), ", piece_string[i],diff_captures[LIGHT][i]);
      }
  printf("\n          Gote: ");
  for ( i = 0; i < PIECE_TYPES; i++ ) 
    if (diff_captures[DARK][i])
      if ( diff_captures[DARK][i] == 1 ) {
	if ( kanji || fullkanji ) 
	  printf("%s%s, ", upper_kanji_string[i], lower_kanji_string[i]);
	else
	  printf("%s, ", piece_string[i]);
      } else {
	if ( kanji || fullkanji ) 
	  printf("%s%s(%d), ", upper_kanji_string[i], lower_kanji_string[i],diff_captures[DARK][i]);
	else
	  printf("%s(%d), ", piece_string[i],diff_captures[DARK][i]);
      }
   printf("\n");
}


void ascii_print_board( FILE *fd )
{
  
  int i, j = 1;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */
  /* get the last move, should be in undo_dat[undos] */

  if ( undos > 30 ) 
    j = undos - 30;

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

  if ( rotated ) {
   fprintf( fd,"\e[0m\n\n   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");
    fprintf( fd,"  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\np ");
    for (i = NUMSQUARES -1; i >= 0; i--) {
      switch (color[i]) {
      case EMPTY:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd, "║\e[43m   \e[0m");
	  else
	    fprintf( fd, "│\e[43m   \e[0m");
	} else {
	  if ( i % RANKS == 15 )
	    fprintf( fd,"║   ");
	  else
	    fprintf( fd,"│   ");
	}
	break;
      case LIGHT:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║\e[43m\e[4m%s\e[0m", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m\e[4m%s\e[0m", piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║\e[4m%s\e[0m", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[4m%s\e[0m", piece_string[piece[i]]);
	}
	break;
      case DARK:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║\e[43m%s\e[0m", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m%s\e[0m", piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║%s", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│%s", piece_string[piece[i]]);
	}
	break;
      }
      if ((i + 1) % RANKS == 1 && i != 0) {
	if ( annotate && undos && (j <= undos )) {
	  fprintf( fd,"║ %c   %d. %s\n", 'a' + GetRank(i) , j, move_str(undo_dat[j-1].m.b));
	  j++;
	  if ( j <=undos ) {
	    fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢     %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'a' + GetRank(i) - 1); 
	    j++;
	  } else {
	    fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i) - 1); 
	  }
	} else {
	  fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'a' + GetRank(i)-1); 
	}
      }
    }
    fprintf( fd,"║ a\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n");
   fprintf( fd,"\e[0m   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");

  } else { 
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
	    fprintf( fd,"║\e[43m\e[4m%s\e[0m", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m\e[4m%s\e[0m", piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 0 )
	    fprintf( fd,"║\e[4m%s\e[0m", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[4m%s\e[0m", piece_string[piece[i]]);
	}
	break;
      case DARK:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 0 )
	    fprintf( fd,"║\e[43m%s\e[0m", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m%s\e[0m", piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 0 )
	    fprintf( fd,"║%s", piece_string[piece[i]]);
	  else
	    fprintf( fd,"│%s", piece_string[piece[i]]);
	}
	break;
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE) {
	if ( annotate && undos && (j <= undos )) {
	  fprintf( fd,"║ %c   %d. %s\n", 'a' + GetRank(i), j, move_str(undo_dat[j-1].m.b));
	  j++;
	if ( j <=undos ) {
	  fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢     %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'b' + GetRank(i)); 
	  j++;
	} else {
	  fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	}
	} else {
	  fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	}
      }
    }
  fprintf( fd,"║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  }
  if ( undos ) { 
    if ( networked_game )
      fprintf( fd, "%s to move (last move: %d. %s)\n", 
	       side_name[side], undos, move_str(undo_dat[undos-1].m.b));
    else
      fprintf( fd, "%s to move (last move: %d. %s)\n",
	       side_name[side], undos, move_str(undo_dat[undos-1].m.b));

  } else {
    fprintf( fd,"%s to move\n", 
	     side_name[side]);
  }

  if ( redos ) {
   fprintf( fd, "(next move: %d. %s)\n",
	      undos+1, move_str(redo_dat[redos-1].m.b));
  } else {
    fprintf( fd, "(no next move)\n");
  }
  fprintf( fd,"\n");
  fflush(fd);
}

void kanji_print_board( FILE *fd )
{
  
  int i, j =  1;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */
  /* get the last move, should be in undo_dat[undos] */

  if ( undos > 30 ) 
    j = undos - 30;


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

  if ( rotated ) {
   fprintf( fd,"\e[0m\n\n   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");
    fprintf( fd,"  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\np ");
    for (i = NUMSQUARES -1; i >= 0; i--) {
      switch (color[i]) {
      case EMPTY:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd, "║\e[43m   \e[0m");
	  else
	    fprintf( fd, "│\e[43m   \e[0m");
	} else {
	  if ( i % RANKS == 15 )
	    fprintf( fd,"║   ");
	  else
	    fprintf( fd,"│   ");
	}
	break;
      case LIGHT:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║\e[43m\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	}
	break;
      case DARK:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║\e[43m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m%s\e[0m", kanji_piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 15 )
	    fprintf( fd,"║%s", kanji_piece_string[piece[i]]);
	  else
	    fprintf( fd,"│%s", kanji_piece_string[piece[i]]);
	}
	break;
      }
      if ((i + 1) % RANKS == 1 && i != 0) {
	if ( annotate && undos && (j <= undos )) {
	  fprintf( fd,"║ %c   %d. %s\n", 'a' + GetRank(i) , j, move_str(undo_dat[j-1].m.b));
	  j++;
	  if ( j <=undos ) {
	    fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢     %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'a' + GetRank(i) - 1); 
	    j++;
	  } else {
	    fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i) - 1); 
	  }
	} else {
	  fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'a' + GetRank(i)-1); 
	}
      }
    }
    fprintf( fd,"║ a\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n");
   fprintf( fd,"\e[0m   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");

  } else {
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
	    fprintf( fd,"║\e[43m\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	} else {
	  if ( i %RANKS == 0 )
	    fprintf( fd,"║\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
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
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE) {
	if ( annotate && undos && (j <= undos )) {
	  fprintf( fd,"║ %c   %d. %s\n", 'a' + GetRank(i), j, move_str(undo_dat[j-1].m.b));
	  j++;
	  if ( j <=undos ) {
	    fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢     %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'b' + GetRank(i)); 
	    j++;
	  } else {
	    fprintf( fd,"  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'b' + GetRank(i)); 
	  }
	} else {
	  fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	}
      }
    }
    fprintf( fd,"║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  }

  if ( undos ) { 
    fprintf( fd, "%s to move (last move: %d. %s)\n",
	     side_name[side], undos, move_str(undo_dat[undos-1].m.b));
  } else {
    fprintf( fd,"%s to move\n", 
	     side_name[side]);
  }

  if ( redos ) {
   fprintf( fd, "(next move: %d. %s)\n",
	      undos+1, move_str(redo_dat[redos-1].m.b));
  } else {
    fprintf( fd, "(no next move)\n");
  }
  fprintf( fd,"\n");
  fflush(fd);
}

void full_kanji_print_board( FILE *fd )
{
  
  int i, j = 1;
  int from_square, to_square; /* last move */
  int next_from, next_to; /* next move */
  BOOL same_rank = FALSE; /* have to pass every rank twice for the two kanjis */
  /* get the last move, should be in undo_dat[undos] */

  if ( undos > 60 ) 
    j = undos - 60;

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
  if ( rotated ) {
    fprintf( fd,"   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");
    fprintf( fd,"  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\np ");
    for (i =  NUMSQUARES - 1; i>=0 ; i--) {
      switch (color[i]) {
      case EMPTY:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    fprintf( fd, "║\e[43m   \e[0m");
	  else
	    fprintf( fd, "│\e[43m   \e[0m");
	} else {
	  if ( i % RANKS == 15 )
	    fprintf( fd,"║   ");
	else
	  fprintf( fd,"│   ");
	}
	break;
      case LIGHT:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    if ( !same_rank )
	      fprintf( fd,"║\e[43m↓%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"║\e[43m↓%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank ) 
	      fprintf( fd,"│\e[43m↓%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m↓%s\e[0m", lower_kanji_string[piece[i]]);
	} else {
	  if ( i %RANKS == 15 )
	    if ( !same_rank )
	      fprintf( fd,"║↓%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"║↓%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      fprintf( fd,"│↓%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"│↓%s\e[0m", lower_kanji_string[piece[i]]);
	}
	break;
      case DARK:
	if (( i == from_square )||( i == to_square )) {
	  if ( i %RANKS == 15 )
	    if ( !same_rank )
	      fprintf( fd,"║\e[43m⇑%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"║\e[43m⇑%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank ) 
	      fprintf( fd,"│\e[43m⇑%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"│\e[43m⇑%s\e[0m", lower_kanji_string[piece[i]]);
	  
	} else {
	  if ( i %RANKS == 15 )
	    if ( !same_rank )
	      fprintf( fd,"║⇑%s", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"║⇑%s", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      fprintf( fd,"│⇑%s", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"│⇑%s", lower_kanji_string[piece[i]]);
	}
	break;
      }
      if ((i + 1) % RANKS == 1 && i != 0 ) {
	if ( annotate && undos && (j <= undos )) {
	  if ( same_rank ) {
	    fprintf( fd,"║ %c  %d. %s\n", 'a' + GetRank(i), j, move_str(undo_dat[j-1].m.b));
	    j++;
	    if ( undos && (j <= undos )) {
	      fprintf( fd, "  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢    %d. %s \n%c ",
		       j, move_str(undo_dat[j-1].m.b), 'a' + GetRank(i) - 1);
	      j++;
	    } else {
	      fprintf( fd, "  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ",
		       'a' + GetRank(i) - 1);
	    }
	    same_rank = FALSE;
	  } else {
	    fprintf( fd, "║    %d. %s \n  ", j, move_str(undo_dat[j-1].m.b)); 
	    i+=16;
	    same_rank = TRUE;
	    j++;
	  }
	  
	  if (( i == 0 )&& !same_rank) {
	    fprintf( fd, "║    %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'a' + GetRank(i) - 1); 
	    i+=16;
	    same_rank = TRUE;
	    j++;
	  }
	  
	} else { /* if annotate && undos && j <= undos */
	  if ( same_rank ) {
	    fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'a' + GetRank(i) -1); 
	    same_rank = FALSE;
	  } else {
	    fprintf( fd, "║\n  ");
	    i+=16;
	    same_rank = TRUE;
	  }
	}
      }
      if (( i == 0 )&& !same_rank) {
	fprintf( fd, "║\n  ");
	i+=16;
	same_rank = TRUE;	
      }
    }
    
    fprintf( fd,"\e[0m║ a\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n");
    fprintf( fd,"   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");
  } else { /* not rotated */
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
	      fprintf( fd,"║\e[43m↑%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"║\e[43m↑%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank ) 
	      fprintf( fd,"│\e[43m↑%s\e[0m", upper_kanji_string[piece[i]]);
	  else
	    fprintf( fd,"│\e[43m↑%s\e[0m", lower_kanji_string[piece[i]]);
	} else {
	  if ( i %RANKS == 0 )
	    if ( !same_rank )
	      fprintf( fd,"║↑%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"║↑%s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      fprintf( fd,"│↑%s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      fprintf( fd,"│↑%s\e[0m", lower_kanji_string[piece[i]]);
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
	if ( annotate && undos && (j <= undos )) {
	  if ( same_rank ) {
	    fprintf( fd,"║ %c  %d. %s\n", 'a' + GetRank(i), j, move_str(undo_dat[j-1].m.b));
	    j++;
	    if ( undos && (j <= undos )) {
	      fprintf( fd, "  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢    %d. %s \n%c ",
		       j, move_str(undo_dat[j-1].m.b), 'b' + GetRank(i));
	      j++;
	    } else {
	      fprintf( fd, "  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ",
		       'b' + GetRank(i));
	    }
	    same_rank = FALSE;
	  } else {
	    fprintf( fd, "║    %d. %s \n  ", j, move_str(undo_dat[j-1].m.b)); 
	    i-=16;
	    same_rank = TRUE;
	    j++;
	  }
	  
	  if (( i == LASTSQUARE )&& !same_rank) {
	    fprintf( fd, "║    %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'b' + GetRank(i)); 
	    i-=16;
	    same_rank = TRUE;
	    j++;
	  }
	  
	} else { /* if annotate && undos && j <= undos */
	  if ( same_rank ) {
	    fprintf( fd,"║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
	    same_rank = FALSE;
	  } else {
	    fprintf( fd, "║\n  ");
	    i-=16;
	    same_rank = TRUE;
	  }
	}
      }
      if (( i == LASTSQUARE )&& !same_rank) {
	fprintf( fd, "║\n  ");
	i-=16;
	same_rank = TRUE;	
      }
    }
    
    fprintf( fd,"║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  }
  if ( undos ) {
    fprintf( fd, "%s to move (last move: %d. %s)\n",
	     side_name[side], undos, move_str(undo_dat[undos-1].m.b));
  } else {
    fprintf( fd,"%s to move\n", 
	     side_name[side]);
  }

  if ( redos ) {
    fprintf( fd, "(next move: %d. %s)\n",
	     undos+1, move_str(redo_dat[redos-1].m.b));
  } else {
    fprintf( fd, "(no next move)\n");
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
  int burned;
  int line = 0;
  char from_rank, over_rank, to_rank, igui, igui2, prom;
  BOOL promote, found;
  FILE *fh;
  DIR *dp;
  struct dirent *ep;

  fh = fopen(fn,"rb");
  if (! fh) {
    printf("%s: No such file! Possible load files are:\n", fn );    
    
    dp = opendir ("./savegames/");
    if (dp != NULL) {
      while (ep = readdir (dp))
	printf ("%s ", ep->d_name);
      (void) closedir (dp);
      printf("\n");
    }
    else
      perror ("Couldn't open the directory");
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
    if ((( 10 == sscanf(s,"%d.%s%d%c%c%d%c%c%d%c",&move_nr, piece, &from_file, &from_rank, &igui,
			&over_file, &over_rank, &igui2, &to_file, &to_rank))&& (igui2 != '!'))||
	(( 10 == sscanf(s,"%d.%s%d%c%c%d%c%c%d%c*",&move_nr, piece, &from_file, &from_rank, 
			 &igui, &over_file, &over_rank, &igui2, &to_file, &to_rank)) &&
	 (igui2 != '!'))) {

      if ( 11 == sscanf(s,"%d.%s%d%c%c%d%c%c%d%c+", &move_nr, 
			piece, &from_file, &from_rank, &igui,
			&over_file, &over_rank, &igui2, &to_file, &to_rank) ) {
	promote = TRUE;
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
    } else if (( 7 == sscanf(s,"%d.%s%d%c%c%d%c",&move_nr, piece, &from_file, &from_rank, &igui,
			     &to_file, &to_rank)) ||
		 ( 8 == sscanf(s,"%d.%s%d%c%c%d%c!%d",&move_nr, piece, &from_file, 
			       &from_rank, &igui, &to_file, &to_rank, &burned)) || 
		 ( 7 == sscanf(s,"%d.%s%d%c%c%d%c*",&move_nr, piece, &from_file, 
			       &from_rank, &igui,  &to_file, &to_rank))) {
      if ( 8 == sscanf(s,"%d.%s%d%c%c%d%c%c",&move_nr, piece, &from_file, &from_rank, &igui,
		       &to_file, &to_rank, &prom) ) {
	if ( prom == '+' ) promote = TRUE;
      }
      sprintf(hist_s,"%d%c%c%d%c",from_file, from_rank,
	      igui,to_file,to_rank); /* readline stuff */
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
      if ( color[ gen_dat[i].m.b.from] != side )
	continue;
      if ( color[ gen_dat[i].m.b.to] == side )
	continue;
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
      if ( color[ gen_dat[i].m.b.from] != side )
	continue;
      if ( color[ gen_dat[i].m.b.to] == side )
	continue;
      if ( promotion && ! ( gen_dat[i].m.b.bits & 16 )) 
	continue;
      else if  ( !promotion && ( gen_dat[i].m.b.bits & 16 )) 
	continue;
      else if ( !capture && piece[gen_dat[i].m.b.to] != EMPTY )
	continue;
      else if ( capture && piece[gen_dat[i].m.b.to] == EMPTY )
	continue;
      else {
	return(i);
      }
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

  if ((( piece[from] == FIRE_DEMON ) || 
       ( piece[from] == PFIRE_DEMON )) && 
      ! hodges_FiD )
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
#ifdef DEBUG
      printf("%s, ", move_str(gen_dat[i].m.b));
#endif
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
#ifdef DEBUG
	printf("hyp: %s, ", move_str(gen_dat[i].m.b));
#endif
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
	  if (( piece[gen_dat[j].m.b.from] == FIRE_DEMON ) ||
	      ( piece[gen_dat[j].m.b.from] == PFIRE_DEMON )) {
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
  if ( rotated ) {
    if ( fullkanji ) {
      printf("   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");
      printf("  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\np ");
      for (i =  NUMSQUARES - 1; i>=0 ; i--) {
	if ( var_board[i] ) {
	  switch (color[i]) {
	  case EMPTY:
	    if ( i %RANKS == 15 )
	      printf( "║\e[43m   \e[0m");
	    else
	      printf( "│\e[43m   \e[0m");
	    break;
	  case LIGHT:
	    if ( i %RANKS == 15 )
	      if ( !same_rank )
		  printf("║↓%s\e[0m", upper_kanji_string[piece[i]]);
		else
		  printf("║↓%s\e[0m", lower_kanji_string[piece[i]]);
	      else
		if ( !same_rank )
		  printf("│↓%s\e[0m", upper_kanji_string[piece[i]]);
		else
		  printf("│↓%s\e[0m", lower_kanji_string[piece[i]]);
	    break;
	  case DARK:
	    if ( i %RANKS == 15 )
	      if ( !same_rank )
		printf("║\e[43m⇑%s\e[0m", upper_kanji_string[piece[i]]);
	      else
		printf("║\e[43m⇑%s\e[0m", lower_kanji_string[piece[i]]);
	    else
	      if ( !same_rank ) 
		printf("│\e[43m⇑%s\e[0m", upper_kanji_string[piece[i]]);
	      else
		printf("│\e[43m⇑%s\e[0m", lower_kanji_string[piece[i]]);
	    break;
	  }	
	  if ((i + 1) % RANKS == 1 && i != 0 ) {
	    if ( annotate && undos && (j <= undos )) {
	      if ( same_rank ) {
		printf("║ %c  %d. %s\n", 'a' + GetRank(i) - 1, j, move_str(undo_dat[j-1].m.b));
		j++;
		if ( undos && (j <= undos )) {
		  printf( "  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢    %d. %s \n%c ",
			  j, move_str(undo_dat[j-1].m.b), 'b' + GetRank(i) - 1);
		  j++;
		} else {
		  printf( "  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ",
			  'b' + GetRank(i) - 1);
		}
		same_rank = FALSE;
	      } else {
		printf( "║    %d. %s \n  ", j, move_str(undo_dat[j-1].m.b)); 
		i+=16;
		same_rank = TRUE;
		j++;
	      }
	      
	      if (( i == 0 )&& !same_rank) {
		printf( "║    %d. %s \n%c ", j, move_str(undo_dat[j-1].m.b), 'b' + GetRank(i) - 1); 
		i+=16;
		same_rank = TRUE;
		j++;
	      } 
	    } else { /* if annotate && undos && j <= undos */
	      if ( same_rank ) {
		printf("║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
		same_rank = FALSE;
	      } else {
		printf( "║\n  ");
		i+=16;
		same_rank = TRUE;
	      }
	    }
	  }
	  if (( i == 0 )&& !same_rank) {
	    printf( "║\n  ");
	    i+=16;
	    same_rank = TRUE;	
	  }
	} else { /* if var_board[i] */
	  ;
	}
      } /* for i to NUMSQUARES */
      printf("\e[0m║ a\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n");
      printf("   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16\n");
    } else if ( kanji ) {
      ;
    } else { /* ascii */
      ;
    }
  } else {
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
		printf( "║%s %s\e[0m", color_string[var_board[i]], upper_kanji_string[piece[i]]);
	      else
		printf( "║%s %s\e[0m", color_string[var_board[i]], lower_kanji_string[piece[i]]);
	    else
	      if ( !same_rank )
		printf( "│%s %s\e[0m", color_string[var_board[i]], upper_kanji_string[piece[i]]);
	      else
		printf( "│%s %s\e[0m", color_string[var_board[i]], lower_kanji_string[piece[i]]);
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
		printf( "║ %s\e[0m", upper_kanji_string[piece[i]]);
	      else
		printf( "║ %s\e[0m", lower_kanji_string[piece[i]]);
	    else
	      if ( !same_rank )
		printf( "│ %s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "│ %s\e[0m", lower_kanji_string[piece[i]]);
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
	      printf("║%s\e[4m%s\e[0m", 
		     color_string[var_board[i]], kanji_piece_string[piece[i]]);
	    else
	      printf("│%s\e[4m%s\e[0m", 
		     color_string[var_board[i]], kanji_piece_string[piece[i]]);
	    break;
	  case DARK:
	    if ( i % RANKS == 0 )
	      printf("║%s%s\e[0m", 
		     color_string[var_board[i]], kanji_piece_string[piece[i]]);
	    else
	      printf("│%s%s\e[0m", color_string[var_board[i]], 
		     kanji_piece_string[piece[i]]);
	    break;	
	  }
	} else {
	  switch (color[i]) {
	  case EMPTY:
	    if ( i % RANKS == 0 )
	      printf("║   ");
	    else
	      printf("│   ");
	    break;
	  case LIGHT:
	    if ( i % RANKS == 0 )
	      printf("║\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	    else
	      printf("│\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	    break;
	  case DARK:
	    if ( i % RANKS == 0 )
	      printf("║%s", kanji_piece_string[piece[i]]);
	    else
	      printf("│%s", kanji_piece_string[piece[i]]);
	    break;
	  }
	}
	if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	  printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
      }
      printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
    } else { /* ascii board */
      printf("\n\n   16  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1\n");
      printf(  "  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
      for (i = 0; i < NUMSQUARES; ++i) {
	if ( var_board[i] ) {
	  switch (color[i]) {
	  case EMPTY:
	    if ( i % RANKS == 0 )
	      printf("║%s   \e[0m", color_string[var_board[i]]); 
	    else
	      printf("│%s   \e[0m", color_string[var_board[i]]); 
	    break;
	  case LIGHT:
	    if ( i % RANKS == 0 )
	      printf("║%s\e[4m%s\e[0m", color_string[var_board[i]], piece_string[piece[i]]);
	    else
	      printf("│%s\e[4m%s\e[0m", color_string[var_board[i]], piece_string[piece[i]]);
	    break;
	  case DARK:
	    if ( i % RANKS == 0 )
	      printf("║%s%s\e[0m", color_string[var_board[i]], piece_string[piece[i]]);
	    else
	      printf("│%s%s\e[0m", color_string[var_board[i]], piece_string[piece[i]]);
	    break;	
	  }
	} else {
	  switch (color[i]) {
	  case EMPTY:
	    if ( i % RANKS == 0 )
	      printf("║   ");
	    else
	      printf("│   ");
	    break;
	  case LIGHT:
	    if ( i % RANKS == 0 )
	      printf("║\e[4m%s\e[0m", piece_string[piece[i]]);
	    else
	      printf("│\e[4m%s\e[0m", piece_string[piece[i]]);
	    break;
	  case DARK:
	    if ( i % RANKS == 0 )
	      printf("║%s", piece_string[piece[i]]);
	    else
	      printf("│%s", piece_string[piece[i]]);
	    break;
	  }
	}
	if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	  printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
      }
      printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
    }   
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
      } else {	/* can a fire demon burn something on the square? */
	if (( piece[ gen_dat[i].m.b.from ] == FIRE_DEMON )||
	    ( piece[ gen_dat[i].m.b.from ] == PFIRE_DEMON ))
	  {
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
	if (( piece[ gen_dat[i].m.b.from ] == FIRE_DEMON )||
	    ( piece[ gen_dat[i].m.b.from ] == PFIRE_DEMON )) {
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
	      printf( "║%s %s\e[0m",inf_color[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "║%s %s\e[0m",inf_color[var_board[i]], lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│%s %s\e[0m",inf_color[var_board[i]], upper_kanji_string[piece[i]]);
	    else
	      printf( "│%s %s\e[0m",inf_color[var_board[i]], lower_kanji_string[piece[i]]);
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
	      printf( "║ %s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "║ %s\e[0m", lower_kanji_string[piece[i]]);
	  else
	    if ( !same_rank )
	      printf( "│ %s\e[0m", upper_kanji_string[piece[i]]);
	    else
	      printf( "│ %s\e[0m", lower_kanji_string[piece[i]]);
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
	    printf("║%s\e[4m%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  else
	    printf("│%s\e[4m%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  else
	    printf("│%s%s\e[0m",inf_color[var_board[i]], kanji_piece_string[piece[i]]);
	  break;	
	}
      } else {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf("║   ");
	  else
	    printf("│   ");
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	  else
	    printf("│\e[4m%s\e[0m", kanji_piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s", kanji_piece_string[piece[i]]);
	  else
	    printf("│%s", kanji_piece_string[piece[i]]);
	  break;
	}
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	    printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
    printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
  } else { /* ascii board */
    printf("\e[0m\n\n   16  15  14  13  12  11  10  9    8   7   6   5   4   3   2   1\n");
    printf(  "  ╔═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╤═══╗\na ");
    for (i = 0; i < NUMSQUARES; ++i) {
      if ( var_board[i] ) {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf("║\e[42m   \e[0m"); 
	  else
	    printf("│\e[42m   \e[0m");
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║\e[45m\e[4m%s\e[0m", piece_string[piece[i]]);
	  else
	    printf("│\e[45m\e[4m%s\e[0m", piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║\e[45m%s\e[0m", piece_string[piece[i]]);
	  else
	    printf("│\e[45m%s\e[0m", piece_string[piece[i]]);
	  break;	
	}
      } else {
	switch (color[i]) {
	case EMPTY:
	  if ( i % RANKS == 0 )
	    printf("║   "); 
	  else
	    printf("│   ");
	  break;
	case LIGHT:
	  if ( i % RANKS == 0 )
	    printf("║\e[4m%s\e[0m", piece_string[piece[i]]);
	  else
	    printf("│\e[4m%s\e[0m", piece_string[piece[i]]);
	  break;
	case DARK:
	  if ( i % RANKS == 0 )
	    printf("║%s", piece_string[piece[i]]);
	  else
	    printf("│%s", piece_string[piece[i]]);
	  break;
	}
      }
      if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
	printf( "║ %c\n  ╟───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───┼───╢\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
    printf( "║ p\n  ╚═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╧═══╝\n   16  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1\n\n");
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
  redo_dat[redos] = hist_dat[0];
  redos++;
  ply = 1;
  takeback();
  gen();
  if (networked_game) {
    send_network_move("undo");
  }
}

void redo_move( void ) {
  if (!redos) return;
  --redos;
  /* has to be put back onto undo_dat as well
     as taken down from redo_dat */
  hist_dat[0] = redo_dat[redos];
  undo_dat[undos] = hist_dat[0];
  undos++;
  ply = 0;
  /* gen(); */
  makemove(hist_dat[0].m.b);
  if (networked_game) {
    send_network_move("redo");
  }
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
  DIR *dp;
  struct dirent *ep;

  if ( ! defaultinputname ) {
    fprintf(stdout, "Enter name for position file (w.o. extension): ");
    scanf("%s",inputname);
  } else {
    strcpy(inputname, defaultinputname);
  }

  sprintf(piecefile, "%s/%s.pieces",positionpath,inputname); 
  sprintf(colorfile, "%s/%s.colors",positionpath,inputname); 

  if ( !(frompiece=fopen(piecefile,"rb"))) {
    printf("%s: No such file! Possible load files are:\n", piecefile );    
    dp = opendir ("./positions/");
    if (dp != NULL) {
      while (ep = readdir (dp))
	printf ("%s ", ep->d_name);
      (void) closedir (dp);
      printf("\n");
    }
    else
      perror ("Couldn't open the directory");
    fflush(stdout); 
  return;
  }
  if ( !(fromcolor=fopen(colorfile,"rb"))) {
    printf("%s: No such file! Possible load files are:\n", colorfile );    
    dp = opendir ("./positions/");
    if (dp != NULL) {
      while (ep = readdir (dp))
	printf ("%s ", ep->d_name);
      (void) closedir (dp);
      printf("\n");
    }
    else
      perror ("Couldn't open the directory");
    fflush(stdout);
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

BOOL connect_db( char *server_name ) {
#ifndef NETWORKING
  fprintf(stderr,"Sorry, I was compiled w.o. networking support.\n");
  fprintf(stderr,"Install a mysql client with development files and recompile.\n");
  return( FALSE );
#else
  char real_server[128];
  char query[1024];
  char whoami[24];
  char start_new;
  MYSQL_RES *res;
  MYSQL_ROW *row;

  printf("What is your name? ");
  scanf("%s",whoami);

  printf("MySQL client version: %s, connecting ...\n", mysql_get_client_info());

  conn = mysql_init(NULL);
  
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    return( FALSE );
  }

  if ( !strcmp(server_name, "localhost") ) { /* connect to local server */
    if (mysql_real_connect(conn, "localhost", "tenjiku", "tenjiku", "tenjiku", 0, NULL, 0) == NULL) {
      printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
      return( FALSE );
    }
    printf("Connected to mysql server on localhost.\n");
  } else { /* connect to other computer */
    scanf("Enter server to connect to: %s", real_server);
    if (mysql_real_connect(conn, real_server, "tenjiku", "tenjiku", "tenjiku", 0, NULL, 0) == NULL) {
      printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
      return(FALSE);
    }
    printf("Connected to mysql server on %s.\n", real_server);
  }
  /* do I have unfinished games? */
  /* select from games player, opponent, options, 
     started = yes, finished = no */
  sprintf(query, "select all game_id, player, opponent, options, moves_so_far from games where (player='%s' or opponent = '%s')  and started != '0' and finished = 'no'", whoami, whoami);
  printf("%s\n", query); 
  if ( mysql_query(conn, query) ) {
    printf("An error has occurred. (1)\n");
    return( FALSE );
  }
  res = mysql_use_result( conn );
  while ( row = mysql_fetch_row( res ) ) {
    printf("Unfinished game: %s - %s: (a)bort, (c)ontinue, (s)kip?\n", 
	   (char *)row[1], (char *)row[2]);
    while (( start_new = getchar() ) != EOF )
      if ((start_new == 'a')||(start_new == 'c')||(start_new == 's'))
	break;    
    switch (start_new) {
    case 'a':
      abort_game( row );
      break;
    case 's':
      printf("skipping game\n");
      break;
    case 'c':
      load_game_from_db( row );
    }
  }
  /* are there games w.o. opponent? */
  /* select from games where player = nil or opponent = nil */
  
  if ( mysql_query( conn, "select all game_id, player, opponent, options, moves_so_far from games where player = 'nil' or opponent = 'nil'")) {
    printf("An error has occurred. (2)\n");
    return( FALSE );
  }
  res = mysql_use_result(conn);
  while ( row = mysql_fetch_row( res ) ) {
    if ( ! strcmp( (char *)row[1], "nil" ) &&  (strcmp( (char *)row[2], whoami ) != 0)) {
      printf("%s as gote is looking for an opponent. Join game? (y/n)\n", 
	     (char *)row[2] );
      while (( start_new = getchar() ) != EOF )
	if ((start_new == 'y')||(start_new == 'n'))
	  break;
      if ( start_new == 'y') {
	join_game( res, row, whoami, 1 );
	return;
      }
    } else if ( ! strcmp( (char *)row[2], "nil" )  &&  (strcmp( (char *)row[1], whoami ) != 0)) {
      printf("%s as sente is looking for an opponent. Join game? (y/n)\n", 
	     (char *)row[1] );
      while (( start_new = getchar() ) != EOF )
	if ((start_new == 'y')||(start_new == 'n'))
	  break;
      if ( start_new == 'y' ) {
	join_game( res, row, whoami, 0 );
	return;
      }
    }
  }
  /* do I want to start a new game? */
  /* insert into games player opponent = nil, options = current_options,
     started = no, finished = no, moves_so_far='' */
    printf("Do you want to start a new game? (y/n)");

    while (( start_new = getchar() ) != EOF )
      if ((start_new == 'y')||(start_new == 'n'))
	break;

    if ( start_new == 'y') {
      new_db_game( whoami );
      mysql_free_result( res );
    } else {
      printf("OK, closing connection to server then\n");
      if ( conn )
	mysql_close( conn );
      else
	printf("Hmmm, the connection is already down ...\n");
    }
  return( TRUE );
#endif
}

void check_db( void ) {
#ifdef NETWORKING
  if ( ! conn && (looking_for_game || accepting_game)) {
    fprintf(stderr,"Something's wrong - a networked game but no connection!\n");
  }
  ;
#else
  ;
#endif
}

void send_network_move( char *thismove ) {
#ifdef NETWORKING
  char msql_stat[128];
 
  mysql_close( conn );

  conn = mysql_init(NULL);
  
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    return;
  }

  if (mysql_real_connect(conn, "localhost", "tenjiku", "tenjiku", "tenjiku", 0, NULL, 0) == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    return;
  }

 if ( mysql_query( conn, "commit" )) 
    printf("send_network_move: Hmmm, something's wrong with the connection: %s\n", mysql_error( conn )); 
  if (! strcmp(thismove, "")) return;
  sprintf(msql_stat,"update games set last_move = '%s' where game_id = '%s'", thismove, game_id);
  printf("%s\n", msql_stat);
  if ( mysql_query( conn, msql_stat )) 
    printf("send_network_move: Hmmm, something's wrong with the connection: %s\n", mysql_error( conn ));
  strcpy(global_last_move, thismove);
#else
  ;
#endif
}

int get_network_move( void ){ 
  /* keep checking the database until a new move appears in last_move, make it */
  /* this should also check whether the other player has gone away */
  /* and save the position in a local file if the connection is gone */
  int move_idx;
  while ( 1) {
    move_idx = check_network_move(); 
    if (  move_idx != -1 )
      return(move_idx);
    sleep( 1);
  }
}
  
int check_network_move( void ) {
 
  char msql_stat[128], msql_stat2[128];
  char last_move[64];
  MYSQL_RES *res;
  MYSQL_ROW *row;
  int move_idx;

  mysql_close( conn );

  conn = mysql_init(NULL);
  
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    return(-1);
  }

  if (mysql_real_connect(conn, "localhost", "tenjiku", "tenjiku", "tenjiku", 0, NULL, 0) == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    return(-1);
  }


  /*  printf("checking ... "); */
  sprintf(msql_stat, "select all last_move from games where game_id = '%s'", game_id);
  sprintf(msql_stat2, "update games set last_move='' where game_id = '%s'", game_id);
  
  /* has something changed? */
  if ( mysql_query( conn, msql_stat )) {
    printf("%s\n", msql_stat); 
    printf("check_network_move: Hmmm, something's wrong with the connection: %s\n", mysql_error( conn ));
    return(-1);
  } else {
    /* check for non-nil opponent */
    res = mysql_use_result(conn);
    row = mysql_fetch_row( res );
      if ( strcmp((char*)row[0],"") &&
	   strcmp((char*)row[0],global_last_move)) { /* a move or command has arrived */
	strcpy(last_move,(char*)row[0]);
	move_idx = row[0];
	printf("Got a move: %s\n", last_move);
	mysql_free_result( res );
	if ( mysql_query( conn, msql_stat2 )) {
	  printf("check_network_move(2): Hmmm, something's wrong with the connection: %s\n", mysql_error( conn ));
	  return(-1);
	}
	return( move_idx );
      }
  }
}

#ifdef NETWORKING

void abort_game(  MYSQL_ROW *row ) {
  /* marks the game in row as finished */

  char ask;
  printf("This will mark the current game as finished. Proceed? (y/n) ");
    while (( ask = getchar() ) != EOF )
      if (( ask == 'y')||( ask == 'n'))
	break;
      else
	  printf("Please answer y or n:");
    if ( ask == 'n') {
      printf("NOT marking game as finished.\n");
      return;
    } else if ( ask == 'y') {
      fflush(stdin);
      printf("How did the game end? (s)ente won, (g)ote won, (d)raw:");
      while (( ask = getchar() ) != EOF )
	if (( ask == 's')||( ask == 'g')||(ask == 'd'))
	  break;
	else
	  printf("Please answer s, g, or d:");
      /* insert into the row */
  /* insert will not work because the result isn't freed */      
    }
}

void load_game_from_db( MYSQL_ROW *row) {
  
  printf("Loading game from db ...");
  /* get the moves in row[3] and options in row[2] and make them */
  /* get last_move as well, that's the one we'll be looking for */
  printf("done!\n");
}

void join_game( MYSQL_RES *res, MYSQL_ROW *row, char *whoami, int which ) {
  /* which is 0 for sente, 1 for gote */
  char query1[128], query2[128];
  char nplayer[128];
  int err;  


  sprintf(query1, "delete from games where player='%s' and opponent='%s' and options='%s' and started = '0'", (char *)row[1], (char *)row[2], (char *)row[3]);
  if ( which ) { /* options aren't used yet */
    sprintf(query2,"insert into games values ('%s', '%s', '%s', '', now()+0, 'no', '', '')", (char *)row[0],
	   whoami, (char *)row[2] );
    strcpy(nplayer,(char *)row[2]);
    network_side = xside;
    
  } else {
    sprintf(query2,"insert into games values ('%s', '%s', '%s', '', now()+0, 'no', '', '')", (char *)row[0],
	   (char *)row[1], whoami );
    strcpy(nplayer,(char *)row[1]);
    network_side = side;
    rotated = TRUE;
  }

  printf("%s\n",query2);

  strcpy(game_id, (char *)row[0]);

  mysql_free_result( res );


  if ( mysql_query( conn, "start transaction")) {
    printf("Couldn't start transaction: %s\n", mysql_error( conn ));
    return;
  };
 

  if ( mysql_query( conn, query2 )) {
    printf("Couldn't join game: %s\n", mysql_error( conn ));
    return;
  };

  printf("%s\n",query1);

  if ( mysql_query( conn, query1 )) {
    printf("Couldn't delete game request: %s\n", mysql_error( conn ));
  }
  
  if ( mysql_query( conn, "commit")) {
    printf("Couldn't commit: %s\n", mysql_error( conn ));
    return;
  };

  networked_game = TRUE;
  strcpy(local_player, whoami);
  strcpy(network_player, nplayer);
  printf("Game joined.\n");
  print_board( stdout );
}

void new_db_game( char *whoami ) {

  char ask;
  char msql_stat[128];
  struct tm *tmp;
  time_t t;
  int i;

  MYSQL_RES *res;
  MYSQL_ROW *row;
  /* check whether I haven't got a game running or started */

  if ( strcmp(game_id,"") ) {
    printf("You have already started or requested a game! (id %s)\n", game_id);
    return;
  }

 /* generate unique id for the game */
  tmp = localtime(&t);
  sprintf(game_id,"%02d%02d%02d%02d%02d%02d", 
     tmp->tm_mday, tmp->tm_mon+1, tmp->tm_year+1900,
	  tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
 

  printf("OK starting new game. ");
  printf("Do you want to play Sente (Black) or Gote (White)? (s/g)");

  
  while (( ask = getchar() ) != EOF )
    if (( ask == 's')||( ask == 'g'))
      break;

  if ( ask == 's') {
    network_side = xside;
    sprintf(msql_stat, "insert into games values ('%s', '%s', 'nil', '', '0', 'no', '', '')", game_id, whoami);
  } else {
    network_side = side;
    sprintf(msql_stat, "insert into games values ('%s', 'nil', '%s', '', '0', 'no', '', '')", game_id, whoami);
  }

  /* now insert values into table */
  printf("%s\n", msql_stat);
  if ( mysql_query( conn, msql_stat )) {
    printf("Couldn't add new game to database. Sorry.\n");
    return;
  }

  printf("Game added. Let's wait for our opponent.\n");
  networked_game = TRUE;
  strcpy(local_player, whoami );

  sprintf(msql_stat, "select all game_id, player, opponent from games where game_id = '%s'", game_id);

  i= 0;
  while ( i++ < MAXWAIT ) {
    sleep( 1);
    /* has something changed? */
    if ( mysql_query( conn, msql_stat ))  {
      printf("Hmmm, something's wrong with the connection: %s\n", mysql_error( conn ));
    
    } else {
      /* check for non-nil opponent */
      res = mysql_use_result(conn);
      row = mysql_fetch_row( res );
      if ( strcmp("nil", (char *)row[1]) && strcmp("nil", (char *)row[2])) {
	/* somebody has joined */
	if ( !strcmp(whoami,(char *)row[1]) ) { /* I am Sente */
	  strcpy(network_player, (char *)row[2]);
	} else if ( !strcmp(whoami,(char *)row[2]) ) { /* I am Gote */
	  strcpy(network_player, (char *)row[1]);
	} else { /* something went wrong */
	  printf(" Weird.\n");
	}
	printf("%s has joined the game.\n", network_player);
	mysql_free_result( res );
	return;
      } else {
	printf(".");
	mysql_free_result( res );
      }
      fflush(stdout);
    }
  }
  printf("Time is up. Nobody has joined.\n");
  sprintf(msql_stat, "delete from games where game_id='%s'", game_id);
  if ( mysql_query( conn, msql_stat )) {
    printf("Cannot remove game request. You'll have a stale game in the db.\n");
  }
  strcpy(network_player,"");
  strcpy(local_player,"");
  strcpy(game_id,"");
} 

#endif /* NETWORKING */
