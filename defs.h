/*
 *	DEFS.H
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 *      adapted for Tenjiku Shogi by E. Werner, 2000
 */

#include <gdbm.h>
#include <stdio.h>

/*************************************/
/* compile options for rule variants */
/*************************************/

/* #define japanese_FiD 1 */

#undef japanese_FiD

#undef edo_style_FEg 
/* #define edo_style_FEg 1 */

#define japanese_HT 1

/************************************/
/*  end of user-configurable stuff  */
/************************************/

#define BOOL			int
#define TRUE			1
#define FALSE			0

#define MOVE_STACK		240000
#define HIST_STACK		800
#define UNDO_STACK		800
#define REDO_STACK		800

#define LIGHT			0
#define DARK			1

#define PAWN			0
#define KING			1
#define DRUNKEN_ELEPHANT       	2
#define GOLD			3
#define SILVER			4
#define COPPER			5
#define IRON			6
#define FEROCIOUS_LEOPARD	7
#define KNIGHT			8
#define LANCE			9
#define REVERSE_CHARIOT         10
#define CHARIOT_SOLDIER         11
#define BLIND_TIGER             12
#define KYRIN                   13
#define PHOENIX                 14
#define FREE_KING               15
#define SIDE_MOVER              16
#define SIDE_SOLDIER            17
#define VERTICAL_MOVER          18
#define VERTICAL_SOLDIER        19
#define ROOK                    20
#define LION                    21
#define BISHOP                  22
#define DRAGON_HORSE            23
#define DRAGON_KING             24
#define HORNED_FALCON           25
#define SOARING_EAGLE           26
#define WATER_BUFFALO           27
#define DOG                     28
#define BISHOP_GENERAL          29
#define ROOK_GENERAL            30
#define GREAT_GENERAL           31
#define FREE_EAGLE              32
#define LION_HAWK               33
#define VICE_GENERAL            34
#define FIRE_DEMON              35
/* promotioned-only pieces */
#define CROWN_PRINCE            36
#define FLYING_OX               37
#define FLYING_STAG             38
#define FREE_BOAR               39
#define HEAVENLY_TETRARCHS      40
#define MULTI_GENERAL           41
#define TOKIN                   42
#define WHALE                   43
#define WHITE_HORSE             44

#define PFREE_KING              45
#define PSIDE_MOVER             46
#define PSIDE_SOLDIER           47
#define PVERTICAL_MOVER         48
#define PVERTICAL_SOLDIER       49
#define PROOK                   50
#define PLION                   51
#define PBISHOP                 52
#define PDRAGON_HORSE           53
#define PDRAGON_KING            54
#define PHORNED_FALCON          55
#define PSOARING_EAGLE          56
#define PWATER_BUFFALO          57
#define PBISHOP_GENERAL         58
#define PROOK_GENERAL           59
#define PCHARIOT_SOLDIER        60
/* pfree_eagle, plion_hawk, pfire_demon, pvice_general, pgreat_general */
#define PFREE_EAGLE             61
#define PLION_HAWK              62
#define PFIRE_DEMON             63
#define PVICE_GENERAL           64
#define PGREAT_GENERAL          65

/* that's different move types, so a 
   SM and a +C are *one*. G and +P
   have different symbols though, so they're
   two
*/
#define PIECE_TYPES             66

#define EMPTY			66
#define BOTH                    255

#define NUMSQUARES               256
#define LASTSQUARE               255
#define RANKS                     16
#define FILES                     16

#define GetFile(x)		(x % 16)
#define GetRank(x)		(x / 16)


/* types of moving, like step, area, slide etc. */
/* slide, offsets, and offset are basically the vectors that
   pieces can move in. If slide for the piece is FALSE, it can
   only move one square in any one direction. offsets is the
   number of directions it can move in, and offset is an array
   of the actual directions. */

typedef enum {
  none,
  step,
  two_steps,
  tetrarch, /* HT  sideways 2jump or 3-step */
  slide,
  lion,  /* Ln, LHk, HF, SEg */
  igui_capture,  /* HT igui capture */
  area2, /* VGn */
  area3, /* FiD */
  free_eagle, /* FEg */
  jumpslide,
  htslide /* jump over the adjacent square and then slide */
} move_type;

typedef enum {
  NO_MOVE,      /* no move possible */
  NORMAL,  /* normal move */
  HYPOTHETICAL, /* hypothetical move (something in the way) */
  DANGER, /* can be captured there */
  SUICIDE,   /* will burn there */
  ALREADY_THERE /* already standing there */
} move_possibilities;

extern char *color_string[6];
extern char *inf_color[5];

extern int lion_jumps[16];
extern int lion_single_steps[8];

extern unsigned char *piece_name[PIECE_TYPES+1];
extern char position_file[1024];

extern move_type move_types[PIECE_TYPES][9];

/* e.g. light_captures[FIRE_DEMON] is the number of light FiDs captured */
int captured[2][PIECE_TYPES];

/* number of pieces for both sides since a bare king loses unless
   he can bare the other king in the next move in which case it's
   a draw.  */ 
extern int pieces_count[2];
extern int fire_demons[2];

/* This is the basic description of a move. promote is what
   piece to promote the pawn to, if the move is a pawn
   promotion. bits is a bitfield that describes the move,
   with the following bits:

   1	capture
   2    suicide move
   4    Lion move 2-steps
   8    Lion jump or 1-step
   16	promotion
   32	igui move
   64   burning move

   It's union'ed with an integer so two moves can easily
   be compared with each other. */

typedef struct {
  unsigned char from;
  unsigned char over;
 
  unsigned char to;
  unsigned char promote;
  unsigned char bits;
  unsigned char burned_pieces; /* no of pieces burnt */
  int oldpiece;

} move_bytes;

typedef union {
	move_bytes b;
	long long u;
} tenjikumove;

/* an element of the move stack. it's just a move with a
   score, so it can be sorted by the search functions. */
typedef struct {
	tenjikumove m;
	long score;
} gen_t;

/* an element of the history stack, with the information
   necessary to take a move back. */
typedef struct {
  tenjikumove m;
  int oldpiece;
  int over; /* Ln, HF, SEg, square over which it moves */
  int capture;  
  int capture2; /* Ln, HF, SEg */
  int burn[8]; /* FiD, one time around, so no squares need to be remembered */
  int last_capture;
} hist_t;

extern int computer_side;
extern int network_side;

extern int piece_value[PIECE_TYPES+1];

extern BOOL search_quiesce;

extern BOOL rotated;
/* TRUE if board is rotated */

extern BOOL loaded_position;
/* TRUE if position is loaded from a file, not played from the 
   initial position */

extern BOOL annotate;
/* TRUE if the moves are displayed next to the board in a column */

extern BOOL jgs_tsa; 
/* TRUE if tsa jgs, FALSE for my jgs */

extern BOOL show_hypothetical_moves;
/* TRUE if move command shows hypothetical moves, too */

extern BOOL tame_FiD;
/* TRUE if FiD doesn't burn when it captures */

extern BOOL verytame_FiD;
/* TRUE if FiD cannot capture */

extern BOOL burn_all_FiD;
/* TRUE if FiD also burns friendly pieces */

extern BOOL modern_lion_hawk;
/* TRUE if colin's LHk, FALSE for TSA */

extern BOOL hodges_FiD;
/* TRUE if hodges FiD, FALSE for TSA */

extern BOOL kanji;
/* TRUE if kanji output on terminal, FALSE for ASCII */

extern BOOL fullkanji;
/* TRUE if full kanji output on terminal, FALSE for ASCII or kanji */

extern BOOL full_captures;
/* TRUE if all captures shown on terminal, FALSE for diff list */

extern BOOL networked_game;
/* TRUE if opponent over the network (not yet working) */

extern int depth_adj[PIECE_TYPES];

#ifdef LOG
extern FILE *logfile;
#endif

#ifdef EZ
extern BOOL move_now;
#endif

extern BOOL have_book;
extern GDBM_FILE book;

extern BOOL tenjiku_server;
extern BOOL tenjiku_client;

extern int read_port;
extern int write_port;

extern FILE *read_pipe;
extern FILE *write_pipe;

#ifdef EVAL_INFLUENCE

extern influences[2][NUMSQUARES];

#endif
