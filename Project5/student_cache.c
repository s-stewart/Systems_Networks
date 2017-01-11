#include "student_cache.h"
#include "math.h"


int student_read(address_t addr, student_cache_t *cache, stat_t *stats){

	int r=0,a,b,hit=0;
	int B=cache[1].B;
	int C=cache[1].C;
	int S=cache[1].S;
	a = B;
	b = (C-B-S) +B;
	stats->accesses++;
	stats->reads++;
	unsigned i;

	for (i=a; i<=b; i++){
     	    r |= 1 << i;
	}

	int index = addr & r; 
	int tag = addr / ((2<<(C-B-S))+(2<<B));  //(2<<C-B-S) = index bit  2<<B = block offset
	
	//check to see if the tag is in the cache 
	block_t *curr = cache[index].block->next;				//curr = first block
	block_t *prev_curr = NULL;
	while(curr != NULL){									//while there are blocks in the set
		if(tag == cache[index].block->tag){				//check the tag of the block
			hit = 1;										//its a hit if the tags match
			stats->read_hit++;
			prev_curr->next = curr->next;					
			cache[index].block->next = curr;				//make that the new head of the set
		}
		prev_curr = curr;
		curr = curr->next;
	}
	
	if (hit ==0){
		stats->read_miss++;
		stats->mem_read_bytes += 4;
		if ( cache[index].block->next == NULL ){				//If nothing is in the set
			block_t *block = (block_t*)malloc(sizeof(block_t));	//make a block
			block->next = NULL;									//it points to nothing
			cache[index].block->next = block;					//it is now the first block in the set
			cache[index].block->dirty = 0;
			cache[index].block->valid = 1;
			cache[index].block->tag = tag;
			cache[index].num_in_set++;
		}
	
	
		else if ( ((cache[index].S)+1) > cache[index].num_in_set ){	//set is not quite full
			block_t *block = (block_t*)malloc(sizeof(block_t));			//make a block
			block->next = cache[index].block->next;					//make the new block's next point to the old first
			cache[index].block->next = block;							//make the new block the first block
			cache[index].block->dirty = 0;
			cache[index].block->valid = 1;
			cache[index].block->tag = tag;
			cache[index].num_in_set++;
		}
	
		else {
			block_t *temp = cache[index].block->next;					//temp = old head
			block_t *block = (block_t*)malloc(sizeof(block_t));			//make a block
			block->next = cache[index].block->next;					//make the new block's next point to the old first
			cache[index].block->next = block;							//make the new block the new first block
			while(temp->next){
				temp = temp->next;										
			}															//temp = last element in list
			if (temp->dirty == 1)
				stats->mem_write_bytes += 4;
			free(temp);													//evict the LRU
			cache[index].block->dirty = 0;
			cache[index].block->valid = 1;
			cache[index].block->tag = tag;
		}
	}	
	
	if (hit == 1)
		return hit;
	else 
		return 0;
}

int student_write(address_t addr, student_cache_t *cache, stat_t *stats){

	int r=0,a,b,hit=0;
	int B=cache[1].B;
	int C=cache[1].C;
	int S=cache[1].S;
	unsigned i;
	a = B;
	b = (2<<(C-B-S)) +B;
	
	stats->writes++;
	stats->accesses++;	

	for (i=a; i<=b; i++){
       		r |= 1 << i;
	}

	int index = addr & r; 
	int tag = addr / ((2<<(C-B-S))+(2<<B));  //(2<<C-B-S) = index bit  2<<B = block offset
	
	if(cache[index].WP==WTWNA){									////////WRITE THROUGH////////
	//writes do nothing but report
		block_t *curr = cache[index].block->next;				//curr = first block
		while(curr->next != NULL){								//search through the list
			if(cache[index].block->tag == tag){				//if the tag matches, its a hit
				stats->write_hit++;
				curr->dirty = 1;
				hit = 1;
				break;
			}
			curr = curr->next;
		}
		stats->write_miss++;									//if no hit, its a miss	
	}
	
	
	
	
	else {												//////WRITE BACK///////
	//write misses must retrieve (Same as read miss - put it into cache & change dirty)
	//write hits just change dirty.
		block_t *curr = cache[index].block->next;				//curr = first block
		while(curr->next != NULL){								//search through the list
			if(cache[index].block->tag == tag){				//write hit if the tag matches
				stats->write_hit++;
				curr->dirty = 1;
				hit = 1;
				break;
			}
			curr = curr->next;
		}
}
		if (hit ==0){
		
			stats->write_miss++;									//if at the end of the set and no hit, its a miss
			stats->mem_read_bytes += 4;								//must read from mem.
			
			if ( cache[index].block->next == NULL ){				//If nothing is in the set
				block_t *block = (block_t*)malloc(sizeof(block_t));	//make a block
				block->next = NULL;									//it points to nothing
				cache[index].block->next = block;					//it is now the first block in the set
				cache[index].block->dirty = 1;
				cache[index].block->valid = 1;
				cache[index].block->tag = tag;
				cache[index].num_in_set++;
			}
	
	
			else if ( ((cache[index].S)+1) > cache[index].num_in_set ){	//set is not quite full
				block_t *block = (block_t*)malloc(sizeof(block_t));			//make a block
				block->next = cache[index].block->next;					//make the new block's next point to the old first
				cache[index].block->next = block;							//make the new block the first block
				cache[index].block->dirty = 1;
				cache[index].block->valid = 1;
				cache[index].block->tag = tag;
				cache[index].num_in_set++;
			}
	
			else {	//if the line is full
				block_t *temp = cache[index].block->next;					//temp = old head
				block_t *block = (block_t*)malloc(sizeof(block_t));			//make a block
				block->next = cache[index].block->next;					//make the new block's next point to the old first
				cache[index].block->next = block;							//make the new block the new first block
				while(temp->next){
					temp = temp->next;										
				}															//temp = last element in list
				if (temp->dirty == 1)
					stats->mem_write_bytes += 4;									//if LRU is dirty, write it back
				free(temp);													//evict the LRU
				cache[index].block->dirty = 1;
				cache[index].block->valid = 1;
				cache[index].block->tag = tag;
			}
		}	//must retrive the block and put it on the list(read miss + change dirty)

	if(hit==1){
		return hit;
	}
	
	else{ 
		return 0;
	}
}

student_cache_t *allocate_cache(int C, int B, int S, int WP, stat_t* stats){
/*You will start by designing a data structure that will hold all meta data 
for a cache. This data structure will be dynamically allocated by a function 
called allocate_cache. The function will take in parameters: C, B, S, WP where 
the cache allocated has 2^C bytes of total data storage, 
having 2^B-byte blocks, 
and with sets of 2^S blocks per set
(note that S=0 is a direct-mapped cache, and S = C - B is a fully associative cache). 
The cache implements one of two 
write policies (WP): write through (WTWNA/WTNA) or write back (WBWA) (sometimes 
called copy back). In the case of write back there will be a dirty bit per tag. 
There is a valid bit for each tag entry that specifies whether or not this entry 
in the cache has been accessed. When the simulator starts, all valid bits are 
set to false (0).) The size of an address is 32 bits. Note: The cache data 
structure may contain the values of C, B, S, and WP if you find it necessary.

The allocate_cache function will return a pointer to the cache data structure.*/

	int cache_rows = 2<<(C-B-S);
	int i;

	
	//cache is array of sets
	student_cache_t *cache = (student_cache_t*)malloc(sizeof(student_cache_t)*cache_rows);

	for (i=0;i<cache_rows;i++){
		cache[i].block = (block_t*)malloc(sizeof(block_t));
		cache[i].S = S;
		cache[i].B = B;
		cache[i].C = C;
		cache[i].WP = WP;
		cache[i].rows = cache_rows;
		cache[i].num_in_set = 0;
	}
	
	//initlaize stats
	stats->accesses = 0;      		
	stats->reads = 0; 
	stats->read_miss= 0; 
	stats->read_hit = 0; 
	stats->writes = 0; 
	stats->write_miss = 0; 
	stats->write_hit = 0; 
	stats->mem_write_bytes = 0; 
	stats->mem_read_bytes = 0; 
	stats->misses = 0; 
	stats->read_hit_rate = 0; 
	stats->hit_rate = 0; 
	stats->total_bits = (2 << C) + 2*cache_rows*(S+1)*B;
	
	return cache;
}

void finalize_stats(student_cache_t *cache, stat_t *stats){

	stats->hit_rate = ((stats->read_hit)+(stats->write_hit)) / (stats->accesses);
	stats->read_hit_rate = (stats->read_hit) / (stats->reads);
	
	/*The Actual memory access time (AAT) assuming:

    * a miss penalty of 50 ns, and
    * a hit time of 2+0.2*S ns (where there are 2^S blocks per set).

The average access time equation is:

AAT = Hit time + Read miss rate x Miss penalty */

	stats->AAT = 2+0.2*(cache[1].S) + (1-(stats->read_hit_rate))*50;

	int cache_rows=0,i=0;
	int S = cache[1].S;
	cache_rows = cache[1].rows;
	block_t *curr;
	for(i=0;i<(cache_rows*(S+1));i++){
		curr = cache[i].block->next;
		while (curr != NULL){
			curr = curr->next;
		}
		free(curr);
	}

}


