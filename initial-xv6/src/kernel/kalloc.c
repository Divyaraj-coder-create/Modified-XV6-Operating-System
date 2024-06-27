// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;


struct spinlock lck;
int counter[PGROUNDUP(PHYSTOP)>>PGSHIFT];


void incr(void* pa)
{
  acquire(&lck);
  // printf("od\n");
  ++counter[((uint64)pa) >> PGSHIFT];
  release(&lck);
  return;
  // return ret;
}




void
kinit()
{
  // printf("here1\n");
  initlock(&kmem.lock, "kmem");
  initlock(&lck,"cntlock");
  acquire(&lck);
  for(int i=0;i<(PGROUNDUP(PHYSTOP)>>PGSHIFT);i++)
  {
    counter[i]=0;
  }
  release(&lck);
  // printf("here2\n");
  freerange(end, (void*)PHYSTOP);
  return;
}




void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {  
    // printf("eb\n");
    incr((void*)p);
    kfree(p);
  
  }
}
// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");



  acquire(&lck);
  // printf("kn\n");
  int cnt= --counter[((uint64)(pa) >> 12)];
  release(&lck);
  if(cnt==0)
  {
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);

  }

  else
  return;

  // Fill with junk to catch dangling refs.
  
}// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 100, PGSIZE); 
    // printf("bn\n");
    incr((void*)r); 
  }// fill with junk
  return (void*)r;
}