#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}


uint64
sys_getreadcount(void)
{
  return count_read;
}



uint64
sys_hello(void)
{
  printf("Hello World\n");
  return 1;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}


uint64
sys_set_priority(void)
{
  int pid;
  int new_priority;
  // struct proc* p=myproc();
  // int old=p->stat_pr;
  argint(0,&pid);
  argint(1,&new_priority);
  // printf("eibne\n");
  // p->stat_pr=new_priority;
  // p->rbi=25;
  // int old=-1;
  // int brkr=0;
  // #ifdef PBS
  // struct proc* p;
  // for(p=proc;p<&proc[NPROC];p++)
  // {
  //   acquire(&p->lock);
  //   if(p->pid==pid)
  //   {
  //     old=p->stat_pr;
  //     p->stat_pr=new_priority;
  //     p->rbi=25;
  //     brkr=1;
  //     release(&p->lock);
  //     break;
  //   }
  //   release(&p->lock);
  // }
  // #endif
  // if(brkr)
  // {
  //   if(new_priority < old)
  // {
  //   yield();
  // }
  // }
  // return old;
  int call=set_priority(pid,new_priority);
  return call;
}


uint64
sys_sigreturn(void)
{
  struct proc* p=myproc();
  memmove(p->trapframe,p->alarm_tf,PGSIZE);
  // kfree(p->alarm_tf);
  p->alarm_on=0;
  usertrapret();
  return 0;
}

uint64
sys_sigalarm(void)
{

  int n;
  struct proc* p=myproc();
  argint(0,&n);
  p->alarm_interval=n;
  uint64 addres;
  argaddr(1,&addres);
  p->handler=addres;
  // argaddr()
  p->alarm_on=0;
  p->ticks_done=0;
  return 0;
  //  sigalarm(n,)
}