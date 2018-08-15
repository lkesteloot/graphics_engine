

/*
 * Squart-root test
 */

#include <stdio.h>
#include <math.h>

typedef signed long Word;

Word m (Word A, Word B)
{
    Word	A1, A2, B1, B2, t1, t2, t3, t4, res;
    int		sign;

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

int main (int argc, char *argv[]) {
    Word	k, s, x, r, kk, fixed, seed;
    int		i;
    float	kf, sf, xf, rf;

    if (argc == 1) {
	fprintf (stderr, "usage: sqrt [number]\n");
	exit (1);
    }

    fixed = 1 << 16;

    kf = atof (argv[1]);
    k = kf * fixed;

    printf ("Number = %g, fixed-point = %ld (0x%08lx)\n", kf, k, k);

    sf = sqrt (kf);
    s = sf * fixed;

    printf ("Square-root = %g, fixed-point = %ld (0x%08lx)\n", sf, s, s);

    sf = 1/sf;
    s = sf * fixed;

    printf ("1/sqrt = %g, fixed-point = %ld (0x%08lx)\n", sf, s, s);

    rf = 1/kf;
    r = rf * fixed;

    printf ("1/k = %g, fixed-point = %ld (0x%08lx)\n", rf, r, r);

    if (kf < 0) {
	printf ("Cannot be negative\n");
	exit (1);
    }

    x = fixed;
    kk = k;

    if (k < fixed) {
	while (kk < fixed) {
	    x <<= 1;
	    kk <<= 2;
	}
    } else {
	while (kk > fixed) {
	    x >>= 1;
	    kk >>= 2;
	}
    }

    seed = x;

    printf ("Seed = %ld (0x%08lx, %g)\n", x, x, x / (float)fixed);

    for (i = 0; i < 10; i++) {
	x = m (x, 3*fixed - m (k, m (x, x))) >> 1;
	printf ("Iteration: %g, 0x%08lx\n", x / (float)fixed, x);
    }

    printf ("\n");

    xf = seed / (float)fixed;

    for (i = 0; i < 10; i++) {
	xf = xf * (3 - kf*xf*xf) / 2;
	printf ("Iteration: %g, 0x%08x\n", xf, (int)(xf * fixed));
    }

    printf ("\n");

    x = fixed;
    kk = k;

    if (k < fixed) {
	while (kk < fixed) {
	    x <<= 1;
	    kk <<= 1;
	}
    } else {
	while (kk > fixed) {
	    x >>= 1;
	    kk >>= 1;
	}
    }
    seed = x;
    printf ("Seed = %ld (0x%08lx, %g)\n", x, x, x / (float)fixed);

    for (i = 0; i < 10; i++) {
	x = m (x, 2*fixed - m (k, x));
	printf ("Iteration: %g, 0x%08lx\n", x / (float)fixed, x);
    }

    printf ("\n");

    xf = seed / (float)fixed;
    for (i = 0; i < 10; i++) {
	xf = xf * (2 - kf*xf);
	printf ("Iteration: %g, 0x%08x\n", xf, (int)(xf * fixed));
    }

    return 0;
}
