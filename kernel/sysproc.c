#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "fs.h"
#include "proc.h"
#include "fcntl.h"
#include "sleeplock.h"
#include "file.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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

  if(argint(0, &pid) < 0)
    return -1;
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
sys_mmap(void)
{
  uint64 addr;
  uint length, offset;
  int prot, flags, fd, i;
  struct file *file;
  struct mapinfo *mp;
  struct proc *p = myproc();

  if(argaddr(0, &addr) < 0 || argint(1, (int*)&length) < 0 ||
      argint(2, &prot) < 0 || argint(3, &flags) < 0 ||
      argint(4, &fd) < 0 || argint(5, (int *)&offset) < 0)
    return -1;

  // assumption addr and offset are both zero
  if(addr != 0 || offset != 0)
    return -1;
  
  if((file = p->ofile[fd]) == 0 || file->type != FD_INODE)
    return -1;
  
  if((flags & MAP_SHARED) && (prot & PROT_WRITE) && !file->writable)
    return -1;

  for(i = 0; i < NMAPS; i++) {
    if(!p->mps[i].used) {
      mp = &p->mps[i];
      goto next;
    }
  }
  return -1;
  
next:
  mp->addr = MAPADDR(i);
  mp->len = length;
  mp->prot = prot;
  mp->flags = flags;
  mp->offset = offset;
  mp->file = file;
  mp->used = 1;

  filedup(mp->file);
  return mp->addr;
}

uint64
sys_munmap(void)
{
  uint64 addr, pa;
  uint length, i;
  pte_t *pte;
  struct mapinfo *mp;
  struct file *file;
  struct proc *p = myproc();

  if(argaddr(0, &addr) < 0 || argint(1, (int *)&length) < 0)
    return -1;

  if(addr < MAPBASE || addr >= MAPEND)
    return -1;
  mp = &p->mps[(addr - MAPBASE) / MAPSIZE];
  if(!mp->used || addr + length > mp->addr + mp->len)
    return -1;
  
  file = mp->file;
  for(i = PGROUNDDOWN(addr); i < addr + length; i += PGSIZE) {
    if(!(pte = walk(p->pagetable, i, 0)) || !(*pte & PTE_V))
      continue;
    pa = PTE2PA(*pte);
    if((mp->flags & MAP_SHARED) && (*pte & PTE_D)) {
      // write back to disk
      uint fstart = i - mp->addr + mp->offset;
      uint wsize = PGSIZE;
      if(fstart + wsize >= file->ip->size)
        wsize = file->ip->size - fstart;
      
      begin_op(file->ip->dev);
      ilock(file->ip);
      writei(file->ip, 0, pa, fstart, wsize);
      iunlock(file->ip);
      end_op(file->ip->dev);
    } 
    uvmunmap(p->pagetable, i, PGSIZE, 0);
    kfree((void *)pa);
  }

  if(addr == mp->addr && length == mp->len) {
    fileclose(mp->file);
    mp->used = 0;
  } else if(addr == mp->addr) {
    mp->addr = addr + length;
  } else if(addr + length == mp->addr + mp->len) {
    mp->len = addr - mp->addr;
  }
  
  return 0;
}