#include <omp.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <errno.h>
#include <hbwmalloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

using namespace std;

// static int NX = 1024*8;
// static int NY = 1024*4;
// static int NT = 200;

#define NX 1024*8
#define NY 1024*4
#define NT 100

#define f(x,y)     (sin(x)*sin(y))
#define randa(x,t) (0.0)
#define randb(x,t) (exp(-2*(t))*sin(x))
#define randc(y,t) (0.0)
#define randd(y,t) (exp(-2*(t))*sin(y))
#define solu(x,y,t) (exp(-2*(t))*sin(x)*sin(y))


int nx, ny, nt;
double xu, xo, yu, yo, tu, to;
double dx, dy, dt;

double dtdxsq, dtdysq;
double t;

int leafmaxcol;

void swap_ranks(double ***from_ranks, double ***to_ranks) {
  double **tmp_ranks = *from_ranks;
  *from_ranks = *to_ranks;
  *to_ranks = tmp_ranks;
}

int heat() {
	struct timeval start, finish;
	struct timeval start_1, finish_1;
	struct timeval collectstart, collectfinish;
	double **before, **after;
	int  c, l;
	size_t malloc_size;
	int f;
	char write_info[100];

	f = open("/proc/heat", O_WRONLY | O_TRUNC);
	before = (double **)malloc(nx * sizeof(double *));
	after = (double **) malloc(nx * sizeof(double *));

  	#pragma omp parallel for schedule(static, 8) 
  	for (int i = 0; i < nx; ++i) 
  	{
  		*(before + i) = (double *) malloc(ny * sizeof(double));
    	*(after + i) = (double *) malloc(ny * sizeof(double));
		printf("before: 0x%lx 0x%lx\n", *(before + i), *(before + i) + ny * sizeof(double));
		printf("after: 0x%lx 0x%lx\n", *(after + i), *(after + i) + ny * sizeof(double));
	}

	malloc_size = 2 * nx * ny * sizeof(double) / (1 << 12);
	ftruncate(f, 0);
    lseek(f, 0, SEEK_SET);
	sprintf(write_info, "malloc 1 %ld", malloc_size);
	write(f, write_info, strlen(write_info));
	cout << "FINISH ALLOCTION *************************" << endl;

	#pragma omp parallel for schedule(static, 8) 
  	for (int i = 0; i < nx; ++i) 
  	{
  		int a, b, llb, lub;

  		a = i;
  		b = 0;
  		before[a][b] = randa(xu + a * dx, 0);

  		b = ny - 1;
  		before[a][b] = randb(xu + a * dx, 0);

  		if (i == 0) {
  			for (a=0, b=0; b < ny; b++){
      			before[a][b] = randc(yu + b * dy, 0);
  			}
  		} else if (i == nx - 1) {
  			for (a=nx-1, b=0; b < ny; b++){
      			before[a][b] = randd(yu + b * dy, 0);
  			}
  		} else {
  			for (b=1; b < ny-1; b++) {
      			before[a][b] = f(xu + a * dx, yu + b * dy);
    		}
  		}
  	}

  	cout << "FINISH INITTTTT *************************" << endl;
	ftruncate(f, 0);
    lseek(f, 0, SEEK_SET);
	sprintf(write_info, "filter %d", getpid());
	write(f, write_info, strlen(write_info));

	gettimeofday(&start, NULL);
	for (c = 1; c <= nt; c++) {
  		t = tu + c * dt;
		gettimeofday(&start_1, NULL);

		#pragma omp parallel for schedule(static, 8)
	    for (int i = 0; i < nx; ++i)
	    {
	        int a, b, llb, lub;
	    
		    if (i == 0) {
		    	for (a=0, b=0; b < ny; b++){
      				before[a][b] = randc(yu + b * dy, t);
		    	}
		    } else if (i == nx - 1) {
		    	for (a=nx-1, b=0; b < ny; b++){
      				before[a][b] = randd(yu + b * dy, t);
		    	}
		    } else {
		    	after[i][ny-1] = randb(xu + a * dx, t);
		    	after[i][0] = randa(xu + a * dx, t);

		    	for (a=i, b=1; b < ny-1; b++) {
			      	after[a][b] =   dtdxsq * (before[a+1][b] - 2 * before[a][b] + before[a-1][b])
				          + dtdysq * (before[a][b+1] - 2 * before[a][b] + before[a][b-1])
			  	          + before[a][b];
			    }
		    }
	    }

  		gettimeofday(&finish_1, NULL);
        printf("Elapsed time of iteration[%d]: %10.6f u seconds\n", c,
         (((finish_1.tv_sec * 1000000.0) + finish_1.tv_usec) -
        ((start_1.tv_sec * 1000000.0) + start_1.tv_usec)) / 1.0);

		gettimeofday(&collectstart, NULL);
		ftruncate(f, 0);
    	lseek(f, 0, SEEK_SET);
		sprintf(write_info, "collect %d", getpid());
		write(f, write_info, strlen(write_info));
		gettimeofday(&collectfinish, NULL);
		printf("Elapsed time of collection[%d]: %10.6f u seconds\n", c,
         (((collectfinish.tv_sec * 1000000.0) + collectfinish.tv_usec) -
        ((collectstart.tv_sec * 1000000.0) + collectstart.tv_usec)) / 1.0);

        swap_ranks(&after, &before);
  	}

  	gettimeofday(&finish, NULL); 
  	printf("Elapsed time of Heat: %10.6f u seconds\n\n", 
		 (((finish.tv_sec * 1000000.0) + finish.tv_usec) -
	  	((start.tv_sec * 1000000.0) + start.tv_usec)) / 1.0);
	ftruncate(f, 0);
	lseek(f, 0, SEEK_SET);
	sprintf(write_info, "print");
	write(f, write_info, strlen(write_info));
	close(f);
	return 0;
}


int main(int argc, char *argv[]){
	nx = atoi(argv[1])*1024;
	ny = NY;
	nt = NT;
	xu = 0.0;
  	xo = 1.570796326794896558;
  	yu = 0.0;
  	yo = 1.570796326794896558;
  	tu = 0.0;
  	to = 0.0000001;
  	leafmaxcol = 8;

  	dx = (xo - xu) / (nx - 1);
  	dy = (yo - yu) / (ny - 1);
  	dt = (to - tu) / nt;	/* nt effective time steps! */

  	dtdxsq = dt / (dx * dx);
  	dtdysq = dt / (dy * dy);

  	heat();
    
    return 0;
}

