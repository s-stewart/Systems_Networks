#include <stdlib.h>

#include "types.h"
#include "pagetable.h"
#include "global.h"
#include "process.h"

/*******************************************************************************
 * Finds a free physical frame. If none are available, uses a clock sweep
 * algorithm to find a used frame for eviction.
 *
 * @return The physical frame number of a free (or evictable) frame.
 */
pfn_t get_free_frame(void) {
   int i;

   /* See if there are any free frames */
   for (i = 0; i < CPU_NUM_FRAMES; i++){
      if (rlt[i].pcb == NULL)
         return i;
	}
   /* FIX ME : Problem 5 */
   /* IMPLEMENT A CLOCK SWEEP ALGORITHM HERE */
	for (i = 0; i < CPU_NUM_FRAMES; i++){
			pcb_t *potential = rlt[i].pcb;
			vpn_t vpn = rlt[i].vpn;
			pte_t *pagetable = potential->pagetable;
			pte_t *page = &pagetable[vpn];
			if (page->used == 1)
				page->used = 0;
			else 
			return i;
	}
   /* If all else fails, return a random frame */
   return rand() % CPU_NUM_FRAMES;
}
