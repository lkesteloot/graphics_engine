#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "dl.h"

#define INPUT_FILE "teapot.geom"

typedef struct {
    double x, y, z;
} Vertex;


Vertex *vertex;
Vertex *normal, *fnormal, *color;
int *triangle;
int num_triangles;

void read_teapot( )
{
    FILE *fd;
    int i, j, dummy;
    int num_vertices;
    int *t;
    Vertex *v1, *v2, *v3;
    double x1, y1, z1, x2, y2, z2, nx, ny, nz, len, maxy, miny, phi;
    
    fd = fopen(INPUT_FILE,"r");
    fscanf(fd, "%d %d %d", &num_vertices, &num_triangles, &dummy);

    vertex = (Vertex *) malloc(num_vertices*sizeof(Vertex));
    normal = (Vertex *) malloc(num_vertices*sizeof(Vertex));
    color = (Vertex *) malloc(num_vertices*sizeof(Vertex));
    fnormal = (Vertex *) malloc(num_triangles*sizeof(Vertex));
    t = triangle = (int *) malloc(num_triangles*3*sizeof(int));

    maxy = -100;
    miny = 100;
    for (i = 0; i < num_vertices; i++) {
	fscanf(fd, "%lf %lf %lf", &vertex[i].x, &vertex[i].y, &vertex[i].z);
	if (vertex[i].y < miny) {
	    miny = vertex[i].y;
	}
	if (vertex[i].y > maxy) {
	    maxy = vertex[i].y;
	}
    }

    for (i = 0; i < num_triangles; i++) {
	fscanf(fd, "%d", &dummy);
	if (dummy != 3) fprintf(stderr, "ERROR: polygon is not a triangle\n");
	fscanf(fd, "%d", t); (*t)--; v1 = &vertex[*t]; t++;
	fscanf(fd, "%d", t); (*t)--; v2 = &vertex[*t]; t++;
	fscanf(fd, "%d", t); (*t)--; v3 = &vertex[*t]; t++;
	x1 = v2->x - v1->x;
	y1 = v2->y - v1->y;
	z1 = v2->z - v1->z;
	x2 = v2->x - v3->x;
	y2 = v2->y - v3->y;
	z2 = v2->z - v3->z;
	fnormal[i].x = y1 * z2 - z1 * y2;
	fnormal[i].y = z1 * x2 - x1 * z2;
	fnormal[i].z = x1 * y2 - y1 * x2;
    }
    fclose(fd);

    /*
     * Average normals for each vertex to smooth surface.
     */

    for (i = 0; i < num_vertices; i++) {
	nx = 0;
	ny = 0;
	nz = 0;
	for (j = 0; j < num_triangles; j++) {
	    if (triangle[j*3] == i || triangle[j*3+1] == i ||
	      triangle[j*3+2] == i) {
		nx += fnormal[j].x;
		ny += fnormal[j].y;
		nz += fnormal[j].z;
	    }
	}
	len = sqrt (nx*nx + ny*ny + nz*nz);
	nx /= len;
	ny /= len;
	nz /= len;
	normal[i].x = nx;
	normal[i].y = ny;
	normal[i].z = nz;

#if 0
	color[i].x = nx * 0.5 + 0.5;
	color[i].y = ny * 0.5 + 0.5;
	color[i].z = nz * 0.5 + 0.5;
#endif

#if 0
	color[i].x = fabs (nx);
	color[i].y = fabs (ny);
	color[i].z = fabs (nz);
#endif

	phi = (vertex[i].y - miny) / (maxy - miny) * 2 * M_PI;
	color[i].x = sin (phi) * 0.5 + 0.5;
	color[i].y = sin (phi + M_PI / 3) * 0.5 + 0.5;
	color[i].z = sin (phi + M_PI * 2 / 3) * 0.5 + 0.5;
    }
}

void draw_teapot( )
{
    Vertex *v = vertex;
    int *t = triangle;
    int i, index;
    
    for (i=0; i < num_triangles; i++) {
	dlBegin(DL_POLYGON);
	index = *t++;
	dlNormal(normal[index].x, normal[index].y, -normal[index].z);
	dlColor(color[index].x, color[index].y, color[index].z);
	dlVertex(v[index].x*50, v[index].y*50, v[index].z*50);
	index = *t++;
	dlNormal(normal[index].x, normal[index].y, -normal[index].z);
	dlColor(color[index].x, color[index].y, color[index].z);
	dlVertex(v[index].x*50, v[index].y*50, v[index].z*50);
	index = *t++;
	dlNormal(normal[index].x, normal[index].y, -normal[index].z);
	dlColor(color[index].x, color[index].y, color[index].z);
	dlVertex(v[index].x*50, v[index].y*50, v[index].z*50);
	dlEnd();
    }
}

int main(void)
{
#if 0
    double      m[16] = {
	 1,  0,  0, 350,
	 0,  1,  0, 200,
	 0,  0,  1, 150,
	 0,  0,  0, 1,
    };
#endif
#if 0
    double      m[16] = {
	 1,    0,    0, 350,
	 0,  .86,   .5, 200,
	 0,  -.5,  .86, 150,
	 0,    0,    0,   1,
    };
#endif
#if 0
    double      m[16] = {
	 1,    0,    0, 350,
	 0,    0,    1, 200,
	 0,   -1,    0, 150,
	 0,    0,    0,   1,
    };
#endif
#if 1
    double      m[16] = {
	  1,   0,   0,  0,
	  0,   1,   0,  0,
	  0,   0,   1,  0,
	  0,   0,   0,  1,
    };
#endif

    read_teapot( );

    dlBeginFile("emem");

    /*dlMatrixMode(DL_PROJECTION);
    dlLoadMatrix();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 2.0, 30.0); 
    glFrustum(-3.5, 3.5, -3.5, 3.5, 7.0, 30.0);*/


    dlMatrixMode(DL_MODELVIEW);
    dlLoadMatrix(m);

    dlTranslate (350, 200, 150);
    dlRotate (-30, 1, 0, 0);
    dlRotate (30, 0, 1, 0);

    /*gluLookAt(0.0,3.0,8.0, 0.0,0.0,0.0, 0.0,1.0,0.0);*/

#if 0
    /* lower left */
    glViewport(0,0,256,256);
    glPushMatrix();
    glScale(2.0, 2.0, 2.0);
    draw_teapot();
    glPopMatrix();
    glColor(1.0,1.0,0.0);
    
    /* lower right */
    glViewport(256,0,256,256);
    glPushMatrix();
    glColor(0.0,0.0,1.0);
    glRotate(30.0,1.0,0.0,0.0);
    draw_teapot();
    glPopMatrix();

    /* upper left */
    glViewport(0,256,256,256);
    glPushMatrix();
    glColor(1.0,0.0,0.0);
    draw_teapot();
    glPopMatrix();

    /* upper right */
    glViewport(256,256,256,256);
    glColor(0.0, 1.0, 0.0);
    glScale(1.0, 1.0, 1.0);
    for (i = 0; i < 180; i++) {
	glClear(GL_COLOR_BUFFER_BIT);
	glRotate(2.0, 0.0, 1.0, 0.1);
	draw_teapot();
    }
#endif

    dlEnable (DL_LIGHTING);
    dlLight (1, -1, -1, 1, 1, 1);

    /* dlColor (.7, 1, 1); */
    draw_teapot();
    dlEndFile();

    return 0;
}
