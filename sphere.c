#include <math.h>
#include "dl.h"

void main() {
    int		sublist, i;
    double	identity[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
    };
    double	t1, t2, r1, g1, b1, r2, g2, b2;

    dlBeginFile("emem");

	dlMatrixMode (DL_PROJECTION);
	/* dlLoadMatrix (identity); */
	dlMatrixMode (DL_MODELVIEW);
	/* dlLoadMatrix (identity); */

	dlDisable (DL_LIGHTING);
	dlDisable (DL_NORMALIZE);
	dlEnable (DL_SMOOTH);
	dlEnable (DL_DEPTH_READ);
	dlEnable (DL_DEPTH_WRITE);
	dlEnable (DL_CULL);

	for (i = 0; i < 25; i++) {
	    t1 = i / 25.0 * M_PI * 2;
	    t2 = (i + 1) / 25.0 * M_PI * 2;

	    r1 = sin (t1) * 0.5 + 0.5;
	    g1 = sin (t1 + M_PI/3) * 0.5 + 0.5;
	    b1 = sin (t1 + M_PI/3*2) * 0.5 + 0.5;
	    r2 = sin (t2) * 0.5 + 0.5;
	    g2 = sin (t2 + M_PI/3) * 0.5 + 0.5;
	    b2 = sin (t2 + M_PI/3*2) * 0.5 + 0.5;

	    dlBegin(DL_POLYGON);
		dlColor(r2, g2, b2);
		dlVertex(sin (t2) * 200 + 320, cos (t2) * 200 + 240 , 0);
		dlColor(r1, g1, b1);
		dlVertex(sin (t1) * 200 + 320, cos (t1) * 200 + 240 , 0);
		dlColor(1, 1, 1);
		dlVertex(320, 240, 0);
	    dlEnd();
	}

    dlEndFile();
}
