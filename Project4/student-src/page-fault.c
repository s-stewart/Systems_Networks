#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "process.h"
#include "global.h"
#include "swapfile.h"
#include "tlb.h"

/*******************************************************************************
 * Page fault handler. When the CPU encounters an invalid address mapping in a
 * process' page table, it invokes the CPU via this handler. The OS then
 * allocates a physical frame for the requested page (either by using a free
 * frame or evicting one), changes the process' page table to reflect the
 * mapping and then restarts the interrupted process.
 *
 * @param vpn The virtual page number requested.
 * @param write If the CPU is writing to the page, this is 1. Otherwise, it's 0.
 * @return The physical frame the OS has mapped to the virtual page.
 */
pfn_t pagefault_handler(vpn_t request_vpn, int write) {
  pfn_t victim_pfn;
  vpn_t victim_vpn;
  pcb_t *victim_pcb;
  pte_t *pagetable;
  pte_t *page;
  int pid;
  pfn_t pfn;
  
  /* Sanity Check */
  assert(current_pagetable != NULL);
  
  /* Find a free frame */
  victim_pfn = get_free_frame();
  assert(victim_pfn < CPU_NUM_FRAMES); /* make sure the victim_pfn is valid */

  /* Use the reverse lookup table to find the victim. */
  victim_vpn = rlt[victim_pfn].vpn;
  victim_pcb = rlt[victim_pfn].pcb;

  /* 
   * FIX ME : Problem 4
   * If victim page is occupied:
   *
   * 1) If it's dirty, save it to disk with page_to_disk()
   * 2) Invalidate the page's entry in the victim's page table.
   * 3) Clear the victim page's TLB entry (hint: tlb_clearone()).
   */
   
   pagetable = victim_pcb->pagetable;
   page = &pagetable[victim_vpn];
   victim_pfn = page->pfn;
   pid = victim_pcb->pid;
	if	(page->dirty)
		page_to_disk(victim_pfn,victim_vpn,pid);
	page->valid = 0;
	tlb_clearone(victim_vpn);

  printf("PAGE FAULT (VPN %u), evicting (PFN %u VPN %u)\n", request_vpn,
      victim_pfn, victim_vpn);

  /* Update the reverse lookup table so that 
     it knows about the requesting process  */
  /* FIX ME */
  
  pfn = current_pagetable[request_vpn].pfn;
  rlt[pfn].pcb = current;
  rlt[pfn].vpn = request_vpn;

  /* Update the requesting process' page table */
  /* FIX ME */
  
	current_pagetable[request_vpn].valid = 1;
	current_pagetable[request_vpn].dirty = 0;
	current_pagetable[request_vpn].used = 1;

  /*
   * Retreive the page from disk. Note that is really a lie: we save pages in
   * memory (since doing file I/O for this simulation would be annoying and
   * wouldn't add that much to the learning). Also, if the page technically
   * does't exist yet (i.e., the page has never been accessed yet, we return a
   * blank page. Real systems would check for invalid pages and possibly read
   * stuff like code pages from disk. For purposes of this simulation, we won't
   * worry about that. =)
   */
  page_from_disk(victim_pfn, request_vpn, current->pid);

  return victim_pfn;
}

