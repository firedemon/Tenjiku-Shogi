/*
 *	PROTOS.H
 *      E. Werner, Tenjiku Shogi
 *      based on
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */
#ifdef EZ
#include "EZ.h"
#endif
/* prototypes */

/* board.c */
void init();
BOOL lost(int s);
BOOL in_check(int s);
BOOL attack(int sq, int s);
BOOL burning(int where, int sq);
int  eval_burned(int where, int sq);
void burn_infl(int where, int sq);
void gen();
void gen_caps();
void gen_infl();
void gen_push(int from, int to, int bits);
void gen_push_igui(int from, int over, int bits);
void gen_push_lion_move(int from, int over, int to, int bits);
void gen_promote(int from, int to, int bits);
int identifymove(int file, char rank, char prom_or_capture);
BOOL makemove(move_bytes m);
BOOL make_igui(move_bytes m);
BOOL make_lion_move(move_bytes m);
BOOL make_FiD(move_bytes m);
BOOL make_normal_move(move_bytes m);
void takeback();
int get_steps( int i, int j, int s);
int get_steps_infl( int i, int j, int s);
int get_steps_caps( int i, int j, int s);
void special_moves(int i, int s);
void special_moves_infl(int i, int s);
BOOL special_moves_caps(int i, int s, int n);
void check_area(int i, int j, int s, int *area, int are_size, int depth);
void take_back_normal(move_bytes m);
void take_back_igui(move_bytes m);
void take_back_lion_move(move_bytes m);
void take_back_FiD(move_bytes m);
BOOL can_promote(int i, int n);
BOOL must_promote(int i, int n);
BOOL higher(int i, int j);
BOOL suicide(int i);

/* search.c */
void think(BOOL quiet);
int search(int alpha, int beta, int depth);
int quiesce(int alpha, int beta, int depth);
void sort_pv();
void sort(int from);
void checkup();

/* eval.c */
int eval();
void eval_influence();

/* main.c */
long get_ms();
void process_arguments(int argc, char **argv);
int main();
unsigned char *move_str(move_bytes m);
unsigned char *half_move_str(move_bytes m);
void print_board( FILE *fd );
void ascii_print_board( FILE *fd );
void kanji_print_board( FILE *fd );
void full_kanji_print_board( FILE *fd );
void xboard();
#ifdef EZ
void load_game(EZ_Widget *widget, void *dat);
void save_game(EZ_Widget *widget, void *dat);
void really_save( EZ_Widget *widget, void *t);
void really_load( EZ_Widget *widget, void *t);
void load_game( EZ_Widget *widget, void *t);
void save_game( EZ_Widget *widget, void *t);

void clean_exit(EZ_Widget *widget, void *dat);
void popup_help();
void quit_splash(EZ_Widget *widget, void *dat);

void show_influences(char *str);
void show_moves(char *str);
void remove_shown_moves( void );
void clean_exit(EZ_Widget *widget, void *dat);
void quit_splash(EZ_Widget *widget, void *dat);

EZ_Item *CreatePieceItem(int side, int piece, int x, int y);
EZ_Item *CreateMoveItem(int level, int x, int y);
EZ_Item *CreateInfluenceItem(int level, int x, int y);
int GetXPixels(int square);
int GetYPixels(int square);
#else
void load_game();
void really_load_game(char *fn);
void load_old_game();
void save_game();
void show_moves();
#endif
void export_TeX( void );
void read_init_file();
void show_influence();
void show_checks();
void print_line();
void undo_move();
void redo_move();
void goto_position();
void setup_board();
void save_position();
void load_position(char *filename);

void strtouppercase( char *in, char *out);

/* book.c */
BOOL in_book(const char *move, char *return_move);
GDBM_FILE book_init();
void book_create( const char *filename );
void book_exit(GDBM_FILE book);
int analyse_move_string( char *move, char *new_move_buf );

/* kbhit.c (by Alan Cox) */
int kbhit( void );
