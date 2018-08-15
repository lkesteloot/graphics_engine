#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "unasm.h"
#include "dlopcode.h"
#include "util.h"

void usage (void) {
    fprintf (stderr, "usage: disasm [-d] mem\n");
    fprintf (stderr, "    -d   treat mem like a display list\n");
    exit (1);
}

int main (int argc, char *argv[]) {
    int			i, j, n, f, c, dl;
    unsigned long	*buf, addr, op, inst;

    if (argc < 2) {
	usage ();
    }

    dl = 0;

    while ((c = getopt (argc, argv, "d")) != EOF) {
	switch (c) {
	    case 'd':
		dl = 1;
		break;
	    case '?':
		usage ();
	}
    }

    f = open (argv[optind], O_RDONLY);
    if (f == -1) {
	perror (argv[optind]);
	exit (1);
    }

    n = lseek (f, 0, SEEK_END) / sizeof (long);
    lseek (f, 0, SEEK_SET);
    buf = (unsigned long *)malloc (n * sizeof (long));

    n = read (f, buf, n * sizeof (long));
    if (n == -1) {
	perror ("read");
	exit (1);
    }

    n /= sizeof (long);

    // The applications (like sphere.c) write things
    // in big-endian explicitly, so swap everything here.
    SwapWords(buf, n);

    if (dl) {
	for (addr = 0; addr < n; addr++) {
	    inst = buf[addr];
	    op = inst >> 24;
	    inst &= 0x00FFFFFF;
	    printf ("0x%04lx: ", addr);
	    switch (op) {
		case DL_VERTEX:
		    printf ("Vertex #%ld: ", inst >> 16);
		    printf ("V: (%d,%d,%d)\n",
			(int)(signed short)(buf[addr + 0] & 0xFFFF),
			(int)(signed short)(buf[addr + 1] >> 16),
			(int)(signed short)(buf[addr + 1] & 0xFFFF));
		    addr++;
		    break;
		case 1:
		    printf ("Call: 0x%lx\n", inst);
		    break;
		case 2:
		    printf ("Return\n");
		    break;
		case 3:
		    printf ("Push Matrix\n");
		    break;
		case 4:
		    printf ("Pop Matrix\n");
		    break;
		case 5:
		    printf ("Load Matrix (%s)\n",
			inst ? "Projection" : "Model-View");
		    for (i = 0; i < 4; i++) {
			printf ("    ");
			for (j = 0; j < 4; j++) {
			    printf ("%8g", buf[addr + 1 + i*4 + j] / 65536.0);
			}
			printf ("\n");
		    }
		    addr += 16;
		    break;
		case 6:
		    printf ("Multiply Matrix (%s)\n",
			inst ? "Projection" : "Model-View");
		    for (i = 0; i < 4; i++) {
			printf ("    ");
			for (j = 0; j < 4; j++) {
			    printf ("%8g", buf[addr + i*4 + j] / 65536.0);
			}
			printf ("\n");
		    }
		    addr += 16;
		    break;
		case 7:
		    printf ("Lighting: Dir (%d,%d,%d), color (%d,%d,%d)\n",
			(int)(signed char)(inst >> 16 & 0xFF),
			(int)(signed char)(inst >> 8 & 0xFF),
			(int)(signed char)(inst >> 0 & 0xFF),
			(int)(unsigned char)(buf[addr + 1] >> 16 & 0xFF),
			(int)(unsigned char)(buf[addr + 1] >> 8 & 0xFF),
			(int)(unsigned char)(buf[addr + 1] >> 0 & 0xFF));
		    addr++;
		    break;
		case 8:
		    printf ("Set mode: 0x%04lx\n", inst);
		    break;
		case 9:
		    printf ("Reset mode: 0x%04lx\n", inst);
		    break;
		case 10:
		    printf ("Set color: (%d,%d,%d)\n",
			(int)(unsigned char)(inst >> 16 & 0xFF),
			(int)(unsigned char)(inst >> 8 & 0xFF),
			(int)(unsigned char)(inst >> 0 & 0xFF));
		    break;
		case 11:
		    printf ("Set normal: (%d,%d,%d)\n",
			(int)(signed char)(inst >> 16 & 0xFF),
			(int)(signed char)(inst >> 8 & 0xFF),
			(int)(signed char)(inst >> 0 & 0xFF));
		    break;
		default:
		    printf ("Unknown (%lu, 0x%08lx)\n", op, inst);
		    break;
	    }
	}
    } else {
	for (addr = 0; addr < n; addr++) {
	    printf ("0x%04lx: %s\n", addr, unasm (buf[addr]));
	}
    }

    free (buf);

    return 0;
}
