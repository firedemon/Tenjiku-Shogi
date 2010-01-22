/*
 *	MAIN.C
 *      E.Werner
 *      derived from
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */

unsigned char tenjiku_version[] = { "0.21" };

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

unsigned char *side_name[2] = { "Sente", "Gote" };
int computer_side;

BOOL search_quiesce = TRUE;
BOOL jgs_tsa = FALSE; /* tsa rule by default off */
BOOL modern_lion_hawk = TRUE; /* tsa rule by default off */
BOOL hodges_FiD = FALSE;

/* get_ms() returns the milliseconds elapsed since midnight,
   January 1, 1970. */

#include <sys/timeb.h>
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
	unsigned char s[256];
	int from, over, to, i;
	int from_file, to_file;
	int over_file, igui2;
       
	char from_rank, over_rank, to_rank, igui, prom, *last_move_ptr;
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
#ifdef CENTIPLY
	max_depth = 400;
#else
	max_depth = 4;
#endif
	strcpy(before_last_move,"");
	for (;;) {

	  found = FALSE;
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
		printf("%s> ", argv[0]);
		fflush(stdout);
		scanf("%s", s);
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
		if (!strcmp(s, "load")|| !strcmp(s,"l")) {
		        load_game("");
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
		if (!strcmp(s, "fid")) {
		  printf("Fire Demon suicide does not burn anything (TSA).\n");
		  fflush(stdout);
		  hodges_FiD = FALSE;
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
		if (!strcmp(s, "undo")) {
		  if (!undos) {
		    fprintf(stderr, "No more undos\n");
				continue;
		  }
		  computer_side = EMPTY;
		  --undos;
		  hist_dat[0] = undo_dat[undos];
		  ply = 1;
		  takeback();
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
		if (!strcmp(s, "xboard")) {
			xboard();
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
			printf("Enter moves, e.g., 2f-2e, 3e3f+, 4b!4c, 4bx3c-3d\n");
			printf("m(oves) - ask for a square and shows moves of the piece\n");
			printf("i(nf) - ask for a square and shows pieces that can move there\n");
			printf("check - shows possible checks\n");
			printf("undo - takes back a move\n");
			printf("------- RULES VARIANTS ----------\n");
			printf("classic - starts a new game with TSA LHk (default)\n");
			printf("modern - starts a new game with Modern Lion Hawk (C.P.Adams)\n");
			printf("ew - starts a new game with changed JGns rule (E.Werner)\n");
			printf("tsa - starts a new game with TSA JGns (default)\n");
			printf("hodges - starts a new game, FiD suicide burns enemy pieces (G.Hodges)\n");
			printf("fid - starts a new game, only FiD suicide only burns own FiD (TSA rule, default)\n");
			printf("------------ I/O ----------------\n");
			printf("create - create a new opening file - please read the comments in book.dat!\n");
			printf("d - display the board\n");
			printf("l(oad) - load a game file\n");
			/* printf("replay - load a game file and replay game\n"); */
			printf("s(ave) - save the current game to a file\n");
			printf("bye/quit  - exit the program\n");
			/* printf("xboard - switch to XBoard mode\n"); */
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
		  }
		  }
		}
		if (!found || !makemove(gen_dat[i].m.b)) {
		  printf("Illegal move.\n");
		  fflush(stdout);
		}
		else {
		  /* save the "backup" data because it will be overwritten */
		  strcpy(before_last_move,s);
		  undo_dat[undos] = hist_dat[0];
		  ++undos;
		  print_board(stdout);
		  ply = 0;
		  gen();
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

void print_board( FILE *fd )
{
  
  int i;
  fprintf( fd,"\n\n   16   15   14   13   12   11   10   9     8    7    6    5    4    3    2    1\n");
  fprintf( fd,"  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    switch (color[i]) {
    case EMPTY:
      fprintf( fd,"|    ");
      break;
    case LIGHT:
      fprintf( fd,"|b%s", piece_string[piece[i]]);
      break;
    case DARK:
      fprintf( fd,"|w%s", piece_string[piece[i]]);
      break;
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      fprintf( fd,"| %c\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  fprintf( fd,"| p\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n   16   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1\n\n");
  fprintf( fd,"%s to move\n\n", side_name[side]);
  fflush(fd);
}


/* xboard() is a substitute for main() that is XBoard
   and WinBoard compatible. See the following page for details:
   http://www.research.digital.com/SRC/personal/mann/xboard/engine-intf.html */

void xboard()
{
	int computer_side;
	unsigned char line[256], command[256];
	int from, to, i;
	BOOL found;

	signal(SIGINT, SIG_IGN);
	printf("\n");
	init();
	gen();
	computer_side = EMPTY;
	for (;;) {
		if (side == computer_side) {
			think(TRUE);
			if (!pv[0][0].u) {
				computer_side = EMPTY;
				continue;
			}
			printf("move %s\n", move_str(pv[0][0].b));
			fflush(stdout);
			makemove(pv[0][0].b);
			undo_dat[undos] = hist_dat[0];
			++undos;
			ply = 0;
			gen();
			continue;
		}
		fgets(line, 256, stdin);
		if (line[0] == '\n')
			continue;
		sscanf(line, "%s", command);
		if (!strcmp(command, "xboard"))
			continue;
		if (!strcmp(command, "new")) {
			init();
			gen();
			computer_side = DARK;
			continue;
		}
		if (!strcmp(command, "quit"))
			return;
		if (!strcmp(command, "force")) {
			computer_side = EMPTY;
			continue;
		}
		if (!strcmp(command, "white")) {
			side = LIGHT;
			xside = DARK;
			undos = 0;
			ply = 0;
			gen();
			computer_side = DARK;
			continue;
		}
		if (!strcmp(command, "black")) {
			side = DARK;
			xside = LIGHT;
			undos = 0;
			ply = 0;
			gen();
			computer_side = LIGHT;
			continue;
		}
		if (!strcmp(command, "st")) {
			sscanf(line, "st %d", &max_time);
			max_time *= 1000;
			max_depth = 100000;
			continue;
		}
		if (!strcmp(command, "sd")) {
			sscanf(line, "sd %d", &max_depth);
			max_time = 1000000;
			continue;
		}
		if (!strcmp(command, "time")) {
			sscanf(line, "time %d", &max_time);
			max_time *= 10;
			max_time /= 30;
			max_depth = 100000;
			continue;
		}
		if (!strcmp(command, "otim")) {
			continue;
		}
		if (!strcmp(command, "go")) {
			computer_side = side;
			continue;
		}
		if (!strcmp(command, "hint")) {
			think(TRUE);
			if (!pv[0][0].u)
				continue;
			printf("Hint: %s\n", move_str(pv[0][0].b));
			fflush(stdout);
			continue;
		}
		if (!strcmp(command, "undo")) {
			if (!undos)
				continue;
			--undos;
			hist_dat[0] = undo_dat[undos];
			ply = 1;
			takeback();
			gen();
			continue;
		}
		if (!strcmp(command, "remove")) {
			if (undos < 2)
				continue;
			--undos;
			hist_dat[0] = undo_dat[undos];
			ply = 1;
			takeback();
			--undos;
			hist_dat[0] = undo_dat[undos];
			ply = 1;
			takeback();
			gen();
			continue;
		}
		from =  8 - line[0] ;
		printf("%s\n",from);
		from += 8 * (8 - (line[1] - '0'));
		printf("%s\n",from);
		to = 8  - line[2] ;
		printf("%s\n",to);
		to += 8 * (8 - (line[3] - '0'));
		printf("%s\n",to);
		found = FALSE;

		/* promotion stuff */
		for (i = 0; i < gen_end[ply]; ++i)
			if (gen_dat[i].m.b.from == from && gen_dat[i].m.b.to == to) {
				found = TRUE;
				if (gen_dat[i].m.b.bits & 16)
					switch (line[4]) {
						case 'n':
							break;
						case 'b':
							i += 1;
							break;
						case 'r':
							i += 2;
							break;
						default:
							i += 3;
							break;
					}
				break;
			}
		if (!found || !makemove(gen_dat[i].m.b)) {
			printf("Error (unknown command): %s\n", command);
			fflush(stdout);
		}
		else {
			undo_dat[undos] = hist_dat[0];
			++undos;
			ply = 0;
			gen();
		}
	}
}

void load_game( void ) {
  unsigned char fn[1024], s[15];
  int from, over, to, i;
  int from_file, to_file;
  int over_file, igui2;
  int line = 0;
  char from_rank, over_rank, to_rank, igui, prom;
  BOOL promote, found;
  FILE *fh;
  printf("Enter load file name: ");
  fflush(stdout);
  scanf("%s",fn);
  fh = fopen(fn,"rb");
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
      if (!undos)
	continue;
      --undos;
      hist_dat[0] = undo_dat[undos];
      ply = 1;
      takeback();
      gen();
      continue;
    }

    promote = FALSE;
    if ( 8 == sscanf(s,"%d%c%c%d%c%c%d%c",&from_file, &from_rank, &igui,
		     &over_file, &over_rank, &igui2, &to_file, &to_rank) ) {
      if ( 9 == sscanf(s,"%d%c%c%d%c%c%d%c%c",&from_file, &from_rank, &igui,
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
    } else if ( 5 == sscanf(s,"%d%c%c%d%c",&from_file, &from_rank, &igui,
			    &to_file, &to_rank) ) {
      if ( 6 == sscanf(s,"%d%c%c%d%c%c",&from_file, &from_rank, &igui,
		       &to_file, &to_rank, &prom) ) {
	if ( prom == '+' ) promote = TRUE;
      }
      from = 16 - from_file;
      from += 16 * (from_rank - 'a');
      to = 16 - to_file;
      to += 16 * (to_rank - 'a');
		    
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
  unsigned char fn[1024];
  int i;
  FILE *fh;
  if (strlen(filename) < 1) {
    printf("Enter save file name: ");
    fflush(stdout);
    scanf("%s",fn);
  } else {
    strcpy(fn, filename);
  }
  fh = fopen(fn,"rb");
  if (fh) {
    printf("File already exists!\n");
    fclose(fh);
    fflush(stdout);
    return;
  }
  fh = fopen(fn,"wb");
  printf("%d\n",undos);
  for (i=0;i < undos; i++) {
    fprintf(fh,"%s\n",half_move_str(undo_dat[i].m.b));
  }
  fclose(fh);
}

void show_moves( void ) {
  /* shows moves of piece on square */
  /* should return number of possible
     moves
  */
  char rank;
  int i, from, file;
  BOOL var_board[NUMSQUARES];

  printf("Which piece (enter square)? ");
  scanf("%d%c",&file,&rank);
  if (((file < 1)&&(file > FILES))||
      ((rank < 'a')&&(rank > 'p')))
    {
      printf("Not a square!\n");
      fflush(stdout);
      return;
    }
  /* init var_board */
  for ( i = 0; i < NUMSQUARES; i++)
    var_board[i] = FALSE;
  from = RANKS - file;
  from += FILES * (rank - 'a');
  for (i = 0; i < gen_end[ply]; ++i) {
    if ( gen_dat[i].m.b.from == from ) {
      printf("%s, ", move_str(gen_dat[i].m.b));
      var_board[gen_dat[i].m.b.to] = TRUE;
    }
  }
  printf("\n All moves.\n");
  /* now print the variant board */
  printf("\n\n   16   15   14   13   12   11   10   9     8    7    6    5    4    3    2    1\n");
  printf("  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    if ( var_board[i] ) {
      if ( color[i] == EMPTY ) printf("| o  ");
      else printf("| x  ");
    }
    else {
      switch (color[i]) {
      case EMPTY:
	printf("|    ");
	break;
      case LIGHT:
	printf("|b%s", piece_string[piece[i]]);
	break;
      case DARK:
	printf("|w%s", piece_string[piece[i]]);
	break;
      }
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      printf("| %c\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  printf("| p\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n   16   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1\n\n");

  fflush(stdout);
}


void show_checks( void ) {

  ;
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
  int i, to, file, level;
  BOOL found_influence = FALSE;
  int var_board[NUMSQUARES];
  int save_piece[NUMSQUARES];
  int save_color[NUMSQUARES];
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

  
  for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	printf("%s, ", move_str(gen_dat[i].m.b));
	var_board[gen_dat[i].m.b.from] = level;
      }
  }
  printf("\n");
  
  /* look at enemy pieces that can walk there */

  side  ^= 1;
  xside ^= 1;

  gen();
  
  for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	printf("%s, ", move_str(gen_dat[i].m.b));
	var_board[gen_dat[i].m.b.from] = level;
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
	printf("%s, ", half_move_str(gen_dat[i].m.b));
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
	printf("%s, ", half_move_str(gen_dat[i].m.b));
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
  }

  /* now print the variant board */

  printf("\n\n   16   15   14   13   12   11   10   9     8    7    6    5    4    3    2    1\n");
  printf("  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    if ( var_board[i] != 0 ) {
       printf("|(%d) ", var_board[i]);
    }
    else {
      switch (color[i]) {
      case EMPTY:
	printf("|    ");
	break;
      case LIGHT:
	printf("|b%s", piece_string[piece[i]]);
	break;
      case DARK:
	printf("|w%s", piece_string[piece[i]]);
	break;
      }
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      printf("| %c\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  printf("| p\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n   16   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1\n\n");

  fflush(stdout);

  gen();
}


