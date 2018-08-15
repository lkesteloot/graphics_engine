#include "dl.h"

int main() {
    double	identity[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
    };

    dlBeginFile("emem");

	dlMatrixMode (DL_PROJECTION);
	dlLoadMatrix (identity);
	dlMatrixMode (DL_MODELVIEW);
	dlLoadMatrix (identity);

	dlDisable (DL_LIGHTING);
	dlDisable (DL_NORMALIZE);
	dlEnable (DL_SMOOTH);
	dlEnable (DL_DEPTH_READ);
	dlEnable (DL_DEPTH_WRITE);
	dlEnable (DL_CULL);

	dlBegin(DL_POLYGON);
	    dlColor(1, 0, 0);
	    dlVertex(100,100,10);
	    dlColor(1, 0, 0);
	    dlVertex(100, 380, 10);
	    dlColor(1, 0, 0);
	    dlVertex(420, 240, 0);
	dlEnd();
	dlBegin(DL_POLYGON);
	    dlColor(0, 0, 1);
	    dlVertex(540,100,10);
	    dlColor(0, 0, 1);
	    dlVertex(220, 240, 0);
	    dlColor(0, 0, 1);
	    dlVertex(540, 380, 10);
	dlEnd();

    dlEndFile();

    return 0;
}
