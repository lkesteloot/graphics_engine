#ifndef _DL_H_
#define _DL_H_

/* DL Constants */

/* BeginMode */
#define DL_POLYGON                          0x0001

/* MatrixMode */
#define DL_MODELVIEW                        0x0
#define DL_PROJECTION                       0x1

#define DL_LIGHTING	0
#define DL_NORMALIZE	1
#define DL_SMOOTH	2
#define DL_DEPTH_READ	3
#define DL_DEPTH_WRITE	4
#define DL_CULL		5



/* Function Prototypes */
extern void dlBeginFile(const char *filename);
extern void dlEndFile(void);
extern void dlBegin (int mode);
extern void dlCall (int a);
extern int dlBeginList (void);
extern void dlEndList (void);
extern void dlColor (double red, double green, double blue);
extern void dlNormal(double nx, double ny, double nz);
extern void dlEnd (void);
extern void dlFlush (void);
extern void dlFrustum (double left, double right, double bottom, 
		       double top, double near, double far);
extern void dlLight(double lx, double ly, double lz,
		    double r, double g, double b);
extern void dlLoadMatrix(const double *m);
extern void dlMatrixMode (int mode);
extern void dlMultMatrix (const double *m);
extern void dlPopMatrix (void);
extern void dlPushMatrix (void);
extern void dlReturn (void);
extern void dlRotate (double andle, double x, double y, double z);
extern void dlScale (double x, double y, double z);
extern void dlTranslate (double x, double y, double z);
extern void dlVertex (double x, double y, double z);

/*
extern void dlViewport (DLint x, DLint y, DLsizei width, DLsizei height);

extern void dluLookAt (double eyex, double eyey, double eyez,
		       double centerx, double centery, double centerz,
		       double upx, double upy, double upz);

*/

extern void dlEnable (int cap);

extern void dlDisable (int cap);

#endif /* _DL_H_ */
