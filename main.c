/*
 *	MAIN.C
 *      E.Werner
 *      derived from
 *	Tom Kerrigan's Simple Chess Program (TSCP)
 *
 *	Copyright 1997 Tom Kerrigan
 */

unsigned char tenjiku_version[] = { "0.14" };
extern unsigned char book_line[1024];

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "defs.h"
#include "data.h"
#include "protos.h"


#include "EZ.h"

#define NORMAL 0
#define CAPTURE 1
#define SUICIDE 2


const int boardwidth = 920;
const int boardheight = 980;
const int xoffset = 0;
const int yoffset = 0;
#define squarewidth ((boardwidth-2*xoffset)/FILES)
#define squareheight ((boardheight-2*yoffset)/RANKS)
const int xbigoffset = 10;
const int ybigoffset = 10;
const int panelwidth = 200;




/* global variables for remembering the move parts */

int first_x, sec_x, third_x;
char  first_y, sec_y, third_y;
BOOL sec_click = FALSE, third_click = FALSE;

EZ_Widget *frame, *frame2, *mbar, *panel, *board, *item[5], *dialogue = NULL;
EZ_Widget *short_help, *start, *undo, *redo, *force, *pvbox, *movebox;
EZ_Widget *time_frame, *time_label, *time_entry, *depth_frame, *depth_label, *depth_entry;

/* global array for showing possible moves */
EZ_Item *possible_move[NUMSQUARES];
EZ_Item *piece_item[NUMSQUARES];
EZ_Item *coord[NUMSQUARES];

EZ_Widget *splashframe, *splash, *player_label, *computer_label;


char lastsquare[3];
void start_computer( void );
void new_game( void );
void make_computer_move( void );
void force_move( void );
void draw_board( void );
void update_board( void );
void toggle_coords( void );
void config_coords( void );
void draw_move( const int from_file, const char from_rank, const int to_file, const char to_rank,
		const int over_file, const char over_rank,BOOL prom);
void undraw_move( void );
void display_short_help(int x, const char y);
void set_time( void );
void set_depth( void );

BOOL move_now = FALSE;
BOOL board_coords = FALSE;
BOOL computer_is_thinking = FALSE;
BOOL search_quiesce = FALSE;

void force_move( void ) {
  move_now = TRUE;
}
/*** unfortunately, this does not work
BOOL prom=FALSE; 
void prom_dialogue(int from_square);
void dialoguetrue(EZ_Widget *w, void *d){
  prom = TRUE;
  prom_not_set = FALSE;
  EZ_ReleaseGrab();
  EZ_EnableWidgetTree(frame);
  EZ_DestroyWidget(dialogue);
}
void dialoguefalse(EZ_Widget *w, void *d){
  prom = FALSE;
  prom_not_set = FALSE;
  EZ_ReleaseGrab();
  EZ_EnableWidgetTree(frame);
  EZ_DestroyWidget(dialogue);
}
****/

char shorthelpstring[40];
unsigned char *side_name[2] = { "Sente", "Gote" };
int computer_side;

BOOL jgs_tsa = TRUE; /* tsa rule by default */
BOOL modern_lion_hawk = FALSE; /* tsa rule by default */
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




void dummy(EZ_Widget *widget, void *dat) {
  fprintf(stderr, "I'm not yet implemented!\n");
}

static void board_handler(EZ_Widget *widget, void *data, int etype, XEvent *xev)
{
  char str[256];
  int x, y;
  if ( computer_is_thinking ) return;
  switch(etype)
    {
    case EZ_BUTTON1_PRESS:
      x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth;
      y =  'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight ;
      sprintf(str, "%d%c", x,y);
      display_short_help(x,y);
      show_moves(str);
      fprintf(stderr,"B1:%s\n",str);
      strcpy(lastsquare,str);
      break;
    case EZ_BUTTON1_RELEASE:
      remove_shown_moves();
      if ( third_click ) { /* lion move */
	sec_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ;
	sec_y = 'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
	sprintf(str, "%d%c",sec_x, sec_y);
	fprintf(stderr,"B1relmove3:%s\n",str);
	draw_move(first_x,first_y,sec_x,sec_y,third_x,third_y,TRUE);
      } else if ( sec_click ) { /* dragging move */
	sec_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ;
	sec_y = 'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
	sprintf(str, "%d%c",sec_x, sec_y);
	fprintf(stderr,"B1relmove2:%s\n",str);
	draw_move(first_x,first_y,sec_x,sec_y, -1, 'x', TRUE);
	sec_click = FALSE;
	third_click = FALSE;
      } else {
	sec_click = TRUE;
	first_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ;
	first_y = 'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
	sprintf(str, "%d%c",first_x, first_y);
	fprintf(stderr,"B1relmove1:%s\n",str);
      }
#ifdef DEBUG
      print_board(stderr);
#endif
      break;
    case EZ_BUTTON2_PRESS:
      sprintf(str, "%d%c",
	      FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ,
	      'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight );
      show_influences(str);
      break;
    case EZ_BUTTON2_RELEASE:
      remove_shown_moves();
      if ( sec_click ) {
	third_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth;
	third_y =  'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
	third_click = TRUE;
      } else {
	third_click = FALSE;
      }
	
#ifdef DEBUG
      print_board(stderr);
#endif
      break;
    case EZ_BUTTON3_PRESS:
      sprintf(str, "%d%c",
	      FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ,
	      'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight );
      show_moves(str);
      strcpy(lastsquare,str);
      break;
    case EZ_BUTTON3_RELEASE:
      sprintf(str, "%d%c",
	      FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ,
	      'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight );
      remove_shown_moves();
      if ( third_click ) { /* lion move */
	sec_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ;
	sec_y = 'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
	sprintf(str, "%d%c",sec_x, sec_y);
	fprintf(stderr,"B3relmove3:%s\n",str);
	draw_move(first_x,first_y,sec_x,sec_y,third_x,third_y,FALSE);
      } else if ( sec_click ) { /* dragging move */
	sec_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ;
	sec_y = 'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
	sprintf(str, "%d%c",sec_x, sec_y);
	draw_move(first_x,first_y,sec_x,sec_y, -1, 'x', FALSE);
      } else {
	sec_click = TRUE;
	first_x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth ;
	first_y = 'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight;
      }
#ifdef DEBUG
      print_board(stderr);
#endif
      break;
      /* 8 case EZ_POINTER_MOTION:
	x = FILES - (EZ_PointerCoordinates[0] - xoffset - 20) / squarewidth;
	y =  'a' + ( EZ_PointerCoordinates[1] - yoffset-12)/ squareheight ;
      break;
    case EZ_KEY_PRESS:
      sprintf(str,"KeyPress: key=%c, keycode=%d", EZ_PressedKey,EZ_PressedKey);
      break;
    case EZ_LEAVE_WINDOW:
      sprintf(str, "LeaveWindow");
      break;
    case EZ_ENTER_WINDOW:
      sprintf(str, "EnterWindow");
      break;      
    case EZ_REDRAW:
      sprintf(str, "Redraw");
      break;
      */
    default:
      str[0]=0;
      break;
    }
  
   if(str[0]) /* EZ_ConfigureWidget(label, EZ_LABEL_STRING, str, 0); */
     fprintf(stderr,"%s\n",str);
}


static EZ_Widget *toplevel = NULL;     /* toplevel frame */
static Atom      deleteWindowAtom;     
static Atom      WMProtocolsAtom;
static int       scaleImage = 0;       /* flag to scale bg image */

static void eventHandler(EZ_Widget *widget, void *data, int etype, XEvent *xev)
{
  if(xev->type == ConfigureNotify) /* && scaleImage == 1) */
    {
      /* scaleSetBgImage(1) */;
    }
}

/*----------------------------------------------------------------------
 * handle  WM_DELETE_WINDOW message from the window manager.
 */
void clientMessageHandler(EZ_Widget *widget, void *data, int etype, XEvent *xev)
{
  XClientMessageEvent *ev = (XClientMessageEvent *)xev;      
      
  if(ev->message_type == WMProtocolsAtom)
    {
      Atom c = (ev->data.l)[0];
      if(c == deleteWindowAtom)
	{
          if(widget != toplevel)  EZ_HideWidget(widget);
	  else
            {
              EZ_Shutdown();
              exit(0);
            }
	}
    }
}


static void setupPanel(EZ_Widget *panel) {
  /* start, undo, short_help is global */
  player_label = EZ_CreateWidget(EZ_WIDGET_LABEL, panel,
				 EZ_LABEL_STRING, (side? "Gote to move" : "Sente to move"),
				 EZ_GRID_CELL_GEOMETRY, 0,1,1,1, 
				 EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
				 NULL);
  /*  time_frame =  EZ_CreateWidget(EZ_WIDGET_FRAME, panel, NULL);*/
  time_label =  EZ_CreateWidget(EZ_WIDGET_LABEL, panel,
				EZ_LABEL_STRING, "Sec/move:",
				EZ_GRID_CELL_GEOMETRY, 0,2,1,1, 
				EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
				NULL);
  time_entry = EZ_CreateWidget(EZ_WIDGET_ENTRY, panel,
			       EZ_CALLBACK, set_time,
			       EZ_BACKGROUND, "white",
			       EZ_BORDER_TYPE, EZ_BORDER_SUNKEN,
			       EZ_GRID_CELL_GEOMETRY, 1,2,1,1, 
			       EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			       EZ_WIDTH,10,
			       EZ_TEXT_LINE_LENGTH, 8,
			       NULL);

  /*  depth_frame =  EZ_CreateWidget(EZ_WIDGET_FRAME, panel, NULL);*/
  depth_label =  EZ_CreateWidget(EZ_WIDGET_LABEL, panel,
				EZ_LABEL_STRING, "Search depth:",
				EZ_GRID_CELL_GEOMETRY, 0,3,1,1, 
				EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
				NULL);
  depth_entry = EZ_CreateWidget(EZ_WIDGET_ENTRY, panel,
				EZ_CALLBACK, set_depth,
				EZ_BORDER_TYPE,EZ_BORDER_SUNKEN,
				EZ_BACKGROUND, "white",
				EZ_ENTRY_STRING, "4",
				EZ_GRID_CELL_GEOMETRY, 1,3,1,1, 
				EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
				EZ_WIDTH,10,
				EZ_TEXT_LINE_LENGTH, 5,NULL);

  pvbox = EZ_CreateWidget(EZ_WIDGET_FANCY_LISTBOX,panel,
			  EZ_WIDTH, panelwidth,
			  EZ_HEIGHT,150,
			  EZ_GRID_CELL_GEOMETRY, 0,4,2,1, 
			  EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			  EZ_OPTIONAL_HSCROLLBAR, True,
			  EZ_OPTIONAL_VSCROLLBAR, True,
			  NULL);
  computer_label = EZ_CreateWidget(EZ_WIDGET_LABEL, panel,
				   EZ_GRID_CELL_GEOMETRY, 0,5,1,1, 
				   EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
				   EZ_LABEL_STRING, "Computer is off",
				   NULL);
  start = EZ_CreateWidget(EZ_WIDGET_NORMAL_BUTTON, panel,
			 EZ_LABEL_STRING, "Start computer",
			  EZ_GRID_CELL_GEOMETRY, 0,6,1,1, 
			  EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			 EZ_CALLBACK,start_computer, NULL, NULL);
  force = EZ_CreateWidget(EZ_WIDGET_NORMAL_BUTTON, panel,
			 EZ_LABEL_STRING, "Force Move",
			    EZ_GRID_CELL_GEOMETRY, 0,7,1,1, 
			    EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			 EZ_CALLBACK,force_move, NULL, NULL);
  movebox = EZ_CreateWidget(EZ_WIDGET_FANCY_LISTBOX,panel,
			    EZ_WIDTH, panelwidth,
			    EZ_HEIGHT,400,
			    EZ_GRID_CELL_GEOMETRY, 0,8,2,1, 
			    EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			    EZ_OPTIONAL_HSCROLLBAR, True,
			    EZ_OPTIONAL_VSCROLLBAR, True,
			    NULL);
  undo = EZ_CreateWidget(EZ_WIDGET_NORMAL_BUTTON, panel,
			 EZ_LABEL_STRING, "Take back ply",
			 EZ_GRID_CELL_GEOMETRY, 0,9,1,1, 
			 EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			 EZ_CALLBACK,undraw_move, NULL, NULL);
  redo =  EZ_CreateWidget(EZ_WIDGET_NORMAL_BUTTON, panel,
			  EZ_LABEL_STRING, "Replay ply",
			  EZ_GRID_CELL_GEOMETRY, 1,9,1,1, 
			  EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			  /* EZ_CALLBACK,undraw_move, NULL, */
			  NULL);

  short_help = EZ_CreateWidget(EZ_WIDGET_LABEL, panel,
			       EZ_GRID_CELL_GEOMETRY, 0,10,1,1, 
			       EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
			       EZ_LABEL_STRING, "          ",
			      NULL);
  EZ_CreateWidget(EZ_WIDGET_NORMAL_BUTTON, panel,
		  EZ_LABEL_STRING, "Toggle Coords",
		  EZ_GRID_CELL_GEOMETRY, 0,11,1,1, 
		  EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
		  EZ_CALLBACK,toggle_coords, NULL, NULL);

}


static void setupMenu(EZ_Widget *mbar) {
  EZ_Widget *menu, *tmp, *mitem, *settingsmenu;

  /* font menu */
  menu =     EZ_CreateWidgetXrm(EZ_WIDGET_MENU,      NULL,
				"FileMenu", "filemenu",
				EZ_MENU_TEAR_OFF,    1,
				NULL);
  
  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,
			     EZ_LABEL_STRING,               "Load File",
			     EZ_UNDERLINE,                  0,
			     EZ_CALLBACK,                   load_game, NULL,
			     NULL);


  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,
			     EZ_LABEL_STRING,               "Save to File",
			     EZ_UNDERLINE,                  0,
			     EZ_CALLBACK,                   save_game, NULL,
			     NULL);

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_SEPARATOR, menu, 0);
  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_SEPARATOR, menu, 0);


  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,
			     EZ_LABEL_STRING,               "Quit",
			     EZ_UNDERLINE,                  0,
			     EZ_CALLBACK,                   clean_exit, NULL,
			     NULL);

  mitem = EZ_MenuBarAddItemAndMenu(mbar, "File", 0, menu);



  /* Game Settings */

  menu =     EZ_CreateWidgetXrm(EZ_WIDGET_MENU,      NULL,
				"SettingsMenu", "settingsmenu",
				EZ_MENU_TEAR_OFF,    1,
				EZ_CALLBACK,             dummy, NULL,
				NULL);

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,  
                             EZ_LABEL_STRING,         "New Game",
			     EZ_UNDERLINE,                  0,
                             EZ_CALLBACK,             new_game, NULL,
                             0);

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_SEPARATOR, menu, 0);

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,  
                             EZ_LABEL_STRING,         "Start Computer Opponent",
			     EZ_UNDERLINE,                  15,
                             EZ_CALLBACK,             start_computer, NULL,
                             0);

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,  
                             EZ_LABEL_STRING,         "Search Time",
			     EZ_UNDERLINE,                  7,
                             EZ_CALLBACK,             dummy, NULL,
                             0);

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,  
                             EZ_LABEL_STRING,         "Search Depth",
			     EZ_UNDERLINE,                  7,
                             EZ_CALLBACK,             dummy, NULL,
                             0);
  
  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU, menu, 0);

  /* EZ_SetSubMenuMenu(tmp, settingsmenu); */

  tmp  =     EZ_CreateWidget(EZ_WIDGET_MENU_NORMAL_BUTTON,  menu,  
                             EZ_LABEL_STRING,         "Rule Variants",
			     EZ_UNDERLINE,                  5,
                             EZ_CALLBACK,             dummy, NULL,
                             0);


  EZ_SetSubMenuMenu(tmp, settingsmenu);

  mitem = EZ_MenuBarAddItemAndMenu(mbar, "Playing", 0, menu);

}


char last_move[15], before_last_move[1024], last_move_buf[1024];

/* main() is basically an infinite loop that either calls
   think() when it's the computer's turn to move or prompts
   the user for a command (and deciphers it). */

int main(int argc,  char **argv)
{
  int i;

   /* to prevent overflowing from nonsense */
  /*
    BOOL found, promote;
  */

	/*
	char      *value;
	*/
        char splashtext[1024] = { "Tenjiku Shogi by E.Werner\nderived from\nTom Kerrigan's TSCP\n Version " };
	strcat(splashtext,tenjiku_version);

	EZ_Initialize(argc,argv,0);


	for (i=0; i< NUMSQUARES; i++) {
	  possible_move[i] = NULL;
	}

	splashframe  = EZ_CreateWidgetXrm(EZ_WIDGET_FRAME,    NULL,
                              "Tenjiku",           "tenjiku",
                              EZ_FILL_MODE,       EZ_FILL_BOTH,
                              EZ_IPADX,           4,
                              EZ_SIZE,            300, 100,
                              0);
	splash = EZ_CreateWidgetXrm(EZ_WIDGET_NORMAL_BUTTON,
				    splashframe,
                              "Tenjiku",           "tenjiku",
                              EZ_LABEL_STRING,    splashtext,
                              EZ_CALLBACK, quit_splash, splashframe,
                              0);
	EZ_DisplayWidget(splashframe);
	EZ_DisplayWidget(splash);
	EZ_WaitAndServiceNextEvent();
	EZ_InitializeXrm("Tenjiku",NULL,0,argc,argv,NULL,0);
	WMProtocolsAtom  = EZ_GetAtom("WM_PROTOCOLS");
	deleteWindowAtom = EZ_GetAtom("WM_DELETE_WINDOW");
	EZ_SetClientMessageHandler(clientMessageHandler,   NULL);
	
	frame  = EZ_CreateWidgetXrm(EZ_WIDGET_FRAME,   NULL,
				    "Toplevel",       "toplevel",
				    EZ_SIZE,           boardwidth+panelwidth+30, boardheight+2*yoffset+20,
				    EZ_PADX,           0,
				    EZ_PADY,           0,
				    EZ_IPADX,          5,
				    EZ_IPADY,          5,
				    EZ_FILL_MODE,      EZ_FILL_BOTH,
				    EZ_ORIENTATION,    EZ_VERTICAL,
				    EZ_EVENT_HANDLER,  eventHandler, NULL,
				    0);
	toplevel = frame;


	frame2  = EZ_CreateWidgetXrm(EZ_WIDGET_FRAME,   frame,
				    "Frame2",       "frame2",
				    EZ_SIZE,           boardwidth+2*xoffset + panelwidth+30, boardheight+2*yoffset+10,
				    EZ_PADX,           5,
				    EZ_PADY,           5,
				    EZ_FILL_MODE,      EZ_FILL_BOTH,
				    EZ_ORIENTATION,    EZ_HORIZONTAL,
				    EZ_EVENT_HANDLER,  eventHandler, NULL,
				    0);


	/*board  = EZ_CreateWidgetXrm(EZ_WIDGET_FRAME,   frame2,
				    "Board",       "board",
				    EZ_SIZE,           700, 80,
				    EZ_PADX,           0,
				    EZ_PADY,           0,
				    EZ_FILL_MODE,      NULL,
				    EZ_EVENT_HANDLER,  eventHandler, NULL,
				    0);
	*/
	board  = EZ_CreateWidgetXrm(EZ_WIDGET_WORK_AREA,   frame2,
				    "Board",       "board",
				    EZ_SIZE,           boardwidth+2*xoffset, boardheight+2*yoffset,
				    EZ_PADX,           0,
				    EZ_PADY,           0,
				    EZ_RUBBER_BAND,   0,
				    EZ_ITEM_MOVABLE,  False,
				    EZ_BACKGROUND, "white",
				    EZ_OPTIONAL_HSCROLLBAR, 0,
				    EZ_OPTIONAL_VSCROLLBAR, 0,
				    EZ_EVENT_HANDLER,  board_handler, NULL,
				    0);
	panel  = EZ_CreateWidgetXrm(EZ_WIDGET_FRAME,   frame2,
				    "Panel",       "panel",
				    EZ_SIZE,           panelwidth, boardheight,
				    EZ_PADX,           2,
				    EZ_PADY,           0,
				    EZ_IPADX,          2,
				    EZ_ORIENTATION,    EZ_VERTICAL,
				    EZ_FILL_MODE,      NULL,
				    EZ_EVENT_HANDLER,  eventHandler, NULL,
				    0);

	mbar =  EZ_CreateWidgetXrm(EZ_WIDGET_MENU_BAR, panel,
				   "MenuBar",           "menubar",
				   EZ_GRID_CELL_GEOMETRY, 0,0,1,1, 
				   EZ_GRID_CELL_PLACEMENT, EZ_FILL_BOTH, EZ_LEFT,
				   EZ_DOCKABLE,            1,
				   NULL);
	setupMenu(mbar);
	setupPanel(panel);

	EZ_SetWorkAreaGeometryManager(board, NULL, NULL); 

	/* disable the default GM manager */

	EZ_DisplayWidget(frame);
	EZ_DisplayWidget(frame2);
	EZ_DisplayWidget(board);
	EZ_DisplayWidget(panel);
	EZ_DisableHighlight();
	EZ_DisableImageDithering();
	
	draw_board();
	new_game();

	EZ_EventMainLoop();
	exit(1);
}


void clean_exit(EZ_Widget *widget, void *dat)  /* push button callback */
{
  EZ_Shutdown();/* shutdown EZWGL */
  exit(0);      /* and exit       */
}

void quit_splash(EZ_Widget *widget, void *dat) {
  EZ_DestroyWidget(widget);
  EZ_DestroyWidget(splashframe);
}



/********************** OLD FUNCTIONS CALLED BY OTHER FILES ***************/

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

void really_load(EZ_Widget *widget, void *dat) {
  unsigned char s[15], new_book[15];
  char *str = EZ_GetFileSelectorSelection(widget);
  int from, over, to, i;
  int from_file, to_file;
  int over_file, igui2;
  int line = 0;
  char from_rank, over_rank, to_rank, igui, prom;
  BOOL promote, found;
  FILE *fh;
  fprintf(stderr, "really loading %s\n", str);
  if (!strlen(str)) return;
  fh = fopen(str,"rb");
  if (! fh) {
    printf("No such file!\n");
    fflush(stdout);
    return;
  }
  EZ_DestroyWidget(widget);
  new_game();

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
#ifdef DEBUG
      print_board(stdout);
#endif
      fflush(stdout);
      return;
    } else {
      draw_move(from_file,from_rank,to_file,to_rank,-1,'x', promote);
      /* put into book_line */
      analyse_move_string(before_last_move,new_book);
      if ( strlen(book_line) == 0 ) {
	strcpy(book_line,new_book);
	strcpy(before_last_move,s);
      } else {
	strcat(book_line,":");
	strcat(book_line,new_book);
	strcpy(last_move_buf,before_last_move);
	strcpy(before_last_move,s);
      }
      /* put into the box */
      EZ_AppendListBoxItem(movebox,s);
      /* save the "backup" data because it will be overwritten */
      undo_dat[undos] = hist_dat[0];
      ++undos;
      ply = 0;
      gen();
    }

  } while (!feof(fh));
  printf("%d plies\n", undos );

  fclose(fh);
  update_board();
#ifdef DEBUG
  print_board(stdout);
  fflush(stdout);
#endif
}



void load_game(EZ_Widget *widget, void *dat ) {


  EZ_Widget *tmp;

  tmp = EZ_CreateWidget(EZ_WIDGET_FILE_SELECTOR, NULL,
                        EZ_CALLBACK, really_load, NULL,
                        EZ_ROW_BG, 1, "white", "bisque",
			EZ_BG_IMAGE_FILE, "bg1.gif",
                        EZ_SELECTION_BACKGROUND, "yellow",
			EZ_GLOB_PATTERN, "*.tnj",
                        0);

  EZ_DisplayWidget(tmp);
}


void save_game(EZ_Widget *widget, void *dat ) {

  EZ_Widget *tmp;

  tmp = EZ_CreateWidget(EZ_WIDGET_FILE_SELECTOR, NULL,
                        EZ_CALLBACK, really_save, NULL,
                        EZ_ROW_BG, 1, "white", "bisque",
			EZ_BG_IMAGE_FILE, "bg1.gif",
                        EZ_SELECTION_BACKGROUND, "yellow",
			EZ_GLOB_PATTERN, "*.tnj",
                        0);

  EZ_DisplayWidget(tmp);

}


void really_save( EZ_Widget *widget, void *t) {
  char *str = EZ_GetFileSelectorSelection(widget);
  int i;
  FILE *fh;
  fprintf(stderr,"really saving to %s\n",str);
  if (!strlen(str)) return;
  fh = fopen(str,"wb");
  printf("%d\n",undos);
  for (i=0;i < undos; i++) {
    fprintf(fh,"%s\n",half_move_str(undo_dat[i].m.b));
  }
  fclose(fh);
  EZ_DestroyWidget(widget);
}

void show_moves(char *str ) {
  /* shows moves of piece on square */
  /* should return number of possible
     moves
  */
  char rank;
  int i, from, file, show = 0;
  BOOL var_board[NUMSQUARES];

  sscanf(str,"%d%c",&file,&rank);
  if (((file < 1)&&(file > 16))||
      ((rank < 'a')&&(rank > 'p')))
    {
      fprintf(stderr,"Not a square!\n");
      return;
    }
  /* init var_board */
  EZ_FreezeWidget(board);
  for ( i = 0; i < NUMSQUARES; i++)
    var_board[i] = FALSE;
  from = 16 - file;
  from += 16 * (rank - 'a');
  for (i = 0; i < gen_end[ply]; ++i) {
    int x, y, to;
    if ( gen_dat[i].m.b.from == from ) {
      to = gen_dat[i].m.b.to;
      fprintf(stderr,"%s, ", move_str(gen_dat[i].m.b));
      var_board[to] = TRUE;
      x = GetXPixels(to);
      y = GetYPixels(to);
      if ( gen_dat[i].m.b.bits & 2) 
	possible_move[show] = CreateMoveItem(SUICIDE,x,y);
      else if ( gen_dat[i].m.b.bits & 1)
	possible_move[show] = CreateMoveItem(CAPTURE,x,y);
      else 
	possible_move[show] = CreateMoveItem(NORMAL,x,y);
      EZ_WorkAreaInsertItem(board, possible_move[show]);  
      EZ_ConfigureItem(possible_move[show],NULL);
      show++;
    }
  }
  EZ_UnFreezeWidget(board);
  fprintf(stderr,"\n All moves.\n");
  fprintf(stderr,"\n\n   16   15   14   13   12   11   10   9     8    7    6    5    4    3    2    1\n");
  fprintf(stderr,"  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    if ( var_board[i] ) {
      if ( color[i] == EMPTY ) { 
	fprintf(stderr,"| o  ");
      }  else {
	fprintf(stderr,"| x  ");
      }
    }
    else {
      switch (color[i]) {
      case EMPTY:
	fprintf(stderr,"|    ");
	break;
      case LIGHT:
	fprintf(stderr,"|b%s", piece_string[piece[i]]);
	break;
      case DARK:
	fprintf(stderr,"|w%s", piece_string[piece[i]]);
	break;
      }
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      fprintf(stderr,"| %c\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  fprintf(stderr,"| p\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n   16   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1\n\n");

}




void show_influences(char *str ) {
  /* shows pieces that can move on square */
  /* must fake a piece for an empty square
     to correctly reflect JGs, furthermore
     must remove the defenders and regenerate
     to correctly reflect stacked pieces
     until theres no more influences found.
     Same thing must be done for other side.
  */
  char rank;
  int i, x, y, from, to, file, level, show=0;
  BOOL found_influence = FALSE;
  int var_board[NUMSQUARES];
  int save_piece[NUMSQUARES];
  int save_color[NUMSQUARES];
  sscanf(str,"%d%c",&file,&rank);
  if (((file < 1)&&(file > 16))||
      ((rank < 'a')&&(rank > 'p')))
    {
      fprintf(stderr,"Not a square!\n");
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

  EZ_FreezeWidget(board);

  for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	fprintf(stderr,"%s, ", move_str(gen_dat[i].m.b));
	from = gen_dat[i].m.b.from;
	var_board[gen_dat[i].m.b.from] = level;
	x = GetXPixels(from);
	y = GetYPixels(from);
	possible_move[show] = CreateInfluenceItem(level-1,x,y);
	if ( possible_move[show] ) {
	  EZ_WorkAreaInsertItem(board, possible_move[show]);  
	  EZ_ConfigureItem(possible_move[show],NULL);
	  show++;
	}
      }
  }
  fprintf(stderr,"\n");
  
  /* look at enemy pieces that can walk there */

  side  ^= 1;
  xside ^= 1;

  gen();
  
  for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	fprintf(stderr,"%s, ", move_str(gen_dat[i].m.b));
	var_board[gen_dat[i].m.b.from] = level;
	from = gen_dat[i].m.b.from;
	var_board[gen_dat[i].m.b.from] = level;
	x = GetXPixels(from);
	y = GetYPixels(from);
	possible_move[show] = CreateInfluenceItem(level-1,x,y);
	if ( possible_move[show] ) {
	  EZ_WorkAreaInsertItem(board, possible_move[show]);  
	  EZ_ConfigureItem(possible_move[show],NULL);
	  show++;
	}
      }
  }
  fprintf(stderr,"\n");


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
	fprintf(stderr,"%s, ", half_move_str(gen_dat[i].m.b));
	piece[gen_dat[i].m.b.from] = EMPTY;
	color[gen_dat[i].m.b.from] = EMPTY;
	found_influence = TRUE;
	if ( var_board[gen_dat[i].m.b.from] == 0 ) {
	  var_board[gen_dat[i].m.b.from] = level;
	  from = gen_dat[i].m.b.from;
	  x = GetXPixels(from);
	  y = GetYPixels(from);
	  possible_move[show] = CreateInfluenceItem(level-1,x,y);
	  if ( possible_move[show] ) {
	    EZ_WorkAreaInsertItem(board, possible_move[show]);  
	    EZ_ConfigureItem(possible_move[show],NULL);
	    show++;
	  }
	}
      }
    }
    side ^=  1;
    xside ^= 1;
    color[to] = xside;

    gen(); /* regenerate */

    for (i = 0; i < gen_end[ply]; ++i) {
      if ( gen_dat[i].m.b.to == to ) {
	fprintf(stderr,"%s, ", half_move_str(gen_dat[i].m.b));
	piece[gen_dat[i].m.b.from] = EMPTY;
	color[gen_dat[i].m.b.from] = EMPTY;
	found_influence = TRUE;
	if ( var_board[gen_dat[i].m.b.from] == 0 ) {
	  var_board[gen_dat[i].m.b.from] = level;
	  from = gen_dat[i].m.b.from;
	  x = GetXPixels(from);
	  y = GetYPixels(from);
	  possible_move[show] = CreateInfluenceItem(level-1,x,y);
	  EZ_WorkAreaInsertItem(board, possible_move[show]);  
	  EZ_ConfigureItem(possible_move[show],NULL);
	  show++;
	}
      }
    }
    side ^=  1;
    xside ^= 1;
    color[to] = xside;

    level++;
    fprintf(stderr,"\n");
  } while ( found_influence );

  EZ_UnFreezeWidget(board);
  fprintf(stderr,"\n All moves.\n");



  /* restore current board */

  for ( i = 0; i < NUMSQUARES; i++) {
     piece[i] = save_piece[i];
     color[i] = save_color[i];
  }

  /* now print the variant board */

  fprintf(stderr,"\n\n   16   15   14   13   12   11   10   9     8    7    6    5    4    3    2    1\n");
  fprintf(stderr,"  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\na ");
  for (i = 0; i < NUMSQUARES; ++i) {
    if ( var_board[i] != 0 ) {
       fprintf(stderr,"|(%d) ", var_board[i]);
    }
    else {
      switch (color[i]) {
      case EMPTY:
	fprintf(stderr,"|    ");
	break;
      case LIGHT:
	fprintf(stderr,"|b%s", piece_string[piece[i]]);
	break;
      case DARK:
	fprintf(stderr,"|w%s", piece_string[piece[i]]);
	break;
      }
    }
    if ((i + 1) % RANKS == 0 && i != LASTSQUARE)
      fprintf(stderr,"| %c\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n%c ", 'a' + GetRank(i), 'b' + GetRank(i)); 
    }
  fprintf(stderr,"| p\n  +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+\n   16   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1\n\n");

  gen();
}


EZ_Item *CreatePieceItem(int side, int piece, int x, int y)
{
  EZ_Item *item;
  EZ_Bitmap *bitmap;

  char pixmapname[50];
  

  if ( piece == EMPTY ) {
    /* fprintf(stderr,"PieceItem for empty square %d,%d requested\n",x,y); */
    return NULL;
  }

  if ( side ) piece += (PIECE_TYPES + 1); /* start with one and 
					     one empty piece in the middle
					  */

  sprintf(pixmapname,"images/tenj%d.tiff",piece+1);
  strcpy(pixmapname,pixmapname);

  bitmap = EZ_CreateLabelPixmapFromImageFile(pixmapname);


  if (bitmap) {
    item = EZ_CreateItem(EZ_LABEL_ITEM,
			 EZ_X, x-5, EZ_Y, y-5,
			 EZ_LABEL_PIXMAP, bitmap,
			 EZ_PIXMAP_FILE, pixmapname, 
			 NULL);
    return(item);
  } else {
    fprintf(stderr, "Can't create item %d\n",piece);
    return(NULL);
  }
}

EZ_Item *CreateMoveItem(int level, int x, int y)
{
  GC gc1, gc2;
  XGCValues gcv;

  EZ_Item *item = EZ_CreateItem(EZ_FIG_ITEM, EZ_X, x, EZ_Y, y, NULL);
  char *influences[] = {"green","red", "black"};
  /* move, capture, suicide */
  /* we'll need s.th f. the burning as well */

  /* an level color forground */
  gcv.foreground = EZ_AllocateColorFromName(influences[level]);
  gc1 = EZ_GetGC(GCForeground, &gcv);
  EZ_FigItemAddFilledRectangle(item, gc1,-8,-8,16,16);
  
  /* a black outline */
  gcv.foreground = EZ_AllocateColorFromName("black");
  gc2 = EZ_GetGC(GCForeground, &gcv);
  EZ_FigItemAddRectangle(item, gc2,-8,-8,16,16);

  return(item);
}


EZ_Item *CreateInfluenceItem(int level, int x, int y)
{
  GC gc1, gc2;
  XGCValues gcv;
  int  i, dx;
  EZ_Item *item = EZ_CreateItem(EZ_FIG_ITEM, EZ_X, x, EZ_Y, y, NULL);
  char *influences[] = {"green","red","blue","grey"};
  if ( level > 3) level = 3;
  gcv.foreground = EZ_AllocateColorFromName(influences[level]);
  gc1 = EZ_GetGC(GCForeground, &gcv);
  EZ_FigItemAddFilledRectangle(item, gc1,-8,-8, 16, 16);

  /* a black outline */
  gcv.foreground = EZ_AllocateColorFromName("black");
  gc2 = EZ_GetGC(GCForeground, &gcv);
  EZ_FigItemAddRectangle(item, gc2,-8,-8, 16, 16);

  return(item);
}

int GetXPixels(int square) {
  return( xbigoffset + (square % FILES)*((boardwidth-2*xoffset)/FILES));
}
int GetYPixels(int square) {
  return( ybigoffset + (square / RANKS)*((boardheight-2*yoffset)/RANKS));
}

void remove_shown_moves( void ) {
  int i;
  EZ_FreezeWidget(board);
  for( i=0; i<NUMSQUARES; i++){
    if (possible_move[i] ) {
      EZ_WorkAreaDeleteItem(board,possible_move[i]);
      EZ_DestroyWidget(possible_move[i]);
      possible_move[i] = NULL;
    }
  }
  EZ_UnFreezeWidget(board);
}

void draw_move( const int from_file, const char from_rank,
			 const int to_file, const char to_rank, 
		 const int over_file, const char over_rank, BOOL prom) {
  int from_square, over_square, to_square, i,j;
  move_bytes m; /* shouldn't be more than two (w and w.o. prom)
		       but gen() is still overgenerating for 
		       area movers
		    */
  BOOL lprom = prom;
  BOOL found = FALSE;

  sec_click = FALSE;
  third_click = FALSE;
  /* over_file is -1 if not a lion move */

  if ( over_file == -1 ) {
    if (( from_file == to_file) && (from_rank == to_rank)) return;
    fprintf(stderr,"Making move %d%c-%d%c\n",from_file,from_rank,
	    to_file,to_rank);
  } else {
    fprintf(stderr,"Making move %d%c-%d%c-%d%c\n",from_file,from_rank,
	    over_file,over_rank,
	    to_file,to_rank);
  }
  /* find it */
  
  /* find the icon, just find the index */
  from_square = (from_rank - 'a')*FILES + (RANKS-from_file);
  if ( over_file != -1 ) {
    over_square = (over_rank - 'a')*FILES + (RANKS-over_file);
  }
  to_square = (to_rank - 'a')*FILES + (RANKS-to_file);
  if (! piece_item[from_square] ) {
    fprintf(stderr,"Can't find Item!\n");
    return;
  }

  if ( over_file == -1 ) { /* not a lion-type move */

    if (( promotion[piece[from_square]] == 0 )||
	! can_promote(from_square, to_square))
      lprom = FALSE; /* so you can move  pieces
			 with both mouse buttons
			 when they can't promote anyway
		     */
  /* now find the move */
    j=0;
    for (i = 0; i < gen_end[ply]; ++i) {
      if (( gen_dat[i].m.b.to == to_square )&&
	  ( gen_dat[i].m.b.from == from_square )){      
	if ((lprom &&( gen_dat[i].m.b.bits & 16 ))||
	    (!lprom && !( gen_dat[i].m.b.bits & 16 ))) {
	  found = TRUE;
	  break;
	}
      }
    }

  } else {

    if (( promotion[piece[from_square]] == 0 )||
	( ! can_promote(from_square, to_square) &&
	  ! can_promote(from_square, over_square))
	) 
      lprom = FALSE; /* so you can move  pieces
			 with both mouse buttons
			 when they can't promote anyway
		     */
    /* now find the move */
    j=0;
    for (i = 0; i < gen_end[ply]; ++i) { /* lion-type double move */
      if (( gen_dat[i].m.b.bits & 4) &&
	  ( gen_dat[i].m.b.to == to_square )&&
	  ( gen_dat[i].m.b.over == over_square )&&
	  ( gen_dat[i].m.b.from == from_square )){      
	if ((lprom &&( gen_dat[i].m.b.bits & 16 ))||
	    (!lprom && !( gen_dat[i].m.b.bits & 16 ))) {
	  found = TRUE;
	  break;
	}
      }
    }
    if (! found ) {
      j=0;
      for (i = 0; i < gen_end[ply]; ++i) { /* igui */
	if (( gen_dat[i].m.b.bits & 32) &&
	    ( gen_dat[i].m.b.to == over_square )&&
	    ( gen_dat[i].m.b.from == from_square )){      
	  if ((lprom &&( gen_dat[i].m.b.bits & 16 ))||
	      (!lprom && !( gen_dat[i].m.b.bits & 16 ))) {
	    found = TRUE;
	    break;
	  }
	}
      }
    }

  }

  if (! found) return;

  makemove(gen_dat[i].m.b);
  EZ_AppendListBoxItem(movebox,half_move_str(gen_dat[i].m.b));
  update_board();
  strcpy(before_last_move,half_move_str(gen_dat[i].m.b));
  undo_dat[undos] = hist_dat[0];
  ++undos;
  ply = 0;
  gen();
  make_computer_move();
  EZ_ConfigureWidget(player_label,
		     EZ_LABEL_STRING, 
		     (side? "Gote to move" : "Sente to move"),
		     NULL);

}

void undraw_move( void ) {
  third_click = FALSE;
  sec_click = FALSE;
  if (!undos)
    return;
  --undos;
  EZ_DeleteListBoxItem(movebox,undos);
  hist_dat[0] = undo_dat[undos];
  ply = 1;
  takeback();
  update_board();
  gen();
  EZ_ConfigureWidget(player_label,
		     EZ_LABEL_STRING, 
		     (side? "Gote to move" : "Sente to move"),
		     NULL);

}

void update_board( void ) {

  int i;
  EZ_FreezeWidget(board);

  for (i = 0; i < NUMSQUARES; ++i) {
    if ( piece_item[i] ) {
      EZ_WorkAreaDeleteItem(board,piece_item[i]);
      EZ_DestroyItem(piece_item[i]);
    }
    piece_item[i] = CreatePieceItem(color[i],piece[i],
				    GetXPixels(i),GetYPixels(i));
    if ( piece_item[i] ) {
      EZ_WorkAreaInsertItem(board, piece_item[i]);
      EZ_ConfigureItem(piece_item[i],NULL);
    }
  }
  EZ_ConfigureWidget(player_label,
		     EZ_LABEL_STRING, 
		     (side? "Gote to move" : "Sente to move"),
		     NULL);
  EZ_UnFreezeWidget(board);
}

void draw_board( void ) {
  int i;
  GC gc;
  XPoint pts[2];
  XGCValues gcv;
  EZ_Item *item = EZ_CreateItem(EZ_FIG_ITEM,NULL);

  gcv.foreground = EZ_AllocateColorFromName("black");
  gc = EZ_GetGC(GCForeground, &gcv);

  pts[0].x = xoffset; pts[1].x = boardwidth-xoffset;  
  pts[0].y = yoffset; pts[1].y = yoffset;  

  for ( i = 0; i <= FILES+1; i++) {
    if (( i == 5 )|| ( i == 11)) {
      EZ_FigItemAddFilledArc(item, gc, 
			     xoffset+ 5*squarewidth - 4, pts[0].y- 4, 
			     7, 7, 0, 360*64);
      EZ_FigItemAddFilledArc(item, gc, xoffset+ 11*squarewidth - 4,
			     pts[0].y -4, 7, 7, 0, 360*64);
    }
    EZ_FigItemAddLines(item,gc, pts, 2, CoordModeOrigin);
    pts[0].y += squareheight;
    pts[1].y = pts[0].y;
  }
  pts[0].x = xoffset; pts[1].x = xoffset;  
  pts[0].y = yoffset; pts[1].y = boardheight-2*yoffset;  
  for ( i = 0; i < RANKS+1; i++) {
    EZ_FigItemAddLines(item,gc, pts, 2, CoordModeOrigin);
    pts[0].x += squarewidth;
    pts[1].x = pts[0].x;
  }
  EZ_WorkAreaInsertItem(board,item);
  EZ_ConfigureItem(item,NULL);
}

void start_computer( void ) {
  computer_side = side;
  make_computer_move();
}

void make_computer_move( void ) {
   unsigned char s[256];
   int from, over, to, i;
   int from_file, to_file;
   int over_file, igui2;
   BOOL promote, found, found_in_book;
   char from_rank, over_rank, to_rank, igui, prom;

  if ( side != computer_side ) return;

  computer_is_thinking=TRUE;
  EZ_DisableWidget(start);
  EZ_DisableWidget(undo);
  EZ_EnableWidget(force);

  EZ_ConfigureWidget(computer_label,
		     EZ_LABEL_STRING, 
		      "Thinking...",
		     NULL);
  if ( have_book)
    found_in_book = in_book(before_last_move,last_move_buf);
  else found_in_book = FALSE;
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
#ifdef EZ
	EZ_ConfigureWidget(computer_label,
			   EZ_LABEL_STRING, 
			   "Illegal book move!",
			   NULL);
#else
	printf("Illegal move %s! Error in book!\n", last_move_buf);
	fflush(stdout);
#endif
	have_book = FALSE; /* should be already set, but ... */
      } else {
#ifdef DEBUG
	print_board(stdout);
#endif
	EZ_ConfigureWidget(computer_label,
			   EZ_LABEL_STRING, 
			   last_move_buf,
			   NULL);
	update_board();
	printf("Computer's move: %s\n", last_move_buf);
	EZ_AppendListBoxItem(movebox,last_move_buf);
      }
  } else { 
      have_book = FALSE; /* should be already set, but ... */
      /* } if (last_move ); */
      /* } else { */
    /* think about the move and make it */

    think(FALSE);
    if ( !pv[0][0].u ) {
      printf("%s loses\n", side_name[side] );
      computer_side = EMPTY;
    }
    strcpy(last_move,half_move_str(pv[0][0].b));
    makemove(pv[0][0].b);
#ifdef DEBUG
    print_board(stdout);
#endif
    EZ_ConfigureWidget(computer_label,
		       EZ_LABEL_STRING, 
		       last_move,
		       NULL);
    EZ_EnableWidget(start);
    update_board();
    printf("Computer's move: %s\n", last_move);
    EZ_AppendListBoxItem(movebox,last_move);
  }
  fflush(stdout);
  /* save the "backup" data because it will be overwritten */

  undo_dat[undos] = hist_dat[0];
  ++undos;
		    
  ply = 0;
  gen();
  EZ_EnableWidget(undo);
  EZ_EnableWidget(start);

  EZ_DisableWidget(force);

  computer_is_thinking=FALSE;
}

void display_short_help(int x, const char y){
  int idx;
  char localhelpstring[40];
  idx = (FILES - x) + (y - 'a')*RANKS;
  sprintf(localhelpstring,"%d%c: %s",x,y,piece_name[piece[idx]]);
  strcpy(shorthelpstring,localhelpstring);
  EZ_ConfigureWidget(short_help,
		     EZ_LABEL_STRING, shorthelpstring,
		     NULL);
}

void toggle_coords( void ) {
  if ( board_coords ) 
    board_coords = FALSE;
  else
    board_coords = TRUE;
  config_coords();
}

void config_coords( void ) {
  int x, y, i, j, k=0;
  EZ_FreezeWidget(board);
  if (board_coords) {
    for ( i = 0; i < FILES; i++) {
      for ( j = 0; j < RANKS; j++) {
	x += squarewidth;
	coord[k] = EZ_CreateItem(EZ_FIG_ITEM,EZ_X,x, EZ_Y,y,
				 NULL);
	k++;
	EZ_WorkAreaInsertItem(board,coord[k]);
	EZ_ConfigureItem(coord[k],EZ_LABEL_STRING,"coords",
			 NULL);

      }
      y += squareheight;
    }
  } else {
    for( i = 0; i < NUMSQUARES; i++ )
      EZ_WorkAreaDeleteItem(board,coord[i]);
  }
  EZ_UnFreezeWidget(board);
}

void new_game( void ) {
  init();
  book = book_init();
  gen();
  computer_side = EMPTY;
  max_time = 1000000;
  max_depth = 4;
  strcpy(before_last_move,"");
  EZ_ClearListBox(movebox);
  EZ_ClearListBox(pvbox);
  EZ_ConfigureWidget(computer_label,
		     EZ_LABEL_STRING, 
		      "Computer is off",
		     NULL);
}

void set_time( void ) {
  sscanf(EZ_GetEntryString(time_entry),"%d",&max_time);
  max_time *= 1000;
  max_depth = 10000;
}

void set_depth( void ) {
  sscanf(EZ_GetEntryString(time_entry),"%d",&max_depth);
  max_time = 1000000;
}

