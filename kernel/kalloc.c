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

struct Reference{
    // 引用计数器
    struct spinlock lock;
    int refcount[PHYSTOP/PGSIZE];
};

struct Reference ref;

struct run {
  struct run *next;
};

int addRefCount(uint64 pa){
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("krefcount");
    acquire(&ref.lock);
    ref.refcount[pa/PGSIZE]++;
    int count = ref.refcount[pa/PGSIZE];
    release(&ref.lock);
    return count;
}

int deleteRefCount(uint64 pa){
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("krefcount");
    acquire(&ref.lock);
    ref.refcount[pa/PGSIZE]--;
    int count = ref.refcount[pa/PGSIZE];
    release(&ref.lock);
    return count;
}

int getRefCount(uint64 pa){
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("krefcount");
    acquire(&ref.lock);
    int count = ref.refcount[pa/PGSIZE];
    release(&ref.lock);
    return count;
}

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock,"ref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
      // 初始化
      acquire(&ref.lock);
      ref.refcount[(uint64)p/PGSIZE] = 1;
      release(&ref.lock);
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

//  int dd = deleteRefCount((uint64)pa);
//    printf("%d \n",dd);
  if (deleteRefCount((uint64)pa) == 0){
      // Fill with junk to catch dangling refs.
      memset(pa, 1, PGSIZE);

      r = (struct run*)pa;

      acquire(&kmem.lock);
      r->next = kmem.freelist;
      kmem.freelist = r;
      release(&kmem.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
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
  if(r) {
      acquire(&ref.lock);
      ref.refcount[(uint64) r/PGSIZE] = 1;
      release(&ref.lock);
      memset((char *) r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}
