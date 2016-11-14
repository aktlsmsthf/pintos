#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include <console.h>
#include "threads/init.h"
#include <syscall.h>
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <list.h>
#include "userprog/pagedir.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include <string.h>
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"


static void syscall_handler (struct intr_frame *);
void exit(int);
struct file* get_file_from_fd(int);
struct list_elem* get_elem_from_fd(int);
void check_valid_ptr(void*, void*);
void check_valid_buffer(void*, void*, unsigned);
//bool user_memory(void *, int);
//bool check_buffer(void *, unsigned);
//bool check_bad_ptr(const void * uaddr);
//bool check_bad_ptr(struct intr_frame *f ,const void * uaddr);
//void check_valid_ptr(void *, const void *);

struct lock sys_lock;
int a=0;
void
syscall_init (void) 
{  
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&sys_lock);
}
static void
syscall_handler (struct intr_frame *f UNUSED) 
{  
  check_valid(f->esp, f->esp);
  switch(*((int *)(f->esp))){
    case SYS_HALT:{
      power_off();
      break;}
      
    case SYS_EXIT:{
      check_valid(f->esp, f->esp+1);
    
      int status = *((int *)(f->esp)+1);
      exit(status);
      break;
    }
      
    case SYS_EXEC:{
      check_valid(f->esp, f->esp+1);
      const char * cmd_line = *((char **)(f->esp)+1);
      check_valid(f->esp, cmd_line);
      
      /**lock_acquire(&sys_lock);**/
      tid_t pid = process_execute(cmd_line);
      /**lock_release(&sys_lock);**/
      f->eax = pid;
      break;
    }
      
    case SYS_WAIT:{
      check_valid(f->esp, f->esp+1);
     
      f->eax =  process_wait((tid_t)*((int *)(f->esp)+1));
      break;
    }
      
    case SYS_CREATE:{
      check_valid(f->esp, f->esp+2);
      
      const char *file = *((char **)(f->esp)+1);
      unsigned initial_size = *((unsigned *)(f->esp)+2);
      
      check_valid(f->esp, file);
      
      if(file==NULL){
        f->eax =-1;
      }
      else{
        lock_acquire(&sys_lock);
        f->eax = filesys_create (file,initial_size);
        lock_release(&sys_lock);
      }
      break;
    }
      
    case SYS_REMOVE:{
      check_valid(f->esp, f->esp+1);
      
      const char *file = *((char **)(f->esp)+1);
      check_valid_ptr(f->esp, file);
      lock_acquire(&sys_lock);
      f->eax = filesys_remove (file);
      lock_release(&sys_lock);
      break;
     }
      
    case SYS_OPEN:{
      check_valid(f->esp, f->esp+1);
      
      const char *name= *((char **)(f->esp)+1);
      
      check_valid_ptr(f->esp, name);

        char *e = "";
        if(name == NULL || strcmp(name, e)==0) {
          f->eax = -1;
        }
        else{
          lock_acquire(&sys_lock);
          struct file *ff = filesys_open(name);
          
          if(ff==NULL) {
            f->eax = -1;
          }
          else{
            struct thread *t = thread_current();
            struct file_fd *ffd = palloc_get_page(0);
            if(ffd==NULL){
              f->eax =-1;
            }
            else{
              ffd -> fd = t->num_file+2;
              ffd -> file = ff;
              ffd -> is_closed=0;
              list_push_front(&(t->file_list),&ffd->elem);
              t->num_file++;
              f->eax = ffd->fd;
            }
          }
          lock_release(&sys_lock);
        
      }
      break;
     }
      
    case SYS_FILESIZE:{
      check_valid(f->esp, f->esp+1);
      int fd = *((int *)(f->esp)+1);
      struct file * ff=get_file_from_fd(fd);
      f->eax = (int) file_length(ff);
      break;
     }
      
    case SYS_READ:{
      check_valid(f->esp, f->esp +3);
      
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);
      
      check_valid(f->esp, buffer);
      check_valid_buffer(f->esp, buffer, size);
      int j=0;
      if(fd == 0){
        for(; j<size; j++){
          *(uint8_t *)(buffer+j)=input_getc();
        }
        f->eax = size;
      }
      else if(fd ==1){
        f->eax =-1;
      }
      else{
        struct file * ff = get_file_from_fd(fd);
        if(ff==NULL){
          f->eax = -1;
        }
        else{
          lock_acquire(&sys_lock);
          int r = (int) file_read(ff, buffer, size);
          lock_release(&sys_lock);
          f->eax = r;
        }
      }
      break;
    }
      
    case SYS_WRITE:{  
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);
      check_valid(f->esp, buffer);
      check_valid_buffer(f->esp, buffer, size);
      if(fd==1){
        putbuf(buffer, size);
        f->eax= size;
      }
      else if(fd == 0){
        f->eax = -1;
      }
      else{
        check_valid(f->esp, f->esp+3);
        struct file *ff = get_file_from_fd(fd);
        if(ff==NULL){ 
          
          f->eax = -1;
        }
        else{
          lock_acquire(&sys_lock);
          int r = (int) file_write(ff, buffer, size);
          lock_release(&sys_lock);
          f->eax = r;
        }
      }
      break;
    }
      
    case SYS_SEEK:{
      check_valid(f->esp, f->esp+2);
      int fd = *((int *)(f->esp)+1);
      unsigned position = *((unsigned *)(f->esp)+2);
      struct file *ff = get_file_from_fd(fd);
      lock_acquire(&sys_lock);
      file_seek(ff, position);
      lock_release(&sys_lock);
      break;
    }
      
    case SYS_TELL:{
      check_valid(f->esp, f->esp+1);
      int fd = *((int *)(f->esp)+1);
      struct file *ff = get_file_from_fd(fd);
      if(ff==NULL){ 
        f->eax = -1;
      }
      else{
        lock_acquire(&sys_lock);
        f->eax = file_tell(ff);
        lock_release(&sys_lock);
      }
      break;
    }
      
    case SYS_CLOSE:{
      check_valid(f->esp, f->esp+1);
      int fd = *((int *)(f->esp)+1);
      if(fd>1){
        struct list_elem *flm = get_elem_from_fd(fd);
        struct file_fd *ffd = list_entry(flm, struct file_fd, elem);
        
        
        if(ffd->is_closed){
          break;
        }
        if(!ffd->is_closed && ffd->file!=NULL){
          lock_acquire(&sys_lock);
           file_close(ffd->file);
          
          ffd->is_closed=1;
          lock_release(&sys_lock);
        }
      }
      break;
    }
  }
}
 
void exit(int status){
      struct thread * curr=thread_current();
      struct child* chd;
  
      chd=curr->child;
      chd->ret =status;
      chd->exit_called =1;
      
      printf("%s: exit(%d)\n",curr->name,chd->ret);
      thread_exit();
}
 
struct file* get_file_from_fd(int fd){
      struct thread * curr=thread_current();
      if(list_empty(&(curr->file_list))){
         return NULL;
      }
      struct list_elem * felem = list_front(&(thread_current()->file_list));
      struct file_fd * ffd;
      while(list_entry(felem, struct file_fd, elem)->fd != fd){
          felem = felem->next;
          if(felem->next==NULL){
             return NULL;
          }
      }
      ffd = list_entry(felem, struct file_fd, elem);
      return ffd->file;
}
 
struct list_elem* get_elem_from_fd(int fd){
      struct thread * curr=thread_current();
      if(list_empty(&(curr->file_list))){
         return NULL;
      }
      struct list_elem * felem = list_front(&(thread_current()->file_list));
      struct file_fd * ffd;
      while(list_entry(felem, struct file_fd, elem)->fd != fd){
          felem = felem->next;
          if(felem->next==NULL){
             return NULL;
          }
      }
      return felem;
}
 
void check_vaild(void *esp, const void *addr){
  if(!is_user_vaddr(addr)){
    exit(-1);
  }
  struct spt_entry *spte = spte_find(pg_round_down(addr));
  if(spte!=NULL){
    if(spte->fe->in_swap){
      uint8_t *frame = palloc_get_page(PAL_USER);
      if(frame == NULL) {frame = frame_evict();}
      swap_in(spte->fe, frame);
    }
  }
  else{
    if(addr>=esp-32){
      uint8_t *frame = palloc_get_page(PAL_USER);
      frame = frame_spt_alloc(frame, &thread_current() -> spt, pg_round_down(addr), true);
    }
  }
}

void check_valid_buffer(void *esp, void *buffer, unsigned size){
  check_valid_ptr(esp, buffer);
  char * b = (char *) buffer;
  int i;
  for(i=0; i<size; i++){
    check_valid(esp, b);
    b++;
  }
}
