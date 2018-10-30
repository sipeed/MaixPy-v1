/* vi: set sw=8 ts=8: */
/*
 * tiny vi.c: A small 'vi' clone
 * Copyright (C) 2000, 2001 Sterling Huxley <sterling@europa.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

static const char vi_Version[] =
	"$Id: vi.c,v 1.18 2002/04/26 08:06:31 andersen Exp $";

/*
 * To compile for standalone use:
 *	gcc -Wall -Os -s -DSTANDALONE -o vi vi.c
 *	  or
 *	gcc -Wall -Os -s -DSTANDALONE -DBB_FEATURE_VI_CRASHME -o vi vi.c		# include testing features
 *	strip vi
 */

/*
 * Things To Do:
 *	EXINIT
 *	$HOME/.exrc  and  ./.exrc
 *	add magic to search	/foo.*bar
 *	add :help command
 *	:map macros
 *	how about mode lines:   vi: set sw=8 ts=8:
 *	if mark[] values were line numbers rather than pointers
 *	   it would be easier to change the mark when add/delete lines
 *	More intelligence in refresh()
 *	":r !cmd"  and  "!cmd"  to filter text through an external command
 *	A true "undo" facility
 *	An "ex" line oriented mode- maybe using "cmdedit"
 */


// To test editor using CRASHME:
//RASHME
// To stop testing, wait until all to text[] is deleted, or
//    Ctrl-Z and kill -9 %1
// while in the editor Ctrl-T will toggle the crashme function on and off.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
//#include <regex.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#if 1
#define O_RDONLY        00000000
#define O_WRONLY        00000001
#define O_RDWR          00000002
#ifndef O_CREAT
#define O_CREAT         00000100        /* not fcntl */
#endif
#ifndef O_TRUNC
#define O_TRUNC         00001000        /* not fcntl */
#endif
#ifndef TRUE
#define TRUE			((int)1)
#define FALSE			((int)0)
#endif							/* TRUE */
#define MAX_SCR_COLS		_BUFSIZ

// Misc. non-Ascii keys that report an escape sequence
#define VI_K_UP			128	// cursor key Up
#define VI_K_DOWN		129	// cursor key Down
#define VI_K_RIGHT		130	// Cursor Key Right
#define VI_K_LEFT		131	// cursor key Left
#define VI_K_HOME		132	// Cursor Key Home
#define VI_K_END		133	// Cursor Key End
#define VI_K_INSERT		134	// Cursor Key Insert
#define VI_K_PAGEUP		135	// Cursor Key Page Up
#define VI_K_PAGEDOWN		136	// Cursor Key Page Down
#define VI_K_FUN1		137	// Function Key F1
#define VI_K_FUN2		138	// Function Key F2
#define VI_K_FUN3		139	// Function Key F3
#define VI_K_FUN4		140	// Function Key F4
#define VI_K_FUN5		141	// Function Key F5
#define VI_K_FUN6		142	// Function Key F6
#define VI_K_FUN7		143	// Function Key F7
#define VI_K_FUN8		144	// Function Key F8
#define VI_K_FUN9		145	// Function Key F9
#define VI_K_FUN10		146	// Function Key F10
#define VI_K_FUN11		147	// Function Key F11
#define VI_K_FUN12		148	// Function Key F12

static const int YANKONLY = FALSE;
static const int YANKDEL = TRUE;
static const int FORWARD = 1;	// code depends on "1"  for array index
static const int BACK = -1;	// code depends on "-1" for array index
static const int LIMITED = 0;	// how much of text[] in char_search
static const int FULL = 1;	// how much of text[] in char_search

static const int S_BEFORE_WS = 1;	// used in skip_thing() for moving "dot"
static const int S_TO_WS = 2;		// used in skip_thing() for moving "dot"
static const int S_OVER_WS = 3;		// used in skip_thing() for moving "dot"
static const int S_END_PUNCT = 4;	// used in skip_thing() for moving "dot"
static const int S_END_ALNUM = 5;	// used in skip_thing() for moving "dot"


typedef unsigned char Byte;

static int editing;		// >0 while we are editing a file
static int cmd_mode;		// 0=command  1=insert
static int file_modified;	// buffer contents changed
static int err_method;		// indicate error with beep or flash
static int fn_start;		// index of first cmd line file name
static int save_argc;		// how many file names on cmd line
static int cmdcnt;		// repetition count
//static fd_set rfds;		// use select() for small sleeps
//static struct timeval tv;	// use select() for small sleeps
static char erase_char;		// the users erase character
static int rows, columns;	// the terminal screen is this size
static int crow, ccol, offset;	// cursor is on Crow x Ccol with Horz Ofset
static char *SOs, *SOn;		// terminal standout start/normal ESC sequence
static char *bell;		// terminal bell sequence
static char *Ceol, *Ceos;	// Clear-end-of-line and Clear-end-of-screen ESC sequence
static char *CMrc;		// Cursor motion arbitrary destination ESC sequence
static char *CMup, *CMdown;	// Cursor motion up and down ESC sequence
static Byte *status_buffer;	// mesages to the user
static Byte last_input_char;	// last char read from user
static Byte last_forward_char;	// last char searched for with 'f'
static Byte *cfn;		// previous, current, and next file name
static Byte *text, *end, *textend;	// pointers to the user data in memory
static Byte *screen;		// pointer to the virtual screen buffer
static int screensize;		//            and its size
static Byte *screenbegin;	// index into text[], of top line on the screen
static Byte *dot;		// where all the action takes place
static int tabstop;
//static struct termios term_orig, term_vi;	// remember what the cooked mode was

static void edit_file(Byte *);	// edit one file
static void do_cmd(Byte);	// execute a command
static void sync_cursor(Byte *, int *, int *);	// synchronize the screen cursor to dot
static Byte *begin_line(Byte *);	// return pointer to cur line B-o-l
static Byte *end_line(Byte *);	// return pointer to cur line E-o-l
static Byte *dollar_line(Byte *);	// return pointer to just before NL
static Byte *prev_line(Byte *);	// return pointer to prev line B-o-l
static Byte *next_line(Byte *);	// return pointer to next line B-o-l
static Byte *end_screen(void);	// get pointer to last char on screen
static int count_lines(Byte *, Byte *);	// count line from start to stop
static Byte *find_line(int);	// find begining of line #li
static Byte *move_to_col(Byte *, int);	// move "p" to column l
static int isblnk(Byte);	// is the char a blank or tab
static void dot_left(void);	// move dot left- dont leave line
static void dot_right(void);	// move dot right- dont leave line
static void dot_begin(void);	// move dot to B-o-l
static void dot_end(void);	// move dot to E-o-l
static void dot_next(void);	// move dot to next line B-o-l
static void dot_prev(void);	// move dot to prev line B-o-l
static void dot_scroll(int, int);	// move the screen up or down
static void dot_skip_over_ws(void);	// move dot pat WS
static void dot_delete(void);	// delete the char at 'dot'
static Byte *bound_dot(Byte *);	// make sure  text[0] <= P < "end"
static Byte *new_screen(int, int);	// malloc virtual screen memory
static Byte *new_text(int);	// malloc memory for text[] buffer
static Byte *char_insert(Byte *, Byte);	// insert the char c at 'p'
static Byte *stupid_insert(Byte *, Byte);	// stupidly insert the char c at 'p'
static Byte find_range(Byte **, Byte **, Byte);	// return pointers for an object
static int st_test(Byte *, int, int, Byte *);	// helper for skip_thing()
static Byte *skip_thing(Byte *, int, int, int);	// skip some object
static Byte *find_pair(Byte *, Byte);	// find matching pair ()  []  {}
static Byte *text_hole_delete(Byte *, Byte *);	// at "p", delete a 'size' byte hole
static Byte *text_hole_make(Byte *, int);	// at "p", make a 'size' byte hole
static Byte *yank_delete(Byte *, Byte *, int, int);	// yank text[] into register then delete
static void show_help(void);	// display some help info
static void print_literal(Byte *, Byte *);	// copy s to buf, convert unprintable
static void rawmode(void);	// set "raw" mode on tty
static void cookmode(void);	// return to "cooked" mode on tty
static int mysleep(int);	// sleep for 'h' 1/100 seconds
static Byte readit(void);	// read (maybe cursor) key from stdin
static Byte get_one_char(void);	// read 1 char from stdin
static int file_size(Byte *);	// what is the byte size of "fn"
static int file_insert(Byte *, Byte *, int);
static int file_write(Byte *, Byte *, Byte *);
static void place_cursor(int, int, int);
static void screen_erase(void);
static void clear_to_eol(void);
static void clear_to_eos(void);
static void standout_start(void);	// send "start reverse video" sequence
static void standout_end(void);	// send "end reverse video" sequence
static void flash(int);		// flash the terminal screen
static void beep(void);		// beep the terminal
static void indicate_error(char);	// use flash or beep to indicate error
static void show_status_line(void);	// put a message on the bottom line
static void psb(char *, ...);	// Print Status Buf
static void psbs(char *, ...);	// Print Status Buf in standout mode
static void ni(Byte *);		// display messages
static void edit_status(void);	// show file status on status line
static void redraw(int);	// force a full screen refresh
static void format_line(Byte*, Byte*, int);
static void refresh(int);	// update the terminal from screen[]
						/* BB_FEATURE_VI_COLON */
static Byte *get_input_line(Byte *);	// get input line- use "status line"						/* BB_FEATURE_VI_DOT_CMD */
#define end_cmd_q()							/* BB_FEATURE_VI_CRASHME */

#endif


#include "vi_platform.c"

static Byte readbuffer[_BUFSIZ];

/****************** strdup *******************/
static char *_strdup(char *s)
{
	char* p;
	p = malloc(strlen(s) + 1);
	if(p == NULL) return NULL;
	strcpy(p, s);
	return p;
}


/****************** getopt *******************/
#if 1
#define assert(expr) ((void)0)
#define OPTERRCOLON (1)
#define OPTERRNF (2)
#define OPTERRARG (3)
char *optarg;
int optreset = 0;
int optind = 1;     //parm index
int opterr = 1;     
int optopt;

static void optinit(void)
{
	optreset = 0;
	optind = 1;     //parm index
	opterr = 1;
	return;
}

//put opt to optopt
static int
optiserr(int argc, char * const *argv, int oint, const char *optstr,
         int optchr, int err)   
{
    if(opterr)
    {
        printf("Error in argument %d, char %d: ", oint, optchr+1);
        switch(err)
        {
        case OPTERRCOLON:
            printf(": in flagsn");
            break;
        case OPTERRNF:
            printf("option not found %cn", argv[oint][optchr]);
            break;
        case OPTERRARG:
            printf("no argument for option %cn", argv[oint][optchr]);
            break;
        default:
            printf("unknownn");
            break;
        }
    }
    optopt = argv[oint][optchr];
    return('?');
}

int
getopt(int argc, char* const *argv, const char *optstr)
{
    static int optchr = 0;  //opt char oft
    static int dash = 0; /* have already seen the - */
    char *cp;

    if (optreset)
        optreset = optchr = dash = 0;
    if(optind >= argc)  //end
        return(EOF);
    if(!dash && (argv[optind][0] !=  '-'))  //end of dash
        return(EOF);
    if(!dash && (argv[optind][0] ==  '-') && !argv[optind][1])  
    {
        /*
        * use to specify stdin. Need to let pgm process this and
        * the following args
        */
        return(EOF);
    }
    if((argv[optind][0] == '-') && (argv[optind][1] == '-'))
    {
        /* -- indicates end of args */
        optind++;
        return(EOF);
    }
    if(!dash)
    {
        assert((argv[optind][0] == '-') && argv[optind][1]);
        dash = 1;
        optchr = 1;
    }
    /* Check if the guy tries to do a -: kind of flag */
    assert(dash);
    if(argv[optind][optchr] == ':')
    {
        dash = 0;
        optind++;
        return (optiserr(argc, argv, optind-1, optstr, optchr, OPTERRCOLON));
    }
    if(!(cp = strchr(optstr, argv[optind][optchr])))    //can't find opt char
    {
        int errind = optind;
        int errchr = optchr;
        if(!argv[optind][optchr+1]) //end of opt
        {
            dash = 0;
            optind++;
        }
        else
            optchr++;
        return (optiserr(argc, argv, errind, optstr, errchr, OPTERRNF));
    }
    if(cp[1] == ':')
    {
        dash = 0;
        optind++;
        if(optind == argc)
            return(optiserr(argc, argv, optind-1, optstr, optchr, OPTERRARG));
        optarg = argv[optind++];
        return(*cp);
    }
    else
    {
        if(!argv[optind][optchr+1])
        {
            dash = 0;
            optind++;
        }
        else
            optchr++;
        return(*cp);
    }
    //assert(0);
    //return(0);
}

#endif

/* Find out if the last character of a string matches the one given Don't
 * underrun the buffer if the string length is 0.  Also avoids a possible
 * space-hogging inline of strlen() per usage.
 */
char * last_char_is(const char *s, int c)
{
	char *sret;
	if (!s)
	    return NULL;
	sret  = (char *)s+strlen(s)-1;
	if (sret>=s && *sret == c) {
		return sret;
	} else {
		return NULL;
	}
}

extern int vi_main(unsigned char * fn)
{
	int c;
	CMrc= "\033[%d;%dH";	// Terminal Crusor motion ESC sequence
	CMup= "\033[A";		// move cursor up one line, same col
	CMdown="\n";		// move cursor down one line, same col
	Ceol= "\033[0K";	// Clear from cursor to end of line
	Ceos= "\033[0J";	// Clear from cursor to end of screen
	SOs = "\033[7m";	// Terminal standout mode on
	SOn = "\033[0m";	// Terminal standout mode off
	bell= "\007";		// Terminal bell sequence

	vi_init();
	optinit();
	status_buffer = (Byte *) malloc(STATUS_LEN);	// hold messages to user
	// The argv array can be used by the ":next"  and ":rewind" commands
	// save optind.
	fn_start = optind;	// remember first file name for :next and :rew
	save_argc = 1;

	//----- This is the main file handling loop --------------
	if(fn != 0)
	{
		cfn = (Byte *)_strdup(fn);

		edit_file(cfn);
	}

	//-----------------------------------------------------------
    free(status_buffer);
	return (0);
}

static void edit_file(Byte * fn)
{
	char c;
	int cnt, size, ch;

	rawmode();
	rows = ROWS;
	columns = COLUMNS;
	ch= -1;
	new_screen(rows, columns);	// get memory for virtual screen

	cnt = file_size(fn);	// file size
	size = 2 * cnt;		// 200% of file size
	new_text(size);		// get a text[] buffer
	screenbegin = dot = end = text;
	//read all to buf...
	if (fn != 0) {
		ch= file_insert(fn, text, cnt);
	}
	//insert if empty
	if (ch < 1) {
		(void) char_insert(text, '\n');	// start empty buf with dummy line
	}
	file_modified = FALSE;
	err_method = 1;		// flash
	last_forward_char = last_input_char = '\0';
	crow = 0;
	ccol = 0;
	edit_status();
	editing = 1;
	cmd_mode = 0;		// 0=command  1=insert  2='R'eplace
	cmdcnt = 0;
	tabstop = 8;
	offset = 0;			// no horizontal offset
	c = '\0';
	redraw(FALSE);			// dont force every col re-draw
	show_status_line();

	//------This is the main Vi cmd handling loop -----------------------
	while (editing > 0) {
		last_input_char = c = get_one_char();	// get a cmd from user
		do_cmd(c);		// execute the user command
		//
		// poll to see if there is input already waiting. if we are
		// not able to display output fast enough to keep up, skip
		// the display update until we catch up with input.
		if (mysleep(0) == 0) {
			// no input pending- so update output
			refresh(FALSE);
			show_status_line();
		}
	}
	//-------------------------------------------------------------------

	place_cursor(rows, 0, FALSE);	// go to bottom of screen
	clear_to_eol();		// Erase to end of line
	cookmode();
}



//---------------------------------------------------------------------
//----- the Ascii Chart -----------------------------------------------
//
//  00 nul   01 soh   02 stx   03 etx   04 eot   05 enq   06 ack   07 bel
//  08 bs    09 ht    0a nl    0b vt    0c np    0d cr    0e so    0f si
//  10 dle   11 dc1   12 dc2   13 dc3   14 dc4   15 nak   16 syn   17 etb
//  18 can   19 em    1a sub   1b esc   1c fs    1d gs    1e rs    1f us
//  20 sp    21 !     22 "     23 #     24 $     25 %     26 &     27 '
//  28 (     29 )     2a *     2b +     2c ,     2d -     2e .     2f /
//  30 0     31 1     32 2     33 3     34 4     35 5     36 6     37 7
//  38 8     39 9     3a :     3b ;     3c <     3d =     3e >     3f ?
//  40 @     41 A     42 B     43 C     44 D     45 E     46 F     47 G
//  48 H     49 I     4a J     4b K     4c L     4d M     4e N     4f O
//  50 P     51 Q     52 R     53 S     54 T     55 U     56 V     57 W
//  58 X     59 Y     5a Z     5b [     5c \     5d ]     5e ^     5f _
//  60 `     61 a     62 b     63 c     64 d     65 e     66 f     67 g
//  68 h     69 i     6a j     6b k     6c l     6d m     6e n     6f o
//  70 p     71 q     72 r     73 s     74 t     75 u     76 v     77 w
//  78 x     79 y     7a z     7b {     7c |     7d }     7e ~     7f del
//---------------------------------------------------------------------

//----- Execute a Vi Command -----------------------------------
static void do_cmd(Byte c)
{
	Byte c1, *p, *q, *msg, buf[9], *save_dot;
	int cnt, i, j, dir, yf;

	c1 = c;				// quiet the compiler
	cnt = yf = dir = 0;	// quiet the compiler
	p = q = save_dot = msg = buf;	// quiet the compiler
	memset(buf, '\0', 9);	// clear buf

	/* if this is a cursor key, skip these checks */
	switch (c) {
		case VI_K_UP:
		case VI_K_DOWN:
		case VI_K_LEFT:
		case VI_K_RIGHT:
		case VI_K_HOME:
		case VI_K_END:
		case VI_K_PAGEUP:
		case VI_K_PAGEDOWN:
			goto key_cmd_mode;
	}

	if (cmd_mode == 2) {
		// we are 'R'eplacing the current *dot with new char
		if (*dot == '\n') {
			// don't Replace past E-o-l
			cmd_mode = 1;	// convert to insert
		} else {
			if (1 <= c && c <= 127) {	// only ASCII chars
				if (c != 27)
					dot = yank_delete(dot, dot, 0, YANKDEL);	// delete char
				dot = char_insert(dot, c);	// insert new char
			}
			goto dc1;
		}
	}
	if (cmd_mode == 1) {
		//  hitting "Insert" twice means "R" replace mode
		if (c == VI_K_INSERT) goto dc5;
		// insert the char c at "dot"
		if (1 <= c && c <= 127) {
			dot = char_insert(dot, c);	// only ASCII chars
		}
		goto dc1;
	}

key_cmd_mode:
	switch (c) {
		//case 0x01:	// soh
		//case 0x09:	// ht
		//case 0x0b:	// vt
		//case 0x0e:	// so
		//case 0x0f:	// si
		//case 0x10:	// dle
		//case 0x11:	// dc1
		//case 0x13:	// dc3
		//case 0x16:	// syn
		//case 0x17:	// etb
		//case 0x18:	// can
		//case 0x1c:	// fs
		//case 0x1d:	// gs
		//case 0x1e:	// rs
		//case 0x1f:	// us
		//case '!':	// !-
		//case '#':	// #-
		//case '&':	// &-
		//case '(':	// (-
		//case ')':	// )-
		//case '*':	// *-
		//case ',':	// ,-
		//case '=':	// =-
		//case '@':	// @-
		//case 'F':	// F-
		//case 'K':	// K-
		//case 'Q':	// Q-
		//case 'S':	// S-
		//case 'T':	// T-
		//case 'V':	// V-
		//case '[':	// [-
		//case '\\':	// \-
		//case ']':	// ]-
		//case '_':	// _-
		//case '`':	// `-
		//case 'g':	// g-
		//case 'u':	// u- FIXME- there is no undo
		//case 'v':	// v-
	default:			// unrecognised command
		buf[0] = c;
		buf[1] = '\0';
		if (c <= ' ') {
			buf[0] = '^';
			buf[1] = c + '@';
			buf[2] = '\0';
		}
		ni((Byte *) buf);
		end_cmd_q();	// stop adding to q
	case 0x00:			// nul- ignore
		break;
	case 2:			// ctrl-B  scroll up   full screen
	case VI_K_PAGEUP:	// Cursor Key Page Up
		dot_scroll(rows - 2, -1);
		break;
	case 4:			// ctrl-D  scroll down half screen
		dot_scroll((rows - 2) / 2, 1);
		break;
	case 5:			// ctrl-E  scroll down one line
		dot_scroll(1, 1);
		break;

	case 6:			// ctrl-F  scroll down full screen
	case VI_K_PAGEDOWN:	// Cursor Key Page Down
		dot_scroll(rows - 2, 1);
		break;
	case 7:			// ctrl-G  show current status
		edit_status();
		break;
	case 'h':			// h- move left
	case VI_K_LEFT:	// cursor key Left
	case 8:			// ctrl-H- move left    (This may be ERASE char)
	case 127:			// DEL- move left   (This may be ERASE char)
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_left();
		break;
	case 10:			// Newline ^J
	case 'j':			// j- goto next line, same col
	case VI_K_DOWN:	// cursor key Down
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_next();		// go to next B-o-l
		dot = move_to_col(dot, ccol + offset);	// try stay in same col
		break;
	case 12:			// ctrl-L  force redraw whole screen
	case 18:			// ctrl-R  force redraw
		place_cursor(0, 0, FALSE);	// put cursor in correct place
		clear_to_eos();	// tel terminal to erase display
		(void) mysleep(10);
		screen_erase();	// erase the internal screen buffer
		refresh(TRUE);	// this will redraw the entire display
		break;
	case 13:			// Carriage Return ^M
	case '+':			// +- goto next line
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_next();
		dot_skip_over_ws();
		break;
	case 21:			// ctrl-U  scroll up   half screen
		dot_scroll((rows - 2) / 2, -1);
		break;
	case 25:			// ctrl-Y  scroll up one line
		dot_scroll(1, -1);
		break;
	case 27:			// esc
		if (cmd_mode == 0)
			indicate_error(c);
		cmd_mode = 0;	// stop insrting
		end_cmd_q();
		*status_buffer = '\0';	// clear status buffer
		break;
	case ' ':			// move right
	case 'l':			// move right
	case VI_K_RIGHT:	// Cursor Key Right
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_right();
		break;
	case '$':			// $- goto end of line
	case VI_K_END:		// Cursor Key End
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot = end_line(dot + 1);
		break;
	case '%':			// %- find matching char of pair () [] {}
		for (q = dot; q < end && *q != '\n'; q++) {
			if (strchr("()[]{}", *q) != NULL) {
				// we found half of a pair
				p = find_pair(q, *q);
				if (p == NULL) {
					indicate_error(c);
				} else {
					dot = p;
				}
				break;
			}
		}
		if (*q == '\n')
			indicate_error(c);
		break;
	case 'f':			// f- forward to a user specified char
		last_forward_char = get_one_char();	// get the search char
		//
		// dont seperate these two commands. 'f' depends on ';'
		//
		//**** fall thru to ... 'i'
	case ';':			// ;- look at rest of line for last forward char
		if (cmdcnt-- > 1) {
			do_cmd(';');
		}				// repeat cnt
		if (last_forward_char == 0) break;
		q = dot + 1;
		while (q < end - 1 && *q != '\n' && *q != last_forward_char) {
			q++;
		}
		if (*q == last_forward_char)
			dot = q;
		break;
	case '-':			// -- goto prev line
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_prev();
		dot_skip_over_ws();
		break;
	case '0':			// 0- goto begining of line
	case '1':			// 1-
	case '2':			// 2-
	case '3':			// 3-
	case '4':			// 4-
	case '5':			// 5-
	case '6':			// 6-
	case '7':			// 7-
	case '8':			// 8-
	case '9':			// 9-
		if (c == '0' && cmdcnt < 1) {
			dot_begin();	// this was a standalone zero
		} else {
			cmdcnt = cmdcnt * 10 + (c - '0');	// this 0 is part of a number
		}
		break;
	case ':':			// :- the colon mode commands
		p = get_input_line((Byte *) ":");	// get input line- use "status line"						/* BB_FEATURE_VI_COLON */
		if (*p == ':')
			p++;				// move past the ':'
		cnt = strlen((char *) p);
		if (cnt <= 0)
			break;
		if (strncasecmp((char *) p, "quit", cnt) == 0 ||
			strncasecmp((char *) p, "q!", cnt) == 0) {	// delete lines
			if (file_modified == TRUE && p[1] != '!') {
				psbs("No write since last change (:quit! overrides)");
			} else {
				editing = 0;
			}
		} else if (strncasecmp((char *) p, "write", cnt) == 0 ||
				   strncasecmp((char *) p, "wq", cnt) == 0) {
			cnt = file_write(cfn, text, end - 1);
			file_modified = FALSE;
			psb("\"%s\" %dL, %dC", cfn, count_lines(text, end - 1), cnt);
			if (p[1] == 'q') {
				editing = 0;
			}
		} else if (strncasecmp((char *) p, "file", cnt) == 0 ) {
			edit_status();			// show current file status
		} else if (sscanf((char *) p, "%d", &j) > 0) {
			dot = find_line(j);		// go to line # j
			dot_skip_over_ws();
		} else {		// unrecognised cmd
			ni((Byte *) p);
		}						/* BB_FEATURE_VI_COLON */
		break;
	case '<':			// <- Left  shift something
	case '>':			// >- Right shift something
		cnt = count_lines(text, dot);	// remember what line we are on
		c1 = get_one_char();	// get the type of thing to delete
		find_range(&p, &q, c1);
		(void) yank_delete(p, q, 1, YANKONLY);	// save copy before change
		p = begin_line(p);
		q = end_line(q);
		i = count_lines(p, q);	// # of lines we are shifting
		for ( ; i > 0; i--, p = next_line(p)) {
			if (c == '<') {
				// shift left- remove tab or 8 spaces
				if (*p == '\t') {
					// shrink buffer 1 char
					(void) text_hole_delete(p, p);
				} else if (*p == ' ') {
					// we should be calculating columns, not just SPACE
					for (j = 0; *p == ' ' && j < tabstop; j++) {
						(void) text_hole_delete(p, p);
					}
				}
			} else if (c == '>') {
				// shift right -- add tab or 8 spaces
				(void) char_insert(p, '\t');
			}
		}
		dot = find_line(cnt);	// what line were we on
		dot_skip_over_ws();
		end_cmd_q();	// stop adding to q
		break;
	case 'A':			// A- append at e-o-l
		dot_end();		// go to e-o-l
		//**** fall thru to ... 'a'
	case 'a':			// a- append after current char
		if (*dot != '\n')
			dot++;
		goto dc_i;
		//break;
	case 'B':			// B- back a blank-delimited Word
	case 'E':			// E- end of a blank-delimited word
	case 'W':			// W- forward a blank-delimited word
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dir = FORWARD;
		if (c == 'B')
			dir = BACK;
		if (c == 'W' || isspace(dot[dir])) {
			dot = skip_thing(dot, 1, dir, S_TO_WS);
			dot = skip_thing(dot, 2, dir, S_OVER_WS);
		}
//		STDIN_FILENO

		if (c != 'W')
			dot = skip_thing(dot, 1, dir, S_BEFORE_WS);
		break;
	case 'C':			// C- Change to e-o-l
	case 'D':			// D- delete to e-o-l
		save_dot = dot;
		dot = dollar_line(dot);	// move to before NL
		// copy text into a register and delete
		dot = yank_delete(save_dot, dot, 0, YANKDEL);	// delete to e-o-l
		if (c == 'C')
			goto dc_i;	// start inserting
		break;
	case 'G':		// G- goto to a line number (default= E-O-F)
		dot = end - 1;				// assume E-O-F
		if (cmdcnt > 0) {
			dot = find_line(cmdcnt);	// what line is #cmdcnt
		}
		dot_skip_over_ws();
		break;
	case 'H':			// H- goto top line on screen
		dot = screenbegin;
		if (cmdcnt > (rows - 1)) {
			cmdcnt = (rows - 1);
		}
		if (cmdcnt-- > 1) {
			do_cmd('+');
		}				// repeat cnt
		dot_skip_over_ws();
		break;
	case 'I':			// I- insert before first non-blank
		dot_begin();	// 0
		dot_skip_over_ws();
		//**** fall thru to ... 'i'
	case 'i':			// i- insert before current char
	case VI_K_INSERT:	// Cursor Key Insert
	  dc_i:
		cmd_mode = 1;	// start insrting
		psb("-- Insert --");
		break;
	case 'J':			// J- join current and next lines together
		if (cmdcnt-- > 2) {
			do_cmd(c);
		}				// repeat cnt
		dot_end();		// move to NL
		if (dot < end - 1) {	// make sure not last char in text[]
			*dot++ = ' ';	// replace NL with space
			while (isblnk(*dot)) {	// delete leading WS
				dot_delete();
			}
		}
		end_cmd_q();	// stop adding to q
		break;
	case 'L':			// L- goto bottom line on screen
		dot = end_screen();
		if (cmdcnt > (rows - 1)) {
			cmdcnt = (rows - 1);
		}
		if (cmdcnt-- > 1) {
			do_cmd('-');
		}				// repeat cnt
		dot_begin();
		dot_skip_over_ws();
		break;
	case 'M':			// M- goto middle line on screen
		dot = screenbegin;
		for (cnt = 0; cnt < (rows-1) / 2; cnt++)
			dot = next_line(dot);
		break;
	case 'O':			// O- open a empty line above
		//    0i\n ESC -i
		p = begin_line(dot);
		if (p[-1] == '\n') {
			dot_prev();
	case 'o':			// o- open a empty line below; Yes, I know it is in the middle of the "if (..."
			dot_end();
			dot = char_insert(dot, '\n');
		} else {
			dot_begin();	// 0
			dot = char_insert(dot, '\n');	// i\n ESC
			dot_prev();	// -
		}
		goto dc_i;
		//break;
	case 'R':			// R- continuous Replace char
	  dc5:
		cmd_mode = 2;
		psb("-- Replace --");
		break;
	case 'X':			// X- delete char before dot
	case 'x':			// x- delete the current char
	case 's':			// s- substitute the current char
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dir = 0;
		if (c == 'X')
			dir = -1;
		if (dot[dir] != '\n') {
			if (c == 'X')
				dot--;	// delete prev char
			dot = yank_delete(dot, dot, 0, YANKDEL);	// delete char
		}
		if (c == 's')
			goto dc_i;	// start insrting
		end_cmd_q();	// stop adding to q
		break;
	case 'Z':			// Z- if modified, {write}; exit
		// ZZ means to save file (if necessary), then exit
		c1 = get_one_char();
		if (c1 != 'Z') {
			indicate_error(c);
			break;
		}
		if (file_modified == TRUE) {
			cnt = file_write(cfn, text, end - 1);
			if (cnt == (end - 1 - text + 1)) {
				editing = 0;
			}
		} else {
			editing = 0;
		}
		break;
	case '^':			// ^- move to first non-blank on line
		dot_begin();
		dot_skip_over_ws();
		break;
	case 'b':			// b- back a word
	case 'e':			// e- end of word
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dir = FORWARD;
		if (c == 'b')
			dir = BACK;
		if ((dot + dir) < text || (dot + dir) > end - 1)
			break;
		dot += dir;
		if (isspace(*dot)) {
			dot = skip_thing(dot, (c == 'e') ? 2 : 1, dir, S_OVER_WS);
		}
		if (isalnum(*dot) || *dot == '_') {
			dot = skip_thing(dot, 1, dir, S_END_ALNUM);
		} else if (ispunct(*dot)) {
			dot = skip_thing(dot, 1, dir, S_END_PUNCT);
		}
		break;
	case 'c':			// c- change something
	case 'd':			// d- delete something
		yf = YANKDEL;	// assume either "c" or "d"
		c1 = 'y';
		if (c != 'Y')
			c1 = get_one_char();	// get the type of thing to delete
		find_range(&p, &q, c1);
		if (c1 == 27) {	// ESC- user changed mind and wants out
			c = c1 = 27;	// Escape- do nothing
		} else if (strchr("wW", c1)) {
			if (c == 'c') {
				// don't include trailing WS as part of word
				while (isblnk(*q)) {
					if (q <= text || q[-1] == '\n')
						break;
					q--;
				}
			}
			dot = yank_delete(p, q, 0, yf);	// delete word
		} else if (strchr("^0bBeEft$", c1)) {
			// single line copy text into a register and delete
			dot = yank_delete(p, q, 0, yf);	// delete word
		} else if (strchr("cdykjHL%+-{}\r\n", c1)) {
			// multiple line copy text into a register and delete
			dot = yank_delete(p, q, 1, yf);	// delete lines
			if (c == 'c') {
				dot = char_insert(dot, '\n');
				// on the last line of file don't move to prev line
				if (dot != (end-1)) {
					dot_prev();
				}
			} else if (c == 'd') {
				dot_begin();
				dot_skip_over_ws();
			}
		} else {
			// could not recognize object
			c = c1 = 27;	// error-
			indicate_error(c);
		}
		if (c1 != 27) {
			// if CHANGING, not deleting, start inserting after the delete
			if (c == 'c') {
				strcpy((char *) buf, "Change");
				goto dc_i;	// start inserting
			}
			if (c == 'd') {
				strcpy((char *) buf, "Delete");
			}
			end_cmd_q();	// stop adding to q
		}
		break;
	case 'k':			// k- goto prev line, same col
	case VI_K_UP:		// cursor key Up
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		dot_prev();
		dot = move_to_col(dot, ccol + offset);	// try stay in same col
		break;
	case 'r':			// r- replace the current char with user input
		c1 = get_one_char();	// get the replacement char
		if (*dot != '\n') {
			*dot = c1;
			file_modified = TRUE;	// has the file been modified
		}
		end_cmd_q();	// stop adding to q
		break;
	case 't':			// t- move to char prior to next x
                last_forward_char = get_one_char();
                do_cmd(';');
                if (*dot == last_forward_char)
                        dot_left();
                last_forward_char= 0;
		break;
	case 'w':			// w- forward a word
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		if (isalnum(*dot) || *dot == '_') {	// we are on ALNUM
			dot = skip_thing(dot, 1, FORWARD, S_END_ALNUM);
		} else if (ispunct(*dot)) {	// we are on PUNCT
			dot = skip_thing(dot, 1, FORWARD, S_END_PUNCT);
		}
		if (dot < end - 1)
			dot++;		// move over word
		if (isspace(*dot)) {
			dot = skip_thing(dot, 2, FORWARD, S_OVER_WS);
		}
		break;
	case 'z':			// z-
		c1 = get_one_char();	// get the replacement char
		cnt = 0;
		if (c1 == '.')
			cnt = (rows - 2) / 2;	// put dot at center
		if (c1 == '-')
			cnt = rows - 2;	// put dot at bottom
		screenbegin = begin_line(dot);	// start dot at top
		dot_scroll(cnt, -1);
		break;
	case '|':			// |- move to column "cmdcnt"
		dot = move_to_col(dot, cmdcnt - 1);	// try to move to column
		break;
	case '~':			// ~- flip the case of letters   a-z -> A-Z
		if (cmdcnt-- > 1) {
			do_cmd(c);
		}				// repeat cnt
		if (islower(*dot)) {
			*dot = toupper(*dot);
			file_modified = TRUE;	// has the file been modified
		} else if (isupper(*dot)) {
			*dot = tolower(*dot);
			file_modified = TRUE;	// has the file been modified
		}
		dot_right();
		end_cmd_q();	// stop adding to q
		break;
		//----- The Cursor and Function Keys -----------------------------
	case VI_K_HOME:	// Cursor Key Home
		dot_begin();
		break;
		// The Fn keys could point to do_macro which could translate them
	case VI_K_FUN1:	// Function Key F1
		//
	case VI_K_FUN2:	// Function Key F2
//		exit(EXIT_FAILURE);
		editing = 0;
		break;
	case VI_K_FUN3:	// Function Key F3
	case VI_K_FUN4:	// Function Key F4
	case VI_K_FUN5:	// Function Key F5
	case VI_K_FUN6:	// Function Key F6
	case VI_K_FUN7:	// Function Key F7
	case VI_K_FUN8:	// Function Key F8
	case VI_K_FUN9:	// Function Key F9
	case VI_K_FUN10:	// Function Key F10
	case VI_K_FUN11:	// Function Key F11
	case VI_K_FUN12:	// Function Key F12
		break;
	}

  dc1:
	// if text[] just became empty, add back an empty line
	if (end == text) {
		(void) char_insert(text, '\n');	// start empty buf with dummy line
		dot = text;
	}
	// it is OK for dot to exactly equal to end, otherwise check dot validity
	if (dot != end) {
		dot = bound_dot(dot);	// make sure "dot" is valid
	}

	if (!isdigit(c))
		cmdcnt = 0;		// cmd was not a number, reset cmdcnt
	cnt = dot - begin_line(dot);
	// Try to stay off of the Newline
	if (*dot == '\n' && cnt > 0 && cmd_mode == 0)
		dot--;
}


//----- Synchronize the cursor to Dot --------------------------
static void sync_cursor(Byte * d, int *row, int *col)
{
	Byte *beg_cur, *end_cur;	// begin and end of "d" line
	Byte *beg_scr, *end_scr;	// begin and end of screen
	Byte *tp;
	int cnt, ro, co;

	beg_cur = begin_line(d);	// first char of cur line
	end_cur = end_line(d);	// last char of cur line

	beg_scr = end_scr = screenbegin;	// first char of screen
	end_scr = end_screen();	// last char of screen

	if (beg_cur < screenbegin) {
		// "d" is before  top line on screen
		// how many lines do we have to move
		cnt = count_lines(beg_cur, screenbegin);
	  sc1:
		screenbegin = beg_cur;
		if (cnt > (rows - 1) / 2) {
			// we moved too many lines. put "dot" in middle of screen
			for (cnt = 0; cnt < (rows - 1) / 2; cnt++) {
				screenbegin = prev_line(screenbegin);
			}
		}
	} else if (beg_cur > end_scr) {
		// "d" is after bottom line on screen
		// how many lines do we have to move
		cnt = count_lines(end_scr, beg_cur);
		if (cnt > (rows - 1) / 2)
			goto sc1;	// too many lines
		for (ro = 0; ro < cnt - 1; ro++) {
			// move screen begin the same amount
			screenbegin = next_line(screenbegin);
			// now, move the end of screen
			end_scr = next_line(end_scr);
			end_scr = end_line(end_scr);
		}
	}
	// "d" is on screen- find out which row
	tp = screenbegin;
	for (ro = 0; ro < rows - 1; ro++) {	// drive "ro" to correct row
		if (tp == beg_cur)
			break;
		tp = next_line(tp);
	}

	// find out what col "d" is on
	co = 0;
	do {				// drive "co" to correct column
		if (*tp == '\n' || *tp == '\0')
			break;
		if (*tp == '\t') {
			//         7       - (co %    8  )
			co += ((tabstop - 1) - (co % tabstop));
		} else if (*tp < ' ') {
			co++;		// display as ^X, use 2 columns
		}
	} while (tp++ < d && ++co);

	// "co" is the column where "dot" is.
	// The screen has "columns" columns.
	// The currently displayed columns are  0+offset -- columns+ofset
	// |-------------------------------------------------------------|
	//               ^ ^                                ^
	//        offset | |------- columns ----------------|
	//
	// If "co" is already in this range then we do not have to adjust offset
	//      but, we do have to subtract the "offset" bias from "co".
	// If "co" is outside this range then we have to change "offset".
	// If the first char of a line is a tab the cursor will try to stay
	//  in column 7, but we have to set offset to 0.

	if (co < 0 + offset) {
		offset = co;
	}
	if (co >= columns + offset) {
		offset = co - columns + 1;
	}
	// if the first char of the line is a tab, and "dot" is sitting on it
	//  force offset to 0.
	if (d == beg_cur && *d == '\t') {
		offset = 0;
	}
	co -= offset;

	*row = ro;
	*col = co;
}

//----- Text Movement Routines ---------------------------------
static Byte *begin_line(Byte * p) // return pointer to first char cur line
{
	while (p > text && p[-1] != '\n')
		p--;			// go to cur line B-o-l
	return (p);
}

static Byte *end_line(Byte * p) // return pointer to NL of cur line line
{
	while (p < end - 1 && *p != '\n')
		p++;			// go to cur line E-o-l
	return (p);
}

static Byte *dollar_line(Byte * p) // return pointer to just before NL line
{
	while (p < end - 1 && *p != '\n')
		p++;			// go to cur line E-o-l
	// Try to stay off of the Newline
	if (*p == '\n' && (p - begin_line(p)) > 0)
		p--;
	return (p);
}

static Byte *prev_line(Byte * p) // return pointer first char prev line
{
	p = begin_line(p);	// goto begining of cur line
	if (p[-1] == '\n' && p > text)
		p--;			// step to prev line
	p = begin_line(p);	// goto begining of prev line
	return (p);
}

static Byte *next_line(Byte * p) // return pointer first char next line
{
	p = end_line(p);
	if (*p == '\n' && p < end - 1)
		p++;			// step to next line
	return (p);
}

//----- Text Information Routines ------------------------------
static Byte *end_screen(void)
{
	Byte *q;
	int cnt;

	// find new bottom line
	q = screenbegin;
	for (cnt = 0; cnt < rows - 2; cnt++)
		q = next_line(q);
	q = end_line(q);
	return (q);
}

static int count_lines(Byte * start, Byte * stop) // count line from start to stop
{
	Byte *q;
	int cnt;

	if (stop < start) {	// start and stop are backwards- reverse them
		q = start;
		start = stop;
		stop = q;
	}
	cnt = 0;
	stop = end_line(stop);	// get to end of this line
	for (q = start; q <= stop && q <= end - 1; q++) {
		if (*q == '\n')
			cnt++;
	}
	return (cnt);
}

static Byte *find_line(int li)	// find begining of line #li
{
	Byte *q;

	for (q = text; li > 1; li--) {
		q = next_line(q);
	}
	return (q);
}

//----- Dot Movement Routines ----------------------------------
static void dot_left(void)
{
	if (dot > text && dot[-1] != '\n')
		dot--;
}

static void dot_right(void)
{
	if (dot < end - 1 && *dot != '\n')
		dot++;
}

static void dot_begin(void)
{
	dot = begin_line(dot);	// return pointer to first char cur line
}

static void dot_end(void)
{
	dot = end_line(dot);	// return pointer to last char cur line
}

static Byte *move_to_col(Byte * p, int l)
{
	int co;

	p = begin_line(p);
	co = 0;
	do {
		if (*p == '\n' || *p == '\0')
			break;
		if (*p == '\t') {
			//         7       - (co %    8  )
			co += ((tabstop - 1) - (co % tabstop));
		} else if (*p < ' ') {
			co++;		// display as ^X, use 2 columns
		}
	} while (++co <= l && p++ < end);
	return (p);
}

static void dot_next(void)
{
	dot = next_line(dot);
}

static void dot_prev(void)
{
	dot = prev_line(dot);
}

static void dot_scroll(int cnt, int dir)
{
	Byte *q;

	for (; cnt > 0; cnt--) {
		if (dir < 0) {
			// scroll Backwards
			// ctrl-Y  scroll up one line
			screenbegin = prev_line(screenbegin);
		} else {
			// scroll Forwards
			// ctrl-E  scroll down one line
			screenbegin = next_line(screenbegin);
		}
	}
	// make sure "dot" stays on the screen so we dont scroll off
	if (dot < screenbegin)
		dot = screenbegin;
	q = end_screen();	// find new bottom line
	if (dot > q)
		dot = begin_line(q);	// is dot is below bottom line?
	dot_skip_over_ws();
}

static void dot_skip_over_ws(void)
{
	// skip WS
	while (isspace(*dot) && *dot != '\n' && dot < end - 1)
		dot++;
}

static void dot_delete(void)	// delete the char at 'dot'
{
	(void) text_hole_delete(dot, dot);
}

static Byte *bound_dot(Byte * p) // make sure  text[0] <= P < "end"
{
	if (p >= end && end > text) {
		p = end - 1;
		indicate_error('1');
	}
	if (p < text) {
		p = text;
		indicate_error('2');
	}
	return (p);
}

//----- Helper Utility Routines --------------------------------

//----------------------------------------------------------------
//----- Char Routines --------------------------------------------
/* Chars that are part of a word-
 *    0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
 * Chars that are Not part of a word (stoppers)
 *    !"#$%&'()*+,-./:;<=>?@[\]^`{|}~
 * Chars that are WhiteSpace
 *    TAB NEWLINE VT FF RETURN SPACE
 * DO NOT COUNT NEWLINE AS WHITESPACE
 */

static Byte *new_screen(int ro, int co)
{
	int li;

	if (screen != 0)
		free(screen);
	screensize = ro * co + 8;
	screen = (Byte *) malloc(screensize);
	// initialize the new screen. assume this will be a empty file.
	screen_erase();
	//   non-existant text[] lines start with a tilde (~).
	for (li = 1; li < ro - 1; li++) {
		screen[(li * co) + 0] = '~';
	}
	return (screen);
}

static Byte *new_text(int size)
{
	if (size < MIN_FILE)
		size = MIN_FILE;	// have a minimum size for new files
	if (text != 0) {
		//text -= 4;
		free(text);
	}
	text = (Byte *) malloc(size + 8);
	memset(text, '\0', size);	// clear new text[]
	//text += 4;		// leave some room for "oops"
	textend = text + size - 1;
	//textend -= 4;		// leave some root for "oops"
	return (text);
}

static Byte *char_insert(Byte * p, Byte c) // insert the char c at 'p'
{
	if (c == 22) {		// Is this an ctrl-V?
		p = stupid_insert(p, '^');	// use ^ to indicate literal next
		p--;			// backup onto ^
		refresh(FALSE);	// show the ^
		c = get_one_char();
		*p = c;
		p++;
		file_modified = TRUE;	// has the file been modified
	} else if (c == 27) {	// Is this an ESC?
		cmd_mode = 0;
		cmdcnt = 0;
		end_cmd_q();	// stop adding to q
		strcpy((char *) status_buffer, " ");	// clear the status buffer
		if ((p[-1] != '\n') && (dot>text)) {
			p--;
		}
	} else if (c == erase_char) {	// Is this a BS
		//     123456789
		if ((p[-1] != '\n') && (dot>text)) {
			p--;
			p = text_hole_delete(p, p);	// shrink buffer 1 char
		}
	} else {
		// insert a char into text[]
		Byte *sp;		// "save p"

		if (c == 13)
			c = '\n';	// translate \r to \n
		sp = p;			// remember addr of insert
		p = stupid_insert(p, c);	// insert the char
	}
	return (p);
}

static Byte *stupid_insert(Byte * p, Byte c) // stupidly insert the char c at 'p'
{
	p = text_hole_make(p, 1);
	if (p != 0) {
		*p = c;
		file_modified = TRUE;	// has the file been modified
		p++;
	}
	return (p);
}

static Byte find_range(Byte ** start, Byte ** stop, Byte c)
{
	Byte *save_dot, *p, *q;
	int cnt;

	save_dot = dot;
	p = q = dot;

	if (strchr("cdy><", c)) {
		// these cmds operate on whole lines
		p = q = begin_line(p);
		for (cnt = 1; cnt < cmdcnt; cnt++) {
			q = next_line(q);
		}
		q = end_line(q);
	} else if (strchr("^%$0bBeEft", c)) {
		// These cmds operate on char positions
		do_cmd(c);		// execute movement cmd
		q = dot;
	} else if (strchr("wW", c)) {
		do_cmd(c);		// execute movement cmd
		if (dot > text)
			dot--;		// move back off of next word
		if (dot > text && *dot == '\n')
			dot--;		// stay off NL
		q = dot;
	} else if (strchr("H-k{", c)) {
		// these operate on multi-lines backwards
		q = end_line(dot);	// find NL
		do_cmd(c);		// execute movement cmd
		dot_begin();
		p = dot;
	} else if (strchr("L+j}\r\n", c)) {
		// these operate on multi-lines forwards
		p = begin_line(dot);
		do_cmd(c);		// execute movement cmd
		dot_end();		// find NL
		q = dot;
	} else {
		c = 27;			// error- return an ESC char
		//break;
	}
	*start = p;
	*stop = q;
	if (q < p) {
		*start = q;
		*stop = p;
	}
	dot = save_dot;
	return (c);
}

static int st_test(Byte * p, int type, int dir, Byte * tested)
{
	Byte c, c0, ci;
	int test, inc;

	inc = dir;
	c = c0 = p[0];
	ci = p[inc];
	test = 0;

	if (type == S_BEFORE_WS) {
		c = ci;
		test = ((!isspace(c)) || c == '\n');
	}
	if (type == S_TO_WS) {
		c = c0;
		test = ((!isspace(c)) || c == '\n');
	}
	if (type == S_OVER_WS) {
		c = c0;
		test = ((isspace(c)));
	}
	if (type == S_END_PUNCT) {
		c = ci;
		test = ((ispunct(c)));
	}
	if (type == S_END_ALNUM) {
		c = ci;
		test = ((isalnum(c)) || c == '_');
	}
	*tested = c;
	return (test);
}

static Byte *skip_thing(Byte * p, int linecnt, int dir, int type)
{
	Byte c;

	while (st_test(p, type, dir, &c)) {
		// make sure we limit search to correct number of lines
		if (c == '\n' && --linecnt < 1)
			break;
		if (dir >= 0 && p >= end - 1)
			break;
		if (dir < 0 && p <= text)
			break;
		p += dir;		// move to next char
	}
	return (p);
}

// find matching char of pair  ()  []  {}
static Byte *find_pair(Byte * p, Byte c)
{
	Byte match, *q;
	int dir, level;

	match = ')';
	level = 1;
	dir = 1;			// assume forward
	switch (c) {
	case '(':
		match = ')';
		break;
	case '[':
		match = ']';
		break;
	case '{':
		match = '}';
		break;
	case ')':
		match = '(';
		dir = -1;
		break;
	case ']':
		match = '[';
		dir = -1;
		break;
	case '}':
		match = '{';
		dir = -1;
		break;
	}
	for (q = p + dir; text <= q && q < end; q += dir) {
		// look for match, count levels of pairs  (( ))
		if (*q == c)
			level++;	// increase pair levels
		if (*q == match)
			level--;	// reduce pair level
		if (level == 0)
			break;		// found matching pair
	}
	if (level != 0)
		q = NULL;		// indicate no match
	return (q);
}

//  open a hole in text[]
static Byte *text_hole_make(Byte * p, int size)	// at "p", make a 'size' byte hole
{
	Byte *src, *dest;
	int cnt;

	if (size <= 0)
		goto thm0;
	src = p;
	dest = p + size;
	cnt = end - src;	// the rest of buffer
	if (memmove(dest, src, cnt) != dest) {
		psbs("can't create room for new characters");
	}
	memset(p, ' ', size);	// clear new hole
	end = end + size;	// adjust the new END
	file_modified = TRUE;	// has the file been modified
  thm0:
	return (p);
}

//  close a hole in text[]
static Byte *text_hole_delete(Byte * p, Byte * q) // delete "p" thru "q", inclusive
{
	Byte *src, *dest;
	int cnt, hole_size;

	// move forwards, from beginning
	// assume p <= q
	src = q + 1;
	dest = p;
	if (q < p) {		// they are backward- swap them
		src = p + 1;
		dest = q;
	}
	hole_size = q - p + 1;
	cnt = end - src;
	if (src < text || src > end)
		goto thd0;
	if (dest < text || dest >= end)
		goto thd0;
	if (src >= end)
		goto thd_atend;	// just delete the end of the buffer
	if (memmove(dest, src, cnt) != dest) {
		psbs("can't delete the character");
	}
  thd_atend:
	end = end - hole_size;	// adjust the new END
	if (dest >= end)
		dest = end - 1;	// make sure dest in below end-1
	if (end <= text)
		dest = end = text;	// keep pointers valid
	file_modified = TRUE;	// has the file been modified
  thd0:
	return (dest);
}

// copy text into register, then delete text.
// if dist <= 0, do not include, or go past, a NewLine
//
static Byte *yank_delete(Byte * start, Byte * stop, int dist, int yf)
{
	Byte *p;

	// make sure start <= stop
	if (start > stop) {
		// they are backwards, reverse them
		p = start;
		start = stop;
		stop = p;
	}
	if (dist <= 0) {
		// we can not cross NL boundaries
		p = start;
		if (*p == '\n')
			return (p);
		// dont go past a NewLine
		for (; p + 1 <= stop; p++) {
			if (p[1] == '\n') {
				stop = p;	// "stop" just before NewLine
				break;
			}
		}
	}
	p = start;
	if (yf == YANKDEL) {
		p = text_hole_delete(start, stop);
	}					// delete lines
	return (p);
}

static void show_help(void)
{
	puts("These features are available:");
}

static void print_literal(Byte * buf, Byte * s) // copy s to buf, convert unprintable
{
	Byte c, b[2];

	b[1] = '\0';
	strcpy((char *) buf, "");	// init buf
	if (strlen((char *) s) <= 0)
		s = (Byte *) "(NULL)";
	for (; *s > '\0'; s++) {
		c = *s;
		if (*s > '~') {
			strcat((char *) buf, SOs);
			c = *s - 128;
		}
		if (*s < ' ') {
			strcat((char *) buf, "^");
			c += '@';
		}
		b[0] = c;
		strcat((char *) buf, (char *) b);
		if (*s > '~')
			strcat((char *) buf, SOn);
		if (*s == '\n') {
			strcat((char *) buf, "$");
		}
	}
}

static int isblnk(Byte c) // is the char a blank or tab
{
	return (c == ' ' || c == '\t');
}

/*
//----- Set terminal attributes --------------------------------
static void rawmode(void)
{
//	cfmakeraw()

	tcgetattr(0, &term_orig);
	term_vi = term_orig;
	term_vi.c_lflag &= (~ICANON & ~ECHO);	// leave ISIG ON- allow intr's
	term_vi.c_iflag &= (~IXON & ~ICRNL);
	term_vi.c_oflag &= (~ONLCR);
#ifndef linux
	term_vi.c_cc[VMIN] = 1;
	term_vi.c_cc[VTIME] = 0;
#endif
	erase_char = term_vi.c_cc[VERASE];
	tcsetattr(0, TCSANOW, &term_vi);
}

static void cookmode(void)
{
	tcsetattr(0, TCSANOW, &term_orig);
}*/

static int mysleep(int hund)	// sleep for 'h' 1/100 seconds
{
	// Don't hang- Wait 5/100 seconds-  1 Sec= 1000000
	//FD_ZERO(&rfds);
	//FD_SET(0, &rfds);
	//tv.tv_sec = 0;
	//tv.tv_usec = hund * 10000;
	return stdin_select(hund*10);
}

//----- IO Routines --------------------------------------------
static Byte readit(void)	// read (maybe cursor) key from stdin
{
	Byte c;
	int i, bufsiz, cnt, cmdindex;
	struct esc_cmds {
		Byte *seq;
		Byte val;
	};

	/* �������������ס������������ն���ô */
	static struct esc_cmds esccmds[] = {
		{(Byte *) "OA", (Byte) VI_K_UP},	// cursor key Up
		{(Byte *) "OB", (Byte) VI_K_DOWN},	// cursor key Down
		{(Byte *) "OC", (Byte) VI_K_RIGHT},	// Cursor Key Right
		{(Byte *) "OD", (Byte) VI_K_LEFT},	// cursor key Left
		{(Byte *) "OH", (Byte) VI_K_HOME},	// Cursor Key Home
		{(Byte *) "OF", (Byte) VI_K_END},	// Cursor Key End
		{(Byte *) "[A", (Byte) VI_K_UP},	// cursor key Up
		{(Byte *) "[B", (Byte) VI_K_DOWN},	// cursor key Down
		{(Byte *) "[C", (Byte) VI_K_RIGHT},	// Cursor Key Right
		{(Byte *) "[D", (Byte) VI_K_LEFT},	// cursor key Left
		{(Byte *) "[H", (Byte) VI_K_HOME},	// Cursor Key Home
		{(Byte *) "[F", (Byte) VI_K_END},	// Cursor Key End
		{(Byte *) "[1~", (Byte) VI_K_HOME},	// Cursor Key Home
		{(Byte *) "[2~", (Byte) VI_K_INSERT},	// Cursor Key Insert
		{(Byte *) "[4~", (Byte) VI_K_END},	// Cursor Key End
		{(Byte *) "[5~", (Byte) VI_K_PAGEUP},	// Cursor Key Page Up
		{(Byte *) "[6~", (Byte) VI_K_PAGEDOWN},	// Cursor Key Page Down
		{(Byte *) "OP", (Byte) VI_K_FUN1},	// Function Key F1
		{(Byte *) "OQ", (Byte) VI_K_FUN2},	// Function Key F2
		{(Byte *) "OR", (Byte) VI_K_FUN3},	// Function Key F3
		{(Byte *) "OS", (Byte) VI_K_FUN4},	// Function Key F4
		{(Byte *) "[15~", (Byte) VI_K_FUN5},	// Function Key F5
		{(Byte *) "[17~", (Byte) VI_K_FUN6},	// Function Key F6
		{(Byte *) "[18~", (Byte) VI_K_FUN7},	// Function Key F7
		{(Byte *) "[19~", (Byte) VI_K_FUN8},	// Function Key F8
		{(Byte *) "[20~", (Byte) VI_K_FUN9},	// Function Key F9
		{(Byte *) "[21~", (Byte) VI_K_FUN10},	// Function Key F10
		{(Byte *) "[23~", (Byte) VI_K_FUN11},	// Function Key F11
		{(Byte *) "[24~", (Byte) VI_K_FUN12},	// Function Key F12
		{(Byte *) "[11~", (Byte) VI_K_FUN1},	// Function Key F1
		{(Byte *) "[12~", (Byte) VI_K_FUN2},	// Function Key F2
		{(Byte *) "[13~", (Byte) VI_K_FUN3},	// Function Key F3
		{(Byte *) "[14~", (Byte) VI_K_FUN4},	// Function Key F4
	};

#define ESCCMDS_COUNT (sizeof(esccmds)/sizeof(struct esc_cmds))

	//(void) alarm(0);	// turn alarm OFF while we wait for input
	// get input from User- are there already input chars in Q?
	bufsiz = strlen((char *) readbuffer);
	if (bufsiz <= 0) {
		// the Q is empty, wait for a typed char
		//while(stdin_available()<=0);
		bufsiz = _read(0, readbuffer, _BUFSIZ - 1);
		if (bufsiz < 0) {	//error
			editing = 0;
			errno = 0;
			bufsiz = 0;
		}
		readbuffer[bufsiz] = '\0';
	}
	// return char if it is not part of ESC sequence
	if (readbuffer[0] != 27)
		goto ri1;

	// This is an ESC char. Is this Esc sequence?
	// Could be bare Esc key. See if there are any
	// more chars to read after the ESC. This would
	// be a Function or Cursor Key sequence.
	//FD_ZERO(&rfds);
	//FD_SET(0, &rfds);
	//tv.tv_sec = 0;
	//tv.tv_usec = 50000;	// Wait 5/100 seconds- 1 Sec=1000000

	// keep reading while there are input chars and room in buffer
	//while (select(1, &rfds, NULL, NULL, &tv) > 0 && bufsiz <= (_BUFSIZ - 5)) {
    while (stdin_select(50)>0 && bufsiz <= (_BUFSIZ - 5)) {   //wait for 50ms
				// read the rest of the ESC string
				i = _read(0, (void *) (readbuffer + bufsiz), _BUFSIZ - bufsiz);
				if (i > 0) {
					bufsiz += i;
					readbuffer[bufsiz] = '\0';	// Terminate the string
				}
	}
	// Maybe cursor or function key?
	for (cmdindex = 0; cmdindex < ESCCMDS_COUNT; cmdindex++) {
		cnt = strlen((char *) esccmds[cmdindex].seq);
		i = strncmp((char *) esccmds[cmdindex].seq, (char *) readbuffer, cnt);
		if (i == 0) {
			// is a Cursor key- put derived value back into Q
			readbuffer[0] = esccmds[cmdindex].val;
			// squeeze out the ESC sequence
			for (i = 1; i < cnt; i++) {
				memmove(readbuffer + 1, readbuffer + 2, _BUFSIZ - 2);
				readbuffer[_BUFSIZ - 1] = '\0';
			}
			break;
		}
	}
  ri1:
	c = readbuffer[0];
	// remove one char from Q
	memmove(readbuffer, readbuffer + 1, _BUFSIZ - 1);
	readbuffer[_BUFSIZ - 1] = '\0';
	//(void) alarm(3);	// we are done waiting for input, turn alarm ON//soft watch dog?
	return (c);
}

//----- IO Routines --------------------------------------------
static Byte get_one_char()
{
	static Byte c;
	c = readit();		// get the users input
	return (c);			// return the char, where ever it came from
}

static Byte *get_input_line(Byte * prompt) // get input line- use "status line"
{
	Byte buf[_BUFSIZ];
	Byte c;
	int i;
	static Byte *obufp = NULL;

	strcpy((char *) buf, (char *) prompt);
	*status_buffer = '\0';	// clear the status buffer
	place_cursor(rows - 1, 0, FALSE);	// go to Status line, bottom of screen
	clear_to_eol();		// clear the line
	_write(1, prompt, strlen((char *) prompt));	// write out the :, /, or ? prompt

	for (i = strlen((char *) buf); i < _BUFSIZ;) {//todo 0
		c = get_one_char();	// read user input
		if (c == '\n' || c == '\r' || c == 27)
			break;		// is this end of input
		if (c == erase_char) {	// user wants to erase prev char
			i--;		// backup to prev char
			buf[i] = '\0';	// erase the char
			buf[i + 1] = '\0';	// null terminate buffer
			_write(1, " ", 3);	// erase char on screen
			if (i <= 0) {	// user backs up before b-o-l, exit
				break;
			}
		} else {
			buf[i] = c;	// save char in buffer
			buf[i + 1] = '\0';	// make sure buffer is null terminated
			_write(1, buf + i, 1);	// echo the char back to user
			i++;
		}
	}
	refresh(FALSE);
	if (obufp != NULL)
		free(obufp);
	obufp = (Byte *) _strdup((char *) buf);
	return (obufp);
}
/*
static int file_size(Byte * fn) // what is the byte size of "fn"
{
	struct stat st_buf;
	int cnt, sr;

	if (fn == 0 || strlen(fn) <= 0)
		return (-1);
	cnt = -1;
	sr = stat((char *) fn, &st_buf);	// see if file exists
	if (sr >= 0) {
		cnt = (int) st_buf.st_size;
	}
	return (cnt);
}*/

static int file_insert(Byte * fn, Byte * p, int size)
{
	int fd, cnt;

	cnt = -1;
	if (fn == 0 || strlen((char*) fn) <= 0) {
		psbs("No filename given");
		goto fi0;
	}
	if (size == 0) {
		// OK- this is just a no-op
		cnt = 0;
		goto fi0;
	}
	if (size < 0) {
		psbs("Trying to insert a negative number (%d) of characters", size);
		goto fi0;
	}
	if (p < text || p > end) {
		psbs("Trying to insert file outside of memory");
		goto fi0;
	}

	// see if we can open the file
	fd = _open((char *) fn, O_RDWR);			// assume read & write
	if (fd < 0) {
		// could not open for writing- maybe file is read only
		fd = _open((char *) fn, O_RDONLY);	// try read-only
		if (fd < 0) {
			psbs("\"%s\" %s", fn, "could not open file");
			goto fi0;
		}
	}
	p = text_hole_make(p, size);
	cnt = _read(fd, p, size);
	_close(fd);
	if (cnt < 0) {
		cnt = -1;
		p = text_hole_delete(p, p + size - 1);	// un-do buffer insert
		psbs("could not read file \"%s\"", fn);
	} else if (cnt < size) {
		// There was a partial read, shrink unused space text[]
		p = text_hole_delete(p + cnt, p + (size - cnt) - 1);	// un-do buffer insert
		psbs("could not read all of file \"%s\"", fn);
	}
	if (cnt >= size)
		file_modified = TRUE;
  fi0:
	return (cnt);
}

static int file_write(Byte * fn, Byte * first, Byte * last)
{
	int fd, cnt, charcnt;

	if (fn == 0) {
		psbs("No current filename");
		return (-1);
	}
	charcnt = 0;
	// FIXIT- use the correct umask()
	fd = _open((char *) fn, (O_WRONLY | O_CREAT | O_TRUNC));
	if (fd < 0)
		return (-1);
	cnt = last - first + 1;
	charcnt = _write(fd, first, cnt);
	if (charcnt == cnt) {
		// good write
		//file_modified= FALSE; // the file has not been modified
	} else {
		charcnt = 0;
	}
	_close(fd);
	return (charcnt);
}

//----- Terminal Drawing ---------------------------------------
// The terminal is made up of 'rows' line of 'columns' columns.
// classicly this would be 24 x 80.
//  screen coordinates
//  0,0     ...     0,79
//  1,0     ...     1,79
//  .       ...     .
//  .       ...     .
//  22,0    ...     22,79
//  23,0    ...     23,79   status line
//

//----- Move the cursor to row x col (count from 0, not 1) -------
static void place_cursor(int row, int col, int opti)
{
	char cm1[_BUFSIZ];
	char *cm;
	int l;

	memset(cm1, '\0', _BUFSIZ - 1);  // clear the buffer

	//λ�õı߽紦��
	if (row < 0) row = 0;
	if (row >= rows) row = rows - 1;
	if (col < 0) col = 0;
	if (col >= columns) col = columns - 1;

	//----- 1.  Try the standard terminal ESC sequence
	sprintf((char *) cm1, CMrc, row + 1, col + 1);
	cm= cm1;
	if (opti == FALSE) goto pc0;
  pc0:
	l= strlen(cm);
	//stdout
	if (l) _write(1, (Byte*)cm, l);			// move the cursor
}

//----- Erase from cursor to end of line -----------------------
static void clear_to_eol()
{
	_write(1, (Byte*)Ceol, strlen(Ceol));	// Erase from cursor to end of line
}

//----- Erase from cursor to end of screen -----------------------
static void clear_to_eos()
{
	_write(1, (Byte*)Ceos, strlen(Ceos));	// Erase from cursor to end of screen
}

//----- Start standout mode ------------------------------------
static void standout_start() // send "start reverse video" sequence
{
	_write(1, (Byte*)SOs, strlen(SOs));	// Start reverse video mode
}

//----- End standout mode --------------------------------------
static void standout_end() // send "end reverse video" sequence
{
	_write(1, (Byte*)SOn, strlen(SOn));	// End reverse video mode
}

//----- Flash the screen  --------------------------------------
static void flash(int h)
{
	standout_start();	// send "start reverse video" sequence
	redraw(TRUE);
	(void) mysleep(h);
	standout_end();		// send "end reverse video" sequence
	redraw(TRUE);
}

static void beep()
{
	_write(1, (Byte*)bell, strlen(bell));	// send out a bell character
}

static void indicate_error(char c)
{
	if (err_method == 0) {
		beep();
	} else {
		flash(10);
	}
}

//----- Screen[] Routines --------------------------------------
//----- Erase the Screen[] memory ------------------------------
static void screen_erase()
{
	memset(screen, ' ', screensize);	// clear new screen
}

//----- Draw the status line at bottom of the screen -------------
static void show_status_line(void)
{
	static int last_cksum;
	int l, cnt, cksum;

	cnt = strlen((char *) status_buffer);

	for (cksum= l= 0; l < cnt; l++) { cksum += (int)(status_buffer[l]); }
	// don't write the status line unless it changes
	if (cnt > 0 && last_cksum != cksum) {
		last_cksum= cksum;		// remember if we have seen this line
		place_cursor(rows - 1, 0, FALSE);	// put cursor on status line
		_write(1, status_buffer, cnt);
		clear_to_eol();
		place_cursor(crow, ccol, FALSE);	// put cursor back in correct place
	}
}

//----- format the status buffer, the bottom line of screen ------
// print status buffer, with STANDOUT mode
static void psbs(char *format, ...)
{
	va_list args;

	va_start(args, format);
	strcpy((char *) status_buffer, SOs);	// Terminal standout mode on
	vsprintf((char *) status_buffer + strlen((char *) status_buffer), format,
			 args);
	strcat((char *) status_buffer, SOn);	// Terminal standout mode off
	va_end(args);

	return;
}

// print status buffer
static void psb(char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsprintf((char *) status_buffer, format, args);
	va_end(args);
	return;
}

static void ni(Byte * s) // display messages
{
	Byte buf[_BUFSIZ];

	print_literal(buf, s);
	psbs("\'%s\' is not implemented", buf);
}

static void edit_status(void)	// show file status on status line
{
	int cur, tot, percent;

	cur = count_lines(text, dot);
	tot = count_lines(text, end - 1);
	//    current line         percent
	//   -------------    ~~ ----------
	//    total lines            100
	if (tot > 0) {
		percent = (100 * cur) / tot;
	} else {
		cur = tot = 0;
		percent = 100;
	}
	psb("\"%s\""
		"%s line %d of %d --%d%%--",
		(cfn != 0 ? (char *) cfn : "No file"),
		(file_modified == TRUE ? " [modified]" : ""),
		cur, tot, percent);
}

//----- Force refresh of all Lines -----------------------------
static void redraw(int full_screen)
{
	place_cursor(0, 0, FALSE);	// put cursor in correct place
	clear_to_eos();		// tel terminal to erase display
	screen_erase();		// erase the internal screen buffer
	refresh(full_screen);	// this will redraw the entire display
}

//----- Format a text[] line into a buffer ---------------------
static void format_line(Byte *dest, Byte *src, int li)
{
	int co;
	Byte c;

	for (co= 0; co < MAX_SCR_COLS; co++) {
		c= ' ';		// assume blank
		if (li > 0 && co == 0) {
			c = '~';        // not first line, assume Tilde
		}
		// are there chars in text[] and have we gone past the end
		if (text < end && src < end) {
			c = *src++;
		}
		if (c == '\n')
			break;
		if (c < ' ' || c > '~') {
			if (c == '\t') {
				c = ' ';
				//       co %    8     !=     7
				for (; (co % tabstop) != (tabstop - 1); co++) {
					dest[co] = c;
				}
			} else {
				dest[co++] = '^';
				c |= '@';       // make it visible
				c &= 0x7f;      // get rid of hi bit
			}
		}
		// the co++ is done here so that the column will
		// not be overwritten when we blank-out the rest of line
		dest[co] = c;
		if (src >= end)
			break;
	}
}

//----- Refresh the changed screen lines -----------------------
// Copy the source line from text[] into the buffer and note
// if the current screenline is different from the new buffer.
// If they differ then that line needs redrawing on the terminal.
//
static void refresh(int full_screen)
{
	static int old_offset;
	int li, changed;
	Byte buf[MAX_SCR_COLS];
	Byte *tp, *sp;		// pointer into text[] and screen[]

	sync_cursor(dot, &crow, &ccol);	// where cursor will be (on "dot")
	tp = screenbegin;	// index into text[] of top line

	// compare text[] to screen[] and mark screen[] lines that need updating
	for (li = 0; li < rows - 1; li++) {
		int cs, ce;				// column start & end
		memset(buf, ' ', MAX_SCR_COLS);		// blank-out the buffer
		buf[MAX_SCR_COLS-1] = 0;		// NULL terminate the buffer
		// format current text line into buf
		format_line(buf, tp, li);

		// skip to the end of the current text[] line
		while (tp < end && *tp++ != '\n') /*no-op*/ ;

		// see if there are any changes between vitual screen and buf
		changed = FALSE;	// assume no change
		cs= 0;
		ce= columns-1;
		sp = &screen[li * columns];	// start of screen line
		if (full_screen == TRUE) {
			// force re-draw of every single column from 0 - columns-1
			goto re0;
		}
		// compare newly formatted buffer with virtual screen
		// look forward for first difference between buf and screen
		for ( ; cs <= ce; cs++) {
			if (buf[cs + offset] != sp[cs]) {
				changed = TRUE;	// mark for redraw
				break;
			}
		}

		// look backward for last difference between buf and screen
		for ( ; ce >= cs; ce--) {
			if (buf[ce + offset] != sp[ce]) {
				changed = TRUE;	// mark for redraw
				break;
			}
		}
		// now, cs is index of first diff, and ce is index of last diff

		// if horz offset has changed, force a redraw
		if (offset != old_offset) {
  re0:
			changed = TRUE;
		}

		// make a sanity check of columns indexes
		if (cs < 0) cs= 0;
		if (ce > columns-1) ce= columns-1;
		if (cs > ce) {  cs= 0;  ce= columns-1;  }
		// is there a change between vitual screen and buf
		if (changed == TRUE) {
			//  copy changed part of buffer to virtual screen
			memmove(sp+cs, buf+(cs+offset), ce-cs+1);

			// move cursor to column of first change
			if (offset != old_offset) {
				// opti_cur_move is still too stupid
				// to handle offsets correctly
				place_cursor(li, cs, FALSE);
			} else {
				place_cursor(li, cs, FALSE);	// use standard ESC sequence
			}

			// write line out to terminal
			_write(1, sp+cs, ce-cs+1);
		}
	}


	place_cursor(crow, ccol, FALSE);

	if (offset != old_offset)
		old_offset = offset;
}
