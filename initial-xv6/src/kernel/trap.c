#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#define AGING 30
struct spinlock tickslock;
uint ticks;

#ifdef MLFQ
extern struct Queue q[4];
#endif

extern char trampoline[], uservec[], userret[];
// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
  initlock(&tickslock, "time");
}


int check_validity(uint64 pte_v_address)
{
  if(pte_v_address >= MAXVA)
  return 0;
  else if(pte_v_address < PGROUNDDOWN(myproc()->trapframe->sp) && pte_v_address >= PGROUNDDOWN((myproc()->trapframe->sp - PGSIZE)))
  return 0;
  return 1;
}

int handle_pgflt(pagetable_t pgtbl, uint64 va)
{

  int ch=check_validity((uint64)va);
  if(ch)
  {
    
    pte_t *pte = walk(pgtbl, (uint64)va, 0);
    // uint64 pa;
    uint fl;
    if (pte && PTE2PA(*pte))
    {      
        fl = PTE_FLAGS(*pte);
        if (fl & PTE_COW)
        {
          fl &= (~PTE_COW);    
          fl = (fl | PTE_W);
          char *copy_mem = 0;
          if((copy_mem=kalloc())!=0)
          {
            memmove(copy_mem, (void *)PTE2PA(*pte), PGSIZE);
            kfree((void *)PTE2PA(*pte));
            *pte=0;
            // *pte = PA2PTE(copy_mem) | fl;
            if(mappages(myproc()->pagetable,PGROUNDDOWN((uint64)va),PGSIZE,(uint64)copy_mem,fl)!=0)
            {
              myproc()->killed=1;
              return 0;
            }
          }
          else
          {
            return 0;
          }
        }
    }
    else
    {
      return 0;
    }
  
  }
  else
  {
    return ch;
  }
  
  return 1;
}


// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void)
{
  int which_dev = 0;

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();

  // save user program counter.
  p->trapframe->epc = r_sepc();

  if (r_scause() == 8)
  {
    // system call

    // myproc()->ticks_done++;
    if (killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  }

  else if(r_scause() == 15 )
  {
    uint64 pte_v_address=r_stval();  
    if(!pte_v_address)
    p->killed=1;
      if(!p->killed)
    {  
    if(handle_pgflt(p->pagetable, (uint64)pte_v_address)==0)
    {
      p->killed=1;
      printf("Page not found\n");
    }
    
    }
    // }

  }

  else if ((which_dev = devintr()) != 0)
  {
    // ok
  }
  else
  {

    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if (killed(p))
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2)
  { 
    
    if (p != 0)
    {
      // p->alarm_on=1;
      // struct trapframe *tf=0;
      p->ticks_done++;
      if(!p->alarm_on&&p->alarm_interval>0)
      {// printf("%lld\n",(ticks-p->ticks_done));
      if (p->ticks_done >= p->alarm_interval)
      {
        p->ticks_done = 0;
        p->alarm_on = 1;
        // p->ticks_done=ticks;
        *(p->alarm_tf) = *(p->trapframe);
        // memmove(p->alarm_tf, p->trapframe, PGSIZE);
        // p->alarm_tf = tf;
        p->trapframe->epc = p->handler;
        
      }
      
      }
    }


  #ifdef MLFQ


  if(myproc()!=0 && myproc()->state == RUNNING)
  {
  


  struct proc* p;



  for (p = proc; p < &proc[NPROC]; p++)
    {
      if (p->state == RUNNABLE)
      {
          p->w++;
        // int diff_ticks = ticks - p->time_ticks_covered;
        if (p->w >= AGING)
        {
          if (p->queue_num > 0)
          {
            acquire(&p->lock);
            p->wait = 0;
            p->w=0;
            p->queue_num--;
            p->time_ticks_covered = ticks;
            // p->wait=0;
            release(&p->lock);
          }
        }
        // else
        {
        }
      }
      else
      p->w=0;
    }

  p=myproc();
  p->wait++;
  // for(struct proc* other=proc;other<&proc[NPROC];other++)
  // {
  //   if(other!=0&&other->state==RUNNABLE)
  //   other->w++;
  // }
  // int diff_ticks=ticks-p->time_ticks_covered;

  if(p->wait>=q[p->queue_num].time_slice)
  {
      if(p->queue_num<3)
      {
        p->queue_num++;
        p->time_ticks_covered=ticks;
        p->wait=0;
        p->w=0;
        yield();
      }
      else
      {
        p->time_ticks_covered=ticks;
        p->wait=0;
        p->w=0;
        yield();
      }
  }
  // int f1=0;
  for(int i=0;i<p->queue_num;i++)
  {
      struct proc* pre;
      // int f2=0;
      for(pre=proc;pre<&proc[NPROC];pre++)
      {
        if(pre->queue_num==i&&pre->state==RUNNABLE)
        {
          // f2=1;
          yield();
        }
      }
  }
  // yield();
  // if(myproc()!=0&&myproc()->state==RUNNING)
  // { 
  //   for(int j=0;j<p->queue_num;j++)
  // {
  //   // if(q[j].size)
  //   yield();  
  // }

  }

#endif
  if(myproc()!=0&&myproc()->state==RUNNING)
  {
  #ifdef RR
  yield();
  #endif
  }

  #ifdef PBS
  yield();
  #endif

    // if (myproc() != 0 && myproc()->state == RUNNING)
    // {
    //   // yield();
    //   #ifndef FCFS
    //     yield();
    //   #endif
    // }


    // if(SCHDLR==0)
  }
  

  // #endif

  usertrapret();
}



//
// return to user space
//
void usertrapret(void)
{

  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if ((which_dev = devintr()) == 0)
  {
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

if(which_dev==2)
{
  #ifdef MLFQ

  if(myproc()!=0 && myproc()->state == RUNNING)
  {
   
  struct proc* p;

  for (p = proc; p < &proc[NPROC]; p++)
    {
      if (p->state == RUNNABLE)
      {
        p->w++;
        // int diff_ticks = ticks - p->time_ticks_covered;
        if (p->w >= AGING)
        {
          if (p->queue_num > 0)
          {
            acquire(&p->lock);
            p->wait = 0;
            p->w=0;
            p->queue_num--;
            p->time_ticks_covered = ticks;
            // p->wait=0;
            release(&p->lock);
          }
        }
      }
    }

  p=myproc();
        p->wait++;
  // int diff_ticks=ticks-p->time_ticks_covered;
  if(p->wait>=q[p->queue_num].time_slice)
  {
      if(p->queue_num<3)
      {
        p->queue_num++;
        p->time_ticks_covered=ticks;
        p->w=0;
        yield();
      }
      else
      {
        p->time_ticks_covered=ticks;
        p->w=0;
        yield();
      }
  }
  for(int i=0;i<p->queue_num;i++)
  {
      struct proc* pre;
      // int f2=0;
      for(pre=proc;pre<&proc[NPROC];pre++)
      {
        if(pre->queue_num==i&&pre->state==RUNNABLE)
        {
          // f2=1;
          yield();
        }
      }
  }
  // yield();
  // if(myproc()!=0&&myproc()->state==RUNNING)
  // { 
  //   for(int j=0;j<p->queue_num;j++)
  // {
  //   // if(q[j].size)
  //   yield();  
  // }
  
  }
#endif
  if(myproc()!=0&&myproc()->state==RUNNING)
  {
  #ifndef MLFQ
  yield();
  #endif
  }
}  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void clockintr()
{
  acquire(&tickslock);
  ticks++;
  update_time();
  // for (struct proc *p = proc; p < &proc[NPROC]; p++)
  // {
  //   acquire(&p->lock);
  //   if (p->state == RUNNING)
  //   {
  //     printf("here");
  //     p->rtime++;
  //   }
  //   // if (p->state == SLEEPING)
  //   // {
  //   //   p->wtime++;
  //   // }
  //   release(&p->lock);
  // }
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr()
{
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) &&
      (scause & 0xff) == 9)
  {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ)
    {
      uartintr();
    }
    else if (irq == VIRTIO0_IRQ)
    {
      virtio_disk_intr();
    }
    else if (irq)
    {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  }
  else if (scause == 0x8000000000000001L)
  {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if (cpuid() == 0)
    {
      clockintr();
    }

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  }
  else
  {
    return 0;
  }
}
