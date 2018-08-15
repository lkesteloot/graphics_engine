#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "dl.h"
#include "dlopcode.h"

#define BUFLEN		1024

static int outf;
static int inBegin = 0;
static int vertexNumber = 0;
static unsigned char buffer[BUFLEN];
static int matrixMode, buflen;
static int addr, mainaddr;
static int totalPolygons = 0;

void writebyte (char b) {
    if (buflen == BUFLEN) {
	write (outf, buffer, buflen);
	buflen = 0;
    }
    buffer[buflen++] = b;
    addr++;
}

void writeshort (short s) {
    /* Big endian */
    writebyte (s >> 8);
    writebyte (s >> 0);
}

void writethree (long a) {
    /* Big endian */
    writebyte (a >> 16);
    writebyte (a >> 8);
    writebyte (a >> 0);
}

void writelong (long a) {
    /* Big endian */
    writebyte (a >> 24);
    writebyte (a >> 16);
    writebyte (a >> 8);
    writebyte (a >> 0);
}

void writematrix (const double *m) {
    int		i;

    for (i = 0; i < 16; i++) {
	writelong ((int)(m[i] * (1 << 16)));
    }
}

int dlBeginList (void) {
    return addr;
}

void dlEndList (void) {
    writebyte (DL_RETURN);
    writethree (0);
    mainaddr = addr;
}

void dlBeginFile(const char *filename) {
    outf = open (filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (outf == -1) {
        perror (filename);
        exit (1);
    }
    inBegin = 0;
    vertexNumber = 0;
    addr = 0;
    mainaddr = 0;

    writebyte (DL_SET); /* Basically NOP, will be written-over later */
    writethree (0);
    writebyte (DL_SET); /* Ditto */
    writethree (0);
}

void dlEndFile(void) {
    writebyte (DL_RETURN);
    writethree (0);
    write (outf, buffer, buflen); /* Flush buffer */
    buflen = 0;
    if (mainaddr != 0) {
	lseek (outf, 0, SEEK_SET);
	writebyte (DL_CALL);
	writethree (mainaddr / 4);
	writebyte (DL_RETURN);
	writethree (0);
	write (outf, buffer, buflen); /* Flush buffer again */
	buflen = 0;
    }
    close (outf);

    fprintf(stderr, "Wrote %d total polygons.\n", totalPolygons);
}

void dlVertex(double x, double y, double z) {
    if (!inBegin) {
	fprintf(stderr, "dlVertex called outside of dlBegin-dlEnd pair\n");
	exit(1);
    }

    writebyte (DL_VERTEX);
    writebyte (vertexNumber);

    writeshort ((short)x);
    writeshort ((short)y);
    writeshort ((short)z);

    vertexNumber++;
}

void dlCall (int a) {
    writebyte (DL_CALL);
    writethree (a / 4);
}

void dlReturn(void) {
    writebyte (DL_RETURN);
    writethree (0);
}

void dlPopMatrix(void) {
    writebyte (DL_POP);
    writethree (0);
}

void dlPushMatrix(void) {
    writebyte (DL_PUSH);
    writethree (0);
}

void dlMatrixMode(int mode) {
    matrixMode = mode;
}

void dlLoadMatrix(const double *m) {
    writebyte (DL_LOAD);
    writeshort (0);
    writebyte (matrixMode);
    writematrix (m);
}

void dlMultMatrix(const double *m) {
    writebyte (DL_MULT);
    writeshort (0);
    writebyte (matrixMode);
    writematrix (m);
}

void dlLight(double lx, double ly, double lz, double r, double g, double b) {
    float	len;

    len = sqrt (lx*lx + ly*ly + lz*lz);

    if (len == 0) {
	return;
    }
    lx /= len;
    ly /= len;
    lz /= len;

    writebyte (DL_LIGHT);
    writebyte ((signed char)(-lx * 127));
    writebyte ((signed char)(-ly * 127));
    writebyte ((signed char)(-lz * 127));
    writebyte (0);
    writebyte ((unsigned char)(r * 255));
    writebyte ((unsigned char)(g * 255));
    writebyte ((unsigned char)(b * 255));
}

void dlColor(double red, double green, double blue) {
    writebyte (DL_COLOR);
    writebyte ((unsigned char) (red * 255));
    writebyte ((unsigned char) (green * 255));
    writebyte ((unsigned char) (blue * 255));
}

void dlNormal(double nx, double ny, double nz) {
    float	len;

    len = sqrt (nx*nx + ny*ny + nz*nz);

    if (len == 0) {
	return;
    }
    nx /= len;
    ny /= len;
    nz /= len;

    writebyte (DL_NORMAL);
    writebyte ((signed char) (nx * 127));
    writebyte ((signed char) (ny * 127));
    writebyte ((signed char) (nz * 127));
}

void dlBegin(int mode) {
    if (mode == DL_POLYGON) {
	inBegin = 1;
	vertexNumber = 0;
        totalPolygons++;
    } else {
	fprintf(stderr, "Unknown mode in dlBegin (%d)\n", mode);
	exit(1);
    }
}

void dlEnd(void) {
    if (!inBegin) {
	fprintf(stderr, "dlEnd called outside of dlBegin-dlEnd pair\n");
	exit(1);
    } else {
	inBegin = 0;
    }
}

void dlRotate (double angle, double x, double y, double z) {
    double	m[16], len, u[3], sa, ca;
    int		i, j;

    for (i = 0; i < 16; i++) {
	m[i] = 0;
    }

    len = sqrt (x*x + y*y + z*z);

    if (len == 0) {
	return;
    }

    u[0] = x / len;
    u[1] = y / len;
    u[2] = z / len;

    m[1] = -u[2];
    m[2] =  u[1];
    m[6] = -u[0];

    m[4] =  u[2];
    m[8] = -u[1];
    m[9] =  u[0];

    angle *= M_PI / 180;

    sa = sin (angle);
    ca = cos (angle);

    for (i = 0; i < 16; i++) {
	m[i] *= sa;
    }

    m[0] += ca;
    m[5] += ca;
    m[10] += ca;

    ca = 1 - ca;

    for (i = 0; i < 3; i++) {
	for (j = 0; j < 3; j++) {
	    m[i * 4 + j] += ca * u[i] * u[j];
	}
    }

    m[15] = 1;

    dlMultMatrix (m);
}

void dlScale (double x, double y, double z) {
    static double m[] = {
	1,  0,  0,  0,
	0,  1,  0,  0,
	0,  0,  1,  0,
	0,  0,  0,  1,
    };

    m[0] = x;
    m[5] = y;
    m[10] = z;

    dlMultMatrix (m);
}

void dlTranslate (double x, double y, double z) {
    static double m[] = {
	1,  0,  0,  0,
	0,  1,  0,  0,
	0,  0,  1,  0,
	0,  0,  0,  1,
    };

    m[3] = x;
    m[7] = y;
    m[11] = z;

    dlMultMatrix (m);
}

void dlEnable(int cap) {
    writebyte (DL_SET);
    writethree (1 << cap);
}

void dlDisable(int cap) {
    writebyte (DL_RESET);
    writethree (1 << cap);
}
