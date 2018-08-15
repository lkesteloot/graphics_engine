#include "dl.h"

int main() {
    int		sublist;
    double	identity[16] = {
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1,
    };

    dlBeginFile("emem");

	sublist = dlBeginList ();
		dlBegin(DL_POLYGON);
		    dlColor(1, 0, 0);
		    dlVertex(123,45 , 0);
		    dlColor(0, 1, 0);
		    dlVertex(510, 5, 0);
		    dlColor(0, 0, 1);
		    dlVertex(42, 420, 0);
		dlEnd();
	dlEndList ();

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
	    dlColor(0, 1, 1);
	    dlVertex(10, 10, 0);
	    dlVertex(100, 10, 0);
	    dlVertex(100, 100, 0);
	    dlVertex(50, 200, 0);
	    dlVertex(10, 100, 0);
	dlEnd();
	dlCall (sublist);

    dlEndFile();

    return 0;
}
