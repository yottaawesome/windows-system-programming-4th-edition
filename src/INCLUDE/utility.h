#ifndef __utility_h
#define __utility_h
/*	PROJECT VERSION						*/

/* 
 * Define a macro that delays for an amount of time proportional
 * to the integer parameter. The delay is a CPU delay and does not
 * voluntarily yield the processor. This simulates computation.
 */

#define delay_cpu(n)  {\
	int i=0, j=0;\
	/* Do some wasteful computations that will not be optimized to nothing */\
	while (i < n) {\
		j = i*i + (float)(2*j)/(float)(i+1);\
		i++;\
	}\
}
#endif

