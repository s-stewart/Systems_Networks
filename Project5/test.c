#include <stdio.h>
#include <stdlib.h>
#include "math.h"

int main(){
	int C=10,S=2,B=3;		 //1KB cache with 8byte blocks = 2 words/block and 4-way set associative
	int cache_rows = pow(2.0,(double)C-B-S);	 //cache_rows = 10-5 = 5
	int tag_length = 32 - cache_rows -B;		//32b - 5 - 3 = 24b tag
	int num_sets = pow(2.0,S);
	

	typedef struct line{
		int dirty = 0;
		int valid = 0;
		int tag;
	}line_t;
	
	typedef struct way{
		line_t line[pow(2.0,(double)cache_rows)];	//array of lines = 1 way of cache
	}way_t;

	way_t *cache = (way_t*) malloc(sizeof(way_t)*(S+1));

}