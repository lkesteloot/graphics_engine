
/*
 * Include file for display.c
 *
 * $Id: display.h,v 1.4 1995/04/17 01:57:20 kesteloo Exp $
 */

void getinput (char *buf, WINDOW *win, int y, int x, char *prompt);
void leftstatus (WINDOW *win, int y, char *format, ...);
void rightstatus (WINDOW *win, int y, char *format, ...);
void centerstatus (WINDOW *win, int y, char *format, ...);
void wholestatus (WINDOW *win, int y, char *format, ...);
