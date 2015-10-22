/** @file part1.c */

/*
 * Machine Problem #0
 * CS 241 Fall2013
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "part2-functions.h"

/**
 * (Edit this function to print out the ten "Illinois" lines in part2-functions.c in order.)
 */
int main()
{
	//first step
	first_step(81);

	//second step
	int *value = malloc(sizeof(int));
	*value = 132;	
	second_step(value);
	free(value);

	//double step
	int **valued = malloc(sizeof(int *));
	valued[0] = malloc(sizeof(int));
	*valued[0] = 8942;
	double_step(valued);
	free(valued[0]);
	free(valued);

	//strange step
	int *values = NULL;
	values = 0;
	strange_step(values);
	free(values);

	//empty step
	void *vptr;
	char *cptr = malloc(4);
	cptr[3] = 0;
	vptr = cptr;
	empty_step(vptr);
	free(cptr);

	//two step
	void *vptrs;
	char *cptrs = "uuuuu";
	vptrs = cptrs;
	two_step(vptrs, cptrs);

	//three_step
	char *one = NULL;
	char *two = NULL;
	char *three = NULL;

	two = one + 2;
	three = two + 2;

	three_step(one, two, three);

	//step step step
	char *first = "00";
	char *second = "888";
	char *third = "@@@@";

	step_step_step(first, second, third);

	//it may be odd
	char *a = malloc(sizeof(char));
	*a = (char) 1;
	int b = 1;
	it_may_be_odd(a, b);
	free(a);

	//the end
	char *blue = malloc(4);
	blue[0] = (char) 1;
	blue[1] = (char) 0;
	blue[2] = (char) 1;
	blue[3] = (char) 1;
	void * orange = blue;
	the_end(blue, orange);
	free(blue);
		
	return 0;
}
