#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include <console.h>
#include "threads/init.h"
#include <syscall.h>
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);
static struct lock file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf("SYstem, %d\n", *((int *)(f->esp)));
  switch(*((int *)(f->esp))){
    case SYS_HALT:{
      power_off();
      break;}
    case SYS_EXIT:{
      int status = *((int *)(f->esp)+1);
      struct child* chd;
      chd=thread_current()->child;
      chd->ret =status;
      chd->exit_called =1;
      thread_exit();
      break;}
    case SYS_EXEC:{
      const char *cmd_line = *((char **)(f->esp)+1);
      tid_t pid = process_execute(cmd_line);
      f->eax = pid;
      break;
    }
    case SYS_WAIT:{
      printf("%d\n",*((tid_t *)(f->esp)+1));
      f->eax =  process_wait((tid_t)*((int *)(f->esp)+1));
    }
    case SYS_CREATE:{
      const char *file = *((char **)(f->esp)+1);
      unsigned initial_size = *((unsigned *)(f->esp)+2);
      break;}
    case SYS_REMOVE:{
      const char *file = *((char **)(f->esp)+1);
      break;}
    case SYS_OPEN:{
      const char *file = *((char **)(f->esp)+1);
      break;}
    case SYS_FILESIZE:{
      int fd = *((int *)(f->esp)+1); 
      break;}
    case SYS_READ:{
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);
      break;}
    case SYS_WRITE:{
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);
      if(fd==STDOUT_FILENO){
        putbuf(buffer, size);
        return size;
      }
      break;}
    case SYS_SEEK:{
      int fd = *((int *)(f->esp)+1);
      unsigned position = *((unsigned *)(f->esp)+2);
      break;}
    case SYS_TELL:{
      int fd = *((int *)(f->esp)+1);
      break;}
    case SYS_CLOSE:{
      int fd = *((int *)(f->esp)+1);
      break;}
  }
}

