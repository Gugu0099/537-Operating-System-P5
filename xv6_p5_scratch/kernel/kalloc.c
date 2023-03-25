// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"


struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;

  /*
  P5 changes
  */
  uint free_pages; //track free pages
  uint ref_cnt[PHYSTOP / PGSIZE]; //track reference count
   

} kmem;

extern char end[]; // first address after kernel loaded from ELF file

void incrementref(char *adr) {
  acquire(&kmem.lock);
  kmem.ref_cnt[PADDR(adr)/PGSIZE]++;
  release(&kmem.lock);
}

void decrementref(char *adr) {
  acquire(&kmem.lock);
  kmem.ref_cnt[PADDR(adr)/PGSIZE]--;
  release(&kmem.lock);
}

uint getref(char *adr){
  return kmem.ref_cnt[PADDR(adr)/PGSIZE]; //TODO add to defs.h
}

// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;

  initlock(&kmem.lock, "kmem");
  // for (int i = 0; i < (PHYSTOP / PGSIZE); i+=1) {
  //   kmem.ref_cnt[i] = 0;
  // }
  kmem.free_pages = 0;
  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE){
    kmem.ref_cnt[(uint)p/PGSIZE] = 0;
    kfree(p);

  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  // memset(v, 1, PGSIZE);

  acquire(&kmem.lock);
  // kmem.freelist = r;
  if (kmem.ref_cnt[PADDR(v)/PGSIZE] > 1)
  {
    kmem.ref_cnt[PADDR(v)/PGSIZE]--;
    // release(&kmem.lock);
    // return;
  } else{
  kmem.ref_cnt[PADDR(v)/PGSIZE] = 0;
  memset(v, 1, PGSIZE);
  
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.free_pages++;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.ref_cnt[PADDR((char*)r)/PGSIZE] = 1;
    kmem.freelist = r->next;
    kmem.free_pages--;
    //incrementref((char*)r); //ask about how this works in the big picture. when a child proc is created, how do we handle since no extra pages would be used
  }
  release(&kmem.lock);
  return (char*)r;
}

int
getFreePagesCount(void)
{
  return kmem.free_pages;
}

