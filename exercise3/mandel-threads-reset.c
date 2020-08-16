/*
 * mandel-threads.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm, using multiple threads.
 *
 * To sychronize the threads it uses one semaphore.
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include  <signal.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/* 
 * POSIX thread functions do not return error numbers in errno,
 * but in the actual return value of the function call instead.
 * This macro helps with error reporting in this case.
 */
#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

int NTHREADS;
sem_t NUM;

/*
 * Function for usage of the executable from pthread-test.c 
 */
void usage(char *argv0)
{
        fprintf(stderr, "Usage: %s NTHREADS\n\n"
                        "Exactly one argument required:\n"
                        "    NTHREADS: The number of threads to create.\n",
                        argv0);
        exit(1);
}

/*
 * Function for safe atoi from pthread-test.c 
 */
int safe_atoi(char *s, int *val)
{
        long l;
        char *endp;

        l = strtol(s, &endp, 10);
        if (s != endp && *endp == '\0') {
                *val = l;
                return 0;
        } else
                return -1;
}


/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
        /*
         * x and y traverse the complex plane.
         */
        double x, y;

        int n;
        int val;

        /* Find out the y value corresponding to this line */
        y = ymax - ystep * line;

        /* and iterate for all points on this line */
        for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

                /* Compute the point's color value */
                val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
                if (val > 255)
                        val = 255;

                /* And store it in the color_val[] array */
                val = xterm_color(val);
                color_val[n] = val;
        }
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
        int i;

        char point ='@';
        char newline='\n';

        for (i = 0; i < x_chars; i++) {
                /* Set the current color, then output the point */
                set_xterm_color(fd, color_val[i]);
                if (write(fd, &point, 1) != 1) {
                        perror("compute_and_output_mandel_line: write point");
                        exit(1);
                }
        }


        /* Now that the line is done, output a newline character */
        if (write(fd, &newline, 1) != 1) {
                perror("compute_and_output_mandel_line: write newline");
                exit(1);
        }
}


void  INThandler(int sig)
{
	signal(sig, SIG_IGN);

	reset_xterm_color(1);
	
	printf("\nReset colors..\n");

	exit(0);	
}

void *compute_and_output_mandel_lines_via_threads(void *arg)
{
	int i, value;
	volatile int *line_start_of_this_thread = arg;
	int line = *line_start_of_this_thread;
	int fd = 1; // Output is sent to file descriptor '1', i.e., standard output.
	int color_val[x_chars]; // A temporary array, used to hold color values for the line being drawn

	for (i = line; i < y_chars; i += NTHREADS) {
		compute_mandel_line(i, color_val);

		// Check if it's the correct line to print
		while(1) {
			sem_getvalue(&NUM,&value);
			if (value == i) {
				break;
			}
		}
		output_mandel_line(fd, color_val);

		sem_post(&NUM);
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	int ret, i;

	if (argc != 2)
		usage(argv[0]);

	if (safe_atoi(argv[1], &NTHREADS) < 0 || NTHREADS <= 0) {
		fprintf(stderr, "`%s' is not valid for `NTHREADS'\n", argv[1]);
		exit(1);
	}


	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;

	// Used to catch Ctrl + C
	signal(SIGINT, INThandler);


	// Create the threads and their starting line
	pthread_t t[NTHREADS];
	int starting_line_of_thread[NTHREADS];

	for(i = 0; i < NTHREADS; i++) {
		starting_line_of_thread[i] = i;
	} 

	// Initialize the semaphore
	sem_init(&NUM, 0, 0); //line to print

	for(i = 0; i < NTHREADS; i++) {
		ret = pthread_create(&(t[i]), NULL, compute_and_output_mandel_lines_via_threads, &starting_line_of_thread[i]);
		if (ret) {
			perror_pthread(ret, "pthread_create");
			exit(1);
		}
	}

	// Wait all threads to terminate
	for(i = 0; i < NTHREADS; i++) {
		ret = pthread_join(t[i], NULL);
		if (ret)
			perror_pthread(ret, "pthread_join");
	}

	// Destroy the semaphore
	sem_destroy(&NUM);

	reset_xterm_color(1);
	return 0;
}
