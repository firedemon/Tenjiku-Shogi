/*
  Opening book routines for Tenjiku Shogi
  E.Werner, Aug 2000
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<gdbm.h>

#include "defs.h"
#include "data.h"
#include "protos.h"

GDBM_FILE book;

BOOL have_book;

unsigned char book_line[1024] = "";
static char bookname[] = "tenjiku.book";


void book_exit(GDBM_FILE book) {
  gdbm_close(book);
  have_book = FALSE;
}

BOOL in_book(const char *move, char *return_move) {
  int found = 1;

  char new_move[20];
  char sec_move[20];
  datum key, value;

  if (! have_book )
    return FALSE;
  if ( strlen(book_line) > 1016 ) {
    return FALSE; /* will overflow */
  }

  /* first move? */

  if ((strlen(book_line) == 0 )&& (strlen(move) == 0)) {
    strcpy(book_line,"8l8k");
    strcpy(return_move,"8l-8k");
    return TRUE;
  }

  /* parse the last move */

  analyse_move_string(move,new_move);

  if (strlen(new_move) > 1 ) {
    if ( strlen(book_line) < 2 ) {
      strcat(book_line,new_move);
    } else {
      strcat(book_line,":");
      strcat(book_line,new_move);
    }
  }
  /* now look it up */

  key.dptr = book_line;
  key.dsize = strlen(book_line);
  value = gdbm_fetch(book, key);
  if ( value.dptr ) { /* add new move to book_line */
    strncpy(sec_move,value.dptr,value.dsize);
    strncpy(return_move,value.dptr,value.dsize);
    analyse_move_string(sec_move,new_move);
    strcat(book_line,":");
    strcat(book_line,new_move);
    /* and return it */
    return TRUE;
  } else {
      found = 0;
      book_exit(book);
      have_book = FALSE;
      return FALSE;
  }
}

int analyse_move_string(char *move, char *new_move_buf) {
  int from_file, over_file, to_file;
  char from_rank, first_move, over_rank, sec_move, to_rank, prom;
  int elems;
  char new_move[12];

  if (strlen(move) == 0) {
    strcpy(new_move_buf,move);
    return 1;
  }

  elems = sscanf(move,"%d%c%c%d%c%c%d%c%c", 
		 &from_file, &from_rank, &first_move,
		 &over_file, &over_rank, &sec_move,
		 &to_file, &to_rank, &prom);
  switch (elems) {
  case 5: /* normal move */
    if ( first_move == '!' ) /* igui move */
      sprintf(new_move,"%d%c!%d%c", from_file, from_rank, over_file, over_rank);
    else
      sprintf(new_move,"%d%c%d%c", from_file, from_rank, over_file, over_rank);
    break;
  case 6: /* normal move with promotion */
    if ( sec_move == '+' ) {
      if ( first_move == '!' ) /* igui move */
	sprintf(new_move,"%d%c!%d%c+", 
		from_file, from_rank, over_file, over_rank);
      else
	sprintf(new_move,"%d%c%d%c+", 
		from_file, from_rank, over_file, over_rank);
    } else { /* noise in sec_move */
      if ( first_move == '!' )  /* igui move */
	sprintf(new_move,"%d%c!%d%c", 
		from_file, from_rank, over_file, over_rank);
      else
	sprintf(new_move,"%d%c%d%c", 
		from_file, from_rank, over_file, over_rank);
    }
    break;
  case 8: /* lion move */
    sprintf(new_move,"%d%c%d%c%d%c", 
	    from_file, from_rank, over_file, over_rank,
	    to_file, to_rank);
    break;
  case 9:/* lion move w. prom */
    if ( prom == '+' ) {
      sprintf(new_move,"%d%c%d%c%d%c+", 
	      from_file, from_rank, over_file, over_rank,
	      to_file, to_rank);
    } else { /* noise in prom */
      sprintf(new_move,"%d%c%d%c%d%c+", 
	      from_file, from_rank, over_file, over_rank,
	      to_file, to_rank);
    }
    break;
  default:
    fprintf(stderr,"I don't understand this move: %s\n",move);
    break;
  }
  strcpy(new_move_buf, new_move);
  return 0;
}



GDBM_FILE book_init() {
  GDBM_FILE dbf;
  if (!jgs_tsa) { /* no book for my variants :-( */
    have_book = FALSE;
    return NULL;
  }
  strcpy(book_line,"");
  dbf = gdbm_open(bookname,0,GDBM_READER,0,NULL);
  if (!dbf) {
    fprintf(stderr, "Can't open %s\n",bookname);
    have_book = FALSE;
    return NULL;
  }
  have_book = TRUE;
  return dbf;
}

void book_create(const char* filename) {
  int ret, res = 2;
  FILE *in;
  datum key, value;
  char keydata[1024],valuedata[128];

  GDBM_FILE dbf;
  if ( !(in = fopen(filename,"r"))) {
    fprintf(stderr, "Can't open %s\n",filename);
    return;
  }
  fprintf(stderr, "opening dbfile\n");
  dbf = gdbm_open(bookname,0,GDBM_WRCREAT,0,NULL);
  while (res == 2) {
    res = fscanf(in,"%s %s\n",keydata,valuedata);
    key.dptr = keydata;
    key.dsize = strlen(keydata);
    value.dptr = valuedata;
    value.dsize = strlen(valuedata);
    ret = gdbm_store(dbf,key,value,GDBM_REPLACE);
  }
  fclose(in);
  gdbm_close(dbf);
  fprintf(stderr, "Book created\n");
}


