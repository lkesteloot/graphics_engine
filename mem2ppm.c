
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

unsigned char	mem[2*1024*1024], *p;

void usage (void) {
    fprintf (stderr, "usage: mem2ppm [-m memfile] [-o ppmfile] [-f fbaddr]\n");
    fprintf (stderr, "  memfile defaults to \"emem.dump\"\n");
    fprintf (stderr, "  ppmfile defaults to stdout\n");
    fprintf (stderr, "  fbaddr defaults to 256*1024 and is in bytes\n");
    exit (1);
}

int main (int argc, char *argv[]) {
    int		c, fbaddr, inf, i;
    char	*memfile, *outfile;
    FILE	*outf;
    unsigned short col;
    unsigned char rgb[3];

    memfile = "emem.dump";
    outfile = NULL;
    fbaddr = 256*1024;

    while ((c = getopt (argc, argv, ":m:o:f:")) != -1) {
	switch (c) {
	    case 'm':
		memfile = optarg;
		break;
	    case 'o':
		outfile = optarg;
		break;
	    case 'f':
		fbaddr = atoi (optarg);
		break;
	    case ':':
		fprintf (stderr, "Option -%c requires an argument\n", optopt);
		usage ();
	    case '?':
		fprintf (stderr, "Unrecognized option -%c\n", optopt);
		usage ();
	}
    }

    inf = open (memfile, O_RDONLY);
    if (inf == -1) {
	perror (memfile);
	exit (1);
    }
    read (inf, mem, sizeof (mem));
    close (inf);

    if (outfile == NULL) {
	outf = stdout;
    } else {
	outf = fopen (outfile, "w");
	if (outf == NULL) {
	    perror (outfile);
	    exit (1);
	}
    }

    fprintf (outf, "P6 640 480 255\n");

    p = &mem[fbaddr];

    for (i = 0; i < 640 * 480; i++) {
        // Big endian.
	col = (p[0] << 8) | p[1];

        // Unpack 5-6-5 into 8-8-8.
	rgb[0] = (col & 0xF800) >> 8 | (col & 0xF800) >> 13;
	rgb[1] = (col & 0x07E0) >> 3 | (col & 0x07E0) >> 9;
	rgb[2] = (col & 0x001F) << 3 | (col & 0x001F) >> 2;
	fwrite (&rgb, sizeof (rgb), 1, outf);
	p += 2;
    }

    fclose (outf);

    return 0;
}
