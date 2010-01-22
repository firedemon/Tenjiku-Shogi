/*
 *	EVAL.C
 *
 *      E.Werner, Tenjiku Shogi
 *
 *      originally based on
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 */

/* #define LOGSCORE */
#include <string.h>
#include "defs.h"
#include "data.h"
#include "protos.h"

static long int infl[2] = {0,0}; /* # of moves */
/* 
   Things to be taken into account:
   - piece value (done)
   - number of plies: the more plies, the more certain, so
     the depth_adj value must result in an uncertainty penalty
     favouring the longer variants (done)
   - need an attacker/defender ratio resulting in prefering captures
     with a less valuable piece (in gen_push() already supplied) 
   - need square values for all pieces driving them tendentially
     into the center and up the board; if this is done slightly
     it will only be respected after captures and so on (done)
   - need an influence array along the lines of Steve Evans'
     evaluation function, with attacking and defending squares (done)
 */

/* the values of the pieces last is empty piece */
int piece_value[PIECE_TYPES+1] = {
  /* Pawn, King, DE, G, S, C, I, FL, N, L */
	 10, 10000, 58, 60, 35, 30, 25, 40,
	 35, 72 /*L*/, 75, 160, 38, 86, 
	 105 /*Ph*/, 170, 85, 90, 105, 
	 110 /*VSd*/, 120, 350 /*Ln*/, 110,
	 150, 160, 160, 165, 170, 20, 400, 
	 550 /*RGn*/, 1000 /*GGn*/,300, 
	 400 /*LHk*/, 800, 3500 /*FiD*/, 80,
	 130, 95, 130, 270 /*HT*/, 90, 50,
	 100, 110 /*WH*/,
	 165, 82, 88, 102, 108, 115 /*+G*/,
	 320, 108, 145, 155, 140, 350, 450, 0
};

/* The "pcsq" arrays are piece/square tables. They're values
   added to the material value of the piece based on the
   location of the piece. */

int bgn_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	-20,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0, -20,
	  0, -20,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0, -20,   0,
	  0,   0, -20,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0, -20,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,  15,   0,   0,	  0,   0,  15,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,  20,   0,	  0,  20,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};

int rgn_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,-100,-100,-100,-100,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,-100,-100,-100,-100,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   20,  0,   0,  20,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   15,  10,  10, 15,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};

int vgn_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
         -5,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
         -5,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,  -5,  -5,
         -5,  -5,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,  -5,  -5,
         -5,  -5,   8,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   8,  -5,  -5,
	 -5,   8,   5,   5,   5,   5,   5,   5,	  5,   5,   5,   5,   5,   5,   8,  -5,
	  8,   5,   5,   5,   5,   5,   5,   5,	  5,   5,   5,   5,   5,   5,   5,   8,
	  0,   0,   0,   0,   0,   0, -10, -10, -10, -10,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0, -20, -10,	-10, -20,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0, -20, -20,	-20, -20,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0, -30, -30, -30,	-30, -30, -30,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0, -40, -40, -40,	-40, -40, -40,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0, -50, -50, -50,	-50, -50, -50,   0,   0,   0,   0,   0,
};

int ggn_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0, -30,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0, -50,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0, -50,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0, -50,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,-100,-100,-100,-100,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,-100,-100,-100,-100,   0,   0,   0,   0,   0,   0,
	  0,   0,  15,   5,   5,   5,   5,   5,	  5,   5,   5,   5,   5,  15,   0,   0,
	  0,  15,   5,   5,   5,   5,  10,   5,	  5,  10,   5,   5,   5,   5,  15,   0,
	 10,   5,   5,   5,   5,   5,  10,   5,	  5,  10,   5,   5,   5,   5,   5,  10,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};

int fid_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   1,   1,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   1,   1,   0,
	  0,   0,   1,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   1,   0,   0,
	  0,   1,   1,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   1,   1,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   2,   2,   0,   0,   0,   0,	  0,   0,   0,   0,   2,   2,   0,   0,
	  0,   0,   2,   2,   1,   0,   0,   0,	  0,   0,   0,   1,   2,   2,   0,   0,
	  0,   0,   2,   2,   2,   2,   0,   0,	  0,   0,   2,   2,   2,   2,   0,   0,
	  0,   0,   0,   0,   0,   1,   0,   1,   0,   0,   1,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	-30, -30, -30, -30, -30, -30, -30, -30,	-30, -30, -30, -30, -30, -30, -30, -30,
	-30, -30, -30, -30, -50, -50, -30, -30,	-30, -30, -50, -50, -50, -30, -30, -30,
	-50, -50, -50, -50, -50, -50, -50, -50,	-50, -50, -50, -50, -50, -50, -50, -50,
	-50, -50, -50, -50, -50, -50, -50, -50,	-50, -50, -50, -50, -50, -50, -50, -50,
};


int king_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,  10,  10,  10,	 10,  10,   0,   0,   0,   0,   0,   0,
};

int pawn_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  8,   8,   8,   8,   8,   8,   8,   8,	  8,   8,   8,   8,   8,   8,   8,   8,
	  7,   7,   7,   7,   7,   7,   7,   7,	  7,   7,   7,   7,   7,   7,   7,   7,
	  6,   6,   6,   6,   6,   6,   6,   6,	  6,   6,   6,   6,   6,   6,   6,   6,
	  5,   5,   5,   5,   5,   5,   5,   5,	  5,   5,   5,   5,   5,   5,   5,   5,
	  4,   4,   4,   4,   4,   4,   4,   4,	  4,   4,   4,   4,   4,   4,   4,   4,
	  3,   3,   3,   3,   3,   3,   5,   5,   5,   5,   3,   3,   3,   3,   3,   3,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};

int hf_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,  10,   0,   0,   0,   0,   0,   0,   0,  10,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};

int seg_tsa_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,  10,   0,   0,   0,  10,   0,   0,  10,   0,   0,   0,  10,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};



int default_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  5,   8,   8,   8,   8,   8,   8,   8,	  8,   8,   8,   8,   8,   8,   8,   5,
	  0,   7,   7,   7,   7,   7,   7,   7,	  7,   7,   7,   7,   7,   7,   7,   0,
	  0,   6,   6,   6,   6,   6,   6,   6,	  6,   6,   6,   6,   6,   6,   6,   0,
	  0,   5,   5,   5,   5,   5,   5,   5,	  5,   5,   5,   5,   5,   5,   5,   0,
	  0,   4,   4,   4,   4,   4,   4,   4,	  4,   4,   4,   4,   4,   4,   4,   0,
	  0,   0,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   0,   0,
	  0,   0,   0,   2,   2,   2,   2,   2,	  2,   2,   2,   2,   2,   0,   0,   0,
	  0,   0,   0,   0,   1,   1,   1,   1,	  1,   1,   1,   1,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};





/* just to keep an uninitialised array :-)

int king_pcsq[NUMSQUARES] = {
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,	  0,   0,   0,   0,   0,   0,   0,   0,
};

*/


/* The flip array is used to calculate the piece/square
   values for DARK pieces. The piece/square value of a
   LIGHT pawn is pawn_pcsq[sq] and the value of a DARK
   pawn is pawn_pcsq[flip[sq]] */
int flip[NUMSQUARES] = {
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237,238, 239,
	208, 209, 210, 211, 212, 213, 214, 215,	216, 217, 218, 219, 220, 221, 222, 223,
	192, 193, 194, 195, 196, 197, 198, 199,	200, 201, 202, 203, 204, 205, 206, 207,
	176, 177, 178, 179, 180, 181, 182, 183,	184, 185, 186, 187, 188, 189, 190, 191,	  
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,158, 159,
	128, 129, 130, 131, 132, 133, 134, 135,	136, 137, 138, 139, 140, 141, 142, 143,
	112, 113, 114, 115, 116, 117, 118, 119,	120, 121, 122, 123, 124, 125, 126, 127,
	 96,  97,  98,  99, 100, 101, 102, 103,	104, 105, 106, 107, 108, 109, 110, 111,	  
	 80,  81,  82,  83,  84,  85,  86,  87, 88,   89,  90,  91,  92,  93,  94,  95,
         64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77, 78,  79,
	 48,  49,  50,  51,  52,  53,  54,  55,	 56,  57,  58,  59,  60,  61,  62,  63,
	 32,  33,  34,  35,  36,  37,  38,  39,	 40,  41,  42,  43,  44,  45,  46,  47,
	 16,  17,  18,  19,  20,  21,  22,  23,	 24,  25,  26,  27,  28,  29,  30,  31,	  
	  0,   1,   2,   3,   4,   5,   6,   7,  8,   9,  10,  11,  12,  13,  14,  15,
};



int piece_mat[2];  /* the value of a side's pieces */


int eval()
{
	int i;
	long int score[2];  /* each side's score */
	/* this is the first pass: set up */

	piece_mat[LIGHT] = 0;
	piece_mat[DARK] = 0;

	if ( lost(side)) 
	  return -1000000;
	if ( lost(xside)) 
	  return 1000000;

	for (i = 0; i < NUMSQUARES; ++i) {
		if (color[i] == EMPTY)
		  continue;
		else
		  piece_mat[color[i]] += piece_value[piece[i]];
	}

	/* this is the second pass: evaluate each piece */
	score[LIGHT] = piece_mat[LIGHT];
	score[DARK] = piece_mat[DARK];

	if ( jgs_tsa ) {
	  for (i = 0; i < NUMSQUARES; ++i) {
	    if (color[i] == EMPTY)
	      continue;
	    if (color[i] == LIGHT) {
	      switch (piece[i]) {
	      case PAWN:
		score[LIGHT] += 20 * pawn_tsa_pcsq[i];
		/* fprintf(stderr,"gote:increasing by %d\n", 400 * pawn_tsa_pcsq[flip[i]]); */
		break;
	      case HORNED_FALCON:
		score[LIGHT] += 5* hf_tsa_pcsq[i];
		break;
	      case SOARING_EAGLE:
		score[LIGHT] += 5* seg_tsa_pcsq[i];
		break;
	      case BISHOP_GENERAL:
		score[LIGHT] += 8* bgn_tsa_pcsq[i];
		break;
	      case ROOK_GENERAL:
		score[LIGHT] += 10* rgn_tsa_pcsq[i];
		break;
	      case VICE_GENERAL:
		score[LIGHT] += 15* vgn_tsa_pcsq[i];
		break;
	      case GREAT_GENERAL:
		score[LIGHT] += 15* ggn_tsa_pcsq[i];
		break;				       
	      case FIRE_DEMON:
		score[LIGHT] += 20* fid_tsa_pcsq[i];
		break;
	      case KING:
		score[LIGHT] += 5 * king_tsa_pcsq[i];
		break;
	      default:
		score[LIGHT] += 5 * default_pcsq[i];
		break;
	      }
	    }
	    else {
	      switch (piece[i]) {
	      case PAWN:
		score[DARK] += 20 * pawn_tsa_pcsq[flip[i]];
		/* fprintf(stderr,"sente: increasing by %d\n", 400 * pawn_tsa_pcsq[flip[i]]); */
		break;
	      case HORNED_FALCON:
		score[DARK] += 5* hf_tsa_pcsq[flip[i]];
		break;
	      case SOARING_EAGLE:
		score[DARK] += 5* seg_tsa_pcsq[flip[i]];
		break;
	      case BISHOP_GENERAL:
		score[DARK] += 8* bgn_tsa_pcsq[flip[i]];
		break;
	      case ROOK_GENERAL:
		score[DARK] += 10* rgn_tsa_pcsq[flip[i]];
		break;
	      case VICE_GENERAL:
		score[DARK] += 15* vgn_tsa_pcsq[flip[i]];
		break;
	      case GREAT_GENERAL:
		score[DARK] += 15* ggn_tsa_pcsq[flip[i]];
		break;				       
	      case FIRE_DEMON:
		score[DARK] += 20* fid_tsa_pcsq[flip[i]];
		break;
	      case KING:
		score[DARK] += 5 * king_tsa_pcsq[flip[i]];
		break;
	      default:
		score[DARK] += 5 * default_pcsq[flip[i]];
		break;
	      }
	    }
	  }
	} /* if jgs_tsa */
	/* evaluate the number of possible moves */
#ifdef EVAL_INFLUENCE
	eval_influence();
#endif
	/* the score[] array is set, now return the score relative
	   to the side to move */
	if (side == LIGHT) {
#ifdef LOGSCORE
		fprintf(stderr,"%d plies: ",ply);
		for ( i = 0; i < ply; i++) {
		  fprintf(stderr,"%s ",half_move_str(hist_dat[i].m.b));
		}
		fprintf(stderr,"%d-%d-%d:%d-%d:%d\n", score[LIGHT],score[DARK],2400/ply, 
			infl[LIGHT],infl[DARK],
			score[LIGHT] - score[DARK] - 2400/ply +infl[LIGHT]-infl[DARK]);
#endif
		return(score[LIGHT] - score[DARK] - 2400/ply +infl[LIGHT]-infl[DARK]);
		
	}
#ifdef LOGSCORE
	fprintf(stderr,"%d plies: ",ply);
	for ( i = 0; i < ply; i++) {
	  fprintf(stderr,"%s ",half_move_str(hist_dat[i].m.b));
	}
	fprintf(stderr,"%d-%d-%d:%d-%d:%d\n", score[DARK],score[LIGHT],2400/ply,
			infl[DARK],infl[LIGHT],
		score[DARK] - score[LIGHT] - 2400/ply +infl[DARK]-infl[LIGHT]);
#endif
	return(score[DARK] - score[LIGHT] - 2400/ply +infl[DARK]-infl[LIGHT]);
}

#ifdef EVAL_INFLUENCE

void eval_influence() {

  int i, locallight = 0, localdark = 0;

  for ( i=0; i < NUMSQUARES; i++ ) {
    influences[0][i] = 0;
    influences[1][i] = 0;
  }


  side ^= 1;
  xside ^= 1;

  gen_infl();


  side ^= 1;
  xside ^= 1;
		
  gen_infl();

  for ( i=0; i< NUMSQUARES; i++ ) {
    if ( piece[i] == EMPTY ) { /* simply count the influences */
      locallight += influences[0][i];
      localdark += influences[1][i];
    } else if ( color[i] == LIGHT ) {
      if ( influences[DARK][i] )
	locallight -= piece_value[piece[i]];
      if ( piece[i] == FIRE_DEMON )
	locallight -= 200*piece_value[piece[i]]; /* 200 */
      if ( influences[LIGHT][i] <= influences[DARK][i]) {
	/* fprintf(stderr,"-light idx:%d, color:%d, piece:%d\n",i,color[i],piece[i]); */
	locallight -= 50*piece_value[piece[i]]; /* 50 */
      } else 
	locallight ++;
    } else { /* color[i] == DARK */
      if ( influences[LIGHT][i] )
	localdark -= piece_value[piece[i]];
      if ( piece[i] == FIRE_DEMON )
	localdark -= 200*piece_value[piece[i]]; /* 200 */
      if ( influences[LIGHT][i] >= influences[DARK][i]) {
	/* fprintf(stderr,"-dark idx:%d, color:%d, piece:%d\n",i,color[i],piece[i]); */
	localdark -= 50*piece_value[piece[i]]; /* 50 */
      }
      else
	localdark++;
    }
  }
  infl[DARK] = localdark;
  infl[LIGHT] = locallight;


}

#endif
