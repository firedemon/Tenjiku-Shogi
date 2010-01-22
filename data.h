/*
 *	DATA.H
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */
#include<wchar.h>

/* this is basically a copy of data.c that's included by most
   of the source files so they can use the data.c variables */

extern int color[NUMSQUARES];
extern int piece[NUMSQUARES];
extern int side;
extern int xside;
extern int last_capture;
extern int ply;
extern gen_t gen_dat[MOVE_STACK];
extern int gen_begin[HIST_STACK], gen_end[HIST_STACK];
extern int history[NUMSQUARES][NUMSQUARES];
extern hist_t hist_dat[HIST_STACK];
extern int max_time;
extern int max_depth;
extern long start_time;
extern long stop_time;
extern long nodes;
extern move pv[HIST_STACK][HIST_STACK];
extern int pv_length[HIST_STACK];
extern BOOL follow_pv;
extern hist_t undo_dat[UNDO_STACK];
extern hist_t redo_dat[REDO_STACK];
extern int undos;
extern int redos;
extern int mailbox[400];
extern int mailbox256[NUMSQUARES];
extern int offsets[PIECE_TYPES];
extern int offset[2][PIECE_TYPES][8];
extern int promotion[PIECE_TYPES];
extern int can_promote_zone[2][PIECE_TYPES];
extern int must_promote_zone[2][PIECE_TYPES];
extern unsigned char *piece_string[PIECE_TYPES];
extern unsigned char *kanji_piece_string[PIECE_TYPES];
extern unsigned char *upper_kanji_string[PIECE_TYPES];
extern unsigned char *lower_kanji_string[PIECE_TYPES];
extern unsigned char *TeX_light_piece_string[PIECE_TYPES];
extern unsigned char *TeX_dark_piece_string[PIECE_TYPES];
extern int init_color[NUMSQUARES];
extern int init_piece[NUMSQUARES];
#ifdef EZ
char *pixmap[2][PIECE_TYPES];
#endif
