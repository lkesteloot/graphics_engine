
/*
 * Support for display (curses) in the simulator.
 *
 * $Id: display.c,v 1.5 1995/04/22 19:23:57 kesteloo Exp $
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <curses.h>
#undef	reg
#include "sim.h"
#include "display.h"

void getinput (char *buf, WINDOW *win, int y, int x, char *prompt) {
    int		len, c;

    mvwprintw (win, y, x, prompt);
    wclrtoeol (win);

    x += strlen (prompt);

    len = 0;

    while (1) {
	wmove (win, y, x);
	wrefresh (win);
	c = getchar ();
	switch (c) {
	    case '\b':			/* Backspace			*/
	    case 127:			/* Delete			*/
		if (len > 0) {
		    len--;
		    x--;
		    wmove (win, y, x);
		    waddch (win, ' ');
		}
		break;
	    case '\r':			/* Return			*/
	    case '\n':			/* Line feed			*/
		buf[len] = '\0';
		return;
	    default:
		wmove (win, y, x);
		waddch (win, c);
		buf[len] = c;
		len++;
		x++;
		break;
	}
    }
}

void leftstatus (WINDOW *win, int y, char *format, ...) {
    int		x;
    va_list	argptr;
    char	buf[128];

    va_start (argptr, format);
    vsprintf (buf, format, argptr);
    va_end (argptr);

    wstandout (win);
    mvwprintw (win, y, 0, buf);
    for (x = strlen (buf); x < COLS / 2; x++) {
	waddch (win, ' ');
    }
    wstandend (win);
}

void rightstatus (WINDOW *win, int y, char *format, ...) {
    int		x;
    va_list	argptr;
    char	buf[128];

    va_start (argptr, format);
    vsprintf (buf, format, argptr);
    va_end (argptr);

    wstandout (win);
    mvwprintw (win, y, COLS / 2, buf);
    for (x = strlen (buf); x < COLS / 2; x++) {
	waddch (win, ' ');
    }
    wstandend (win);
}

void centerstatus (WINDOW *win, int y, char *format, ...) {
    int		x, i;
    va_list	argptr;
    char	buf[128];

    va_start (argptr, format);
    vsprintf (buf, format, argptr);
    va_end (argptr);

    x = (COLS - strlen (buf)) / 2;

    wstandout (win);
    wmove (win, y, 0);
    for (i = 0; i < x; i++) {
	waddch (win, ' ');
    }
    wprintw (win, buf);
    for (x += strlen (buf); x < COLS; x++) {
	waddch (win, ' ');
    }
    wstandend (win);
}

void wholestatus (WINDOW *win, int y, char *format, ...) {
    int		x;
    va_list	argptr;
    char	buf[128];

    va_start (argptr, format);
    vsprintf (buf, format, argptr);
    va_end (argptr);

    wstandout (win);
    mvwprintw (win, y, 0, buf);
    for (x = strlen (buf); x < COLS; x++) {
	waddch (win, ' ');
    }
    wstandend (win);
}
