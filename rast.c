
#undef IRIS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifdef IRIS
#include <gl/gl.h>
#include <gl/device.h>
#endif

#define X 0
#define Y 1

#define TREESIZE	16

typedef long fixed;
typedef fixed Point[2];
typedef fixed Color[3];

#define SHIFT	(1<<16)

#define CY(c)	(cycle += (c))

struct tree {
    int		I, O, Z, col;
} tree[TREESIZE];

Point p1 = {10, 10};
Point p2 = {200, 400};
Point p3 = {500, 100};
#ifdef IRIS
Color red = {255, 0, 0};
Color green = {0, 255, 0};
Color blue = {0, 0, 255};
#else
/* Color red = {31, 0, 0}; */
/* Color green = {0, 63, 0}; */
/* Color blue = {0, 0, 31}; */
Color red = {16, 32, 16};
Color green = {31, 62, 31};
Color blue = {1, 2, 1};
#endif

unsigned long	screen[480][640];
long		sx, sy, cycle, pixels, usetree, drawall;

void printinfo (int pos, float det, long A, long B, long C)
{
    char	s[128];

    sprintf (s, "Det = %g (%d), A = %08lx, B = %08lx, C = %08lx",
	det, (int)(1/det * (1<<31)), A, B, C);

#if IRIS
    cmov2i (10, pos * 12);
    cpack (0x00ffffff);
    charstr (s);
#else
    printf ("%s\n", s);
#endif
}

unsigned long mult32x32 (unsigned long A, unsigned long B)
{
    unsigned long	A1, A2, B1, B2, t1, t2, t3, t4, res;
    int			sign;

    sign = (A >> 31) ^ (B >> 31);

    if ((long)A < 0) {
	A = -A;
    }
    if ((long)B < 0) {
	B = -B;
    }

    A1 = A >> 16;
    A2 = A & 0xFFFF;
    B1 = B >> 16;
    B2 = B & 0xFFFF;

    t1 = B2 * A2;
    t2 = B2 * A1;
    t3 = B1 * A2;
    t4 = B1 * A1;

    res = (t1 >> 16) + t2 + t3 + (t4 << 16);

    if (sign) {
	return -res;
    } else {
	return res;
    }
}

void treeclr (void)
{
    int		i;
    struct tree	*t;

    t = tree;
    for (i = 0; i < TREESIZE; i++) {
	t->I = 1;
	t->O = 0;
	t->Z = 0;
	t->col = 0;
	t++;
    }
    CY (1);
    CY (3);  /* To start the tree */
}

void treein (int x, int y, fixed *c, fixed A)
{
    int		i;
    fixed	tmp;

    for (i = 0; i < TREESIZE; i++) {
	tmp = *c + i * A;
	tmp = (tmp >> 31) & 1;
	tree[i].I &= tmp;
	tree[i].O |= tmp;
    }
    *c += A * 16;
    CY (1);
}

void treeval (int x, int y, fixed *c, fixed A, int n)
{
    int		i;
    fixed	tmp;
    struct tree	*t;

    tmp = *c;
    t = tree;
    switch (n) {
	case 0:		/* Z */
	    for (i = 0; i < TREESIZE; i++) {
		t->Z = tmp;
		/* Check Z here and set tree[i].I = 0 if too far */
		/* otherwise write Z */
		/* (Obey Z read/write modes) */
		tmp += A;
		t++;
	    }
	    break;
	case 1:		/* R */
	    for (i = 0; i < TREESIZE; i++) {
		t->col |= (tmp >> 16) & 0xFF;
		tmp += A;
		t++;
	    }
	    break;
	case 2:		/* G */
	    for (i = 0; i < TREESIZE; i++) {
		t->col |= (tmp >> 8) & 0xFF00;
		tmp += A;
		t++;
	    }
	    break;
	case 3:		/* B */
	    for (i = 0; i < TREESIZE; i++) {
		t->col |= tmp & 0xFF0000;
		if (t->I || !t->O || drawall) {
		    screen[y][x + i] = t->col;
		    pixels++;
		}
		tmp += A;
		t++;
	    }
	    break;
    }
    *c = tmp;
    CY (1);
}

void rasterize(Point P0, Color C0, Point P1, Color C1, Point P2, Color C2)
{
    int x, y, i;
    int min_x, min_y, max_x, max_y;
    fixed A[3], B[3], C[3], left[3], old_left[3];
    fixed AA[3], BB[3], CC[3];
    Color color, old_color;
    int det;
    int detinv;

    pixels = 0;
    cycle = 0;

    /* Compute bounding rectangle */
    if (CY(1), P0[X] < P1[X]) {
	if (CY(1), P0[X] < P2[X]) {
	    min_x = P0[X]; CY(1);
	    max_x = (P1[X] < P2[X]) ? P2[X] : P1[X]; CY(4);
	} else {
	    min_x = P2[X]; CY(1);
	    max_x = P1[X]; CY(1);
	}
    } else if (CY(1), P1[X] < P2[X]) {
	min_x = P1[X]; CY(1);
        max_x = (P0[X] < P2[X]) ? P2[X] : P0[X]; CY(4);
    } else {
	min_x = P2[X]; CY(1);
	max_x = P0[X]; CY(1);
    }

    if (CY(1),P0[Y] < P1[Y]) {
	if (CY(1),P0[Y] < P2[Y]) {
	    min_y = P0[Y]; CY(1);
	    max_y = (P1[Y] < P2[Y]) ? P2[Y] : P1[Y]; CY(4);
	} else {
	    min_y = P2[Y]; CY(1);
	    max_y = P1[Y]; CY(1);
	}
    } else if (CY(1),P1[Y] < P2[Y]) {
	min_y = P1[Y]; CY(1);
        max_y = (P0[Y] < P2[Y]) ? P2[Y] : P0[Y]; CY(4);
    } else {
	min_y = P2[Y]; CY(1);
	max_y = P0[Y]; CY(1);
    }

    /*
     * Clip to screen
     */

    if (CY(1),min_x < 0) {
	min_x = 0; CY(1);
    }
    if (CY(1),max_x > sx - 1) {
	max_x = sx - 1; CY(1);
    }
    if (CY(1),min_y < 0) {
	min_y = 0; CY(1);
    }
    if (CY(1),max_y > sy - 1) {
	max_y = sy - 1; CY(1);
    }

    if (usetree) {
	min_x &= ~15;
    }

    /* Set up plane equations */
    A[0] = P0[Y] - P1[Y]; CY(1);
    B[0] = P1[X] - P0[X]; CY(1);
    C[0] = P1[Y] * P0[X] - P0[Y] * P1[X]; CY(3);
    left[0] = A[0] * min_x + B[0] * min_y + C[0]; CY(3);

    A[1] = P1[Y] - P2[Y]; CY(1);
    B[1] = P2[X] - P1[X]; CY(1);
    C[1] = P2[Y] * P1[X] - P1[Y] * P2[X]; CY(3);
    left[1] = A[1] * min_x + B[1] * min_y + C[1]; CY(3);

    A[2] = P2[Y] - P0[Y]; CY(1);
    B[2] = P0[X] - P2[X]; CY(1);
    C[2] = P0[Y] * P2[X] - P2[Y] * P0[X]; CY(3);
    left[2] = A[2] * min_x + B[2] * min_y + C[2]; CY(3);

#ifndef IRIS
    for (i = 0; i < 3; i++) {
	printf ("A[%d] = %ld, B[%d] = %ld, C[%d] = %ld\n",
	    i, A[i], i, B[i], i, C[i]);
    }
#endif

    det = P1[X]*P2[Y] - P2[X]*P1[Y] - P0[X]*P2[Y] + P0[Y]*P2[X] + 
	    P0[X]*P1[Y] - P0[Y]*P1[X]; CY(10);
    detinv = 1.0/det * (1<<16) * (1<<16); CY(30); /* ? */

    for (i = 0; i < 3; i++) {
	AA[i] = C1[i]*P2[Y] - C2[i]*P1[Y] - C0[i]*P2[Y] + P0[Y]*C2[i] + 
		C0[i]*P1[Y] - P0[Y]*C1[i]; CY(10);
	AA[i] = mult32x32 (AA[i], detinv); CY(17);

	BB[i] = P1[X]*C2[i] - P2[X]*C1[i] - P0[X]*C2[i] + C0[i]*P2[X] + 
		P0[X]*C1[i] - C0[i]*P1[X]; CY(10);
	BB[i] = mult32x32 (BB[i], detinv); CY(17);

	CC[i] = (P1[X]*P2[Y] - P2[X]*P1[Y])*C0[i] +
		(-P0[X]*P2[Y] + P0[Y]*P2[X])*C1[i] +
		(P0[X]*P1[Y] - P0[Y]*P1[X])*C2[i]; CY(14);
	CC[i] = mult32x32 (CC[i], detinv); CY(17);

	color[i] = AA[i] * min_x + BB[i] * min_y + CC[i]; CY(4);
    }

    printf ("%ld cycles setup, ", cycle);

    if (usetree) {
	for (y = min_y; y <= max_y; y++) {
	    old_left[0] = left[0]; old_left[1] = left[1]; old_left[2] = left[2];
	    old_color[0] = color[0]; 
	    old_color[1] = color[1];
	    old_color[2] = color[2]; 
	    CY(1);
	    for (x = min_x; x <= max_x; x += 16) {
		treeclr ();
		treein (x, y, &left[0], A[0]);
		treein (x, y, &left[1], A[1]);
		treein (x, y, &left[2], A[2]);
		treeval (x, y, &color[0], AA[0], 1);
		treeval (x, y, &color[1], AA[1], 2);
		treeval (x, y, &color[2], AA[2], 3);
#if 0
		left[0] += A[0] * 16;
		left[1] += A[1] * 16;
		left[2] += A[2] * 16;
		color[0] += AA[0] * 16;
		color[1] += AA[1] * 16;
		color[2] += AA[2] * 16;
		CY(6);
#endif
	    }
	    left[0] = old_left[0] + B[0];
	    left[1] = old_left[1] + B[1];
	    left[2] = old_left[2] + B[2];
	    color[0] = old_color[0] + BB[0];
	    color[1] = old_color[1] + BB[1];
	    color[2] = old_color[2] + BB[2];
	    CY(1);
	}
    } else {
	for (y = min_y; y <= max_y; y++) {
	    old_left[0] = left[0]; old_left[1] = left[1]; old_left[2] = left[2];
	    CY(3);
	    old_color[0] = color[0]; 
	    old_color[1] = color[1];
	    old_color[2] = color[2]; 
	    CY(3);
	    for (x = min_x; x <= max_x; x++) {
		if (CY(2), left[0] < 0 && left[1] < 0 && left[2] < 0) {
		    break;
		}
		left[0] += A[0]; left[1] += A[1]; left[2] += A[2];
		color[0] += AA[0]; color[1] += AA[1]; color[2] += AA[2];
		CY(2); /* Originally 6 */
	    }
	    for (; x <= max_x; x++) {
		if (CY(0), left[0] < 0 && left[1] < 0 && left[2] < 0) {
		    CY(1); /* Originally 10 */
		    screen[y][x] = (color[0] >> 16 & 0xFF) |
			    (color[1] >> 8 & 0xFF00) |
			    (color[2] & 0xFF0000);
		    pixels++;
		} else {
		    break;
		}
		left[0] += A[0]; left[1] += A[1]; left[2] += A[2];
		color[0] += AA[0]; color[1] += AA[1]; color[2] += AA[2];
		CY(2); /* Originally 6 */
	    }
	    left[0] = old_left[0] + B[0];
	    left[1] = old_left[1] + B[1];
	    left[2] = old_left[2] + B[2];
	    color[0] = old_color[0] + BB[0];
	    color[1] = old_color[1] + BB[1];
	    color[2] = old_color[2] + BB[2];
	    CY(6);
	}
    }

#ifdef IRIS
    lrectwrite (0, 0, 639, 399, (unsigned long *)screen);
    if (getbutton (LEFTSHIFTKEY))
#endif
    {
	printinfo (0, det, AA[0], BB[0], CC[0]);
	printinfo (1, det, AA[1], BB[1], CC[1]);
	printinfo (2, det, AA[2], BB[2], CC[2]);
    }
    if (pixels != 0) {
	printf ("%ld cycles total, %ld pixels, %.1f cycles per pixel\n",
		cycle, pixels, (float)cycle / pixels);
    } else {
	printf ("No pixels\n");
    }
}

void redraw (void)
{
    memset (screen, 0, sizeof (screen));
    rasterize(p1, red, p2, green, p3, blue);
}

int main(void)
{

#ifdef IRIS
    long	winid, ox, oy, dev, inside;
    short	val;

    prefposition (256, 256 + 640 - 1, 256, 256 + 480 - 1);
    foreground ();
    winid = winopen ("rasterize");
    RGBmode();
    gconfig ();
    qdevice (ESCKEY);
    qdevice (SKEY);
    qdevice (TKEY);
    qdevice (RIGHTSHIFTKEY);
    qenter (REDRAW, winid);
    getorigin (&ox, &oy);
    getsize (&sx, &sy);
    inside = 1;
#endif

    usetree = 0;
    drawall = 0;

#ifdef IRIS
    while (1) {
	redraw ();
	if (getbutton (LEFTMOUSE) && inside) {
	    p1[0] = getvaluator (MOUSEX) - ox;
	    p1[1] = getvaluator (MOUSEY) - oy;
	}
	if (getbutton (MIDDLEMOUSE) && inside) {
	    p2[0] = getvaluator (MOUSEX) - ox;
	    p2[1] = getvaluator (MOUSEY) - oy;
	}
	if (getbutton (RIGHTMOUSE) && inside) {
	    p3[0] = getvaluator (MOUSEX) - ox;
	    p3[1] = getvaluator (MOUSEY) - oy;
	}
	if (qtest ()) {
	    dev = qread (&val);
	    switch (dev) {
		case REDRAW:
		    reshapeviewport ();
		    getorigin (&ox, &oy);
		    getsize (&sx, &sy);
		    break;
		case INPUTCHANGE:
		    inside = val;
		    break;
		case RIGHTSHIFTKEY:
		    drawall = val;
		    break;
		case SKEY:
		    if (val == 1) {
			/* 50-pixel triangle */
			p1[0] = 300; p1[1] = 200;
			p2[0] = 305; p2[1] = 211;
			p3[0] = 310; p3[1] = 200;
		    }
		    break;
		case TKEY:
		    if (val == 1) {
			usetree = !usetree;
		    }
		    break;
		case ESCKEY:
		    if (val == 0) {
			exit (0);
		    }
		    break;
	    }
	}
    }
#else
    redraw ();
#endif

    return 0;
}
