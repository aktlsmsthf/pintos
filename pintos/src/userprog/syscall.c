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

static void syscall_handler (struct intr_frame *);
void exit(int);
struct file* get_file_from_fd(int);
struct list_elem* get_elem_from_fd(int);
void user_memory(void *, int);
void check_buffer(void *, unsigned);
bool check_bad_ptr(uint32_t * pd,const void * uaddr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if(!is_user_vaddr((const void *)f->esp)) exit(-1);
  if(check_bad_ptr(thread_current()->pagedir,f->esp)) exit(-1);
  if(f->esp<0x08048000) exit(-1);
  
  switch(*((int *)(f->esp))){
    case SYS_HALT:{
      power_off();
      break;}
      
    case SYS_EXIT:{
      user_memory(f->esp, 1);
      int status = *((int *)(f->esp)+1);
      exit(status);
      break;}
      
    case SYS_EXEC:{
      user_memory(f->esp, 1);
      
      const char * cmd_line = *((char **)(f->esp)+1);
      user_memory((void *)cmd_line, 0);
      if(check_bad_ptr(thread_current()->pagedir,(const void *)cmd_line))exit(-1);
      tid_t pid = process_execute(cmd_line);
      f->eax = pid;
      break;
    }
      
    case SYS_WAIT:{
      user_memory(f->esp,1);
      f->eax =  process_wait((tid_t)*((int *)(f->esp)+1));
      break;
    }
      
    case SYS_CREATE:{
      user_memory(f->esp,2);
      const char *file = *((char **)(f->esp)+1);
      unsigned initial_size = *((unsigned *)(f->esp)+2);
      
      user_memory((void *)file, 0);
      if(check_bad_ptr(thread_current()->pagedir,(const void *)file))exit(-1);
      if(file==NULL){
        exit(-1);
      }
      else{
        f->eax = filesys_create (file,initial_size);
      }
      break;
      }
      
    case SYS_REMOVE:{
      user_memory(f->esp,1);
      const char *file = *((char **)(f->esp)+1);
      f->eax = filesys_remove (file); 
      break;
     }
      
    case SYS_OPEN:{
      user_memory(f->esp,1);
      const char *name= *((char **)(f->esp)+1);
      
      user_memory((void *)name, 0);
      if(check_bad_ptr(thread_current()->pagedir,(const void *)name))exit(-1);
      char *e = "";
      if(name == NULL || strcmp(name, e)==0) {
        f->eax = -1;
        break;
      }
      else{
        struct file *ff = filesys_open(name);
      
        if(ff==NULL) {
          f->eax = -1;
          break;
        }
        else{
          struct thread *t = thread_current();
          struct file_fd *ffd;
          ffd -> fd = t->num_file+2;
          ffd -> file = ff;
          list_push_front(&(t->file_list),&ffd->elem);
          t->num_file++;
          f->eax = ffd->fd;
        }
      }
      break;
     }
      
    case SYS_FILESIZE:{
      user_memory(f->esp,1);
      int fd = *((int *)(f->esp)+1);
      struct file * ff=get_file_from_fd(fd);
      f->eax = (int) file_length(ff);
      break;
     }
      
    case SYS_READ:{
      user_memory(f->esp,3);
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);
      
      user_memory((void *)buffer, 0);
      if(check_bad_ptr(thread_current()->pagedir,(const void *)buffer))exit(-1);
      check_buffer(buffer, size);
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
          int r = (int) file_read(ff, buffer, size);
          f->eax = r;
        }
      }  
      break;}
      
    case SYS_WRITE:{
      
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);

      user_memory((void *)buffer, 0);
      if(check_bad_ptr(thread_current()->pagedir,(const void *)buffer))exit(-1);
      check_buffer(buffer, size);
      if(fd==1){
        putbuf(buffer, size);
        f->eax= size;
      }
      else if(fd == 0){
        f->eax = -1;
      }
      else{
        user_memory(f->esp, 3);
        struct file *ff = get_file_from_fd(fd);
        if(ff==NULL){ 
          f->eax = -1;
        }
        else{
          int r = (int) file_write(ff, buffer, size);
          f->eax = r;
        }
      }
      break;
      }
      
    case SYS_SEEK:{
      user_memory(f->esp,2);
      int fd = *((int *)(f->esp)+1);
      unsigned position = *((unsigned *)(f->esp)+2);
      struct file *ff = get_file_from_fd(fd);
      file_seek(ff, position);
      break;
      }
      
    case SYS_TELL:{
      user_memory(f->esp,1);
      int fd = *((int *)(f->esp)+1);
      struct file *ff = get_file_from_fd(fd);
      if(ff==NULL){ 
        f->eax = -1;
      }
      else{
        f->eax = file_tell(ff);
      }
      break;}
      
    case SYS_CLOSE:{
      user_memory(f->esp,1);
      int fd = *((int *)(f->esp)+1);
      if(fd>1){
        struct file *ff = get_file_from_fd(fd);
        struct file *flm = get_elem_from_fd(fd);
       
        if(ff!=NULL){
           file_close(ff);
        }
         if(flm!=NULL){
           list_remove(flm);
        }
      }
      break;}
  }
}


void exit(int status){
      struct thread * curr=thread_current();
      struct child* chd;
      chd=curr->child;
      chd->ret =status;
      chd->exit_called =1;
      chd->dying=1;
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

void user_memory(void *esp, int n){
  int * p;
  p = (int *)esp + n;
  if(!is_user_vaddr((const void *) p)) exit(-1);
}

void check_buffer(void *buffer, unsigned size){
  unsigned i=0;
  char * b = (char *) buffer;
  for(;i++;i<size){
    if(!is_user_vaddr((const void *) b)) exit(-1);
    b++;
  }
}

bool check_bad_ptr(uint32_t * pd,const void * uaddr){
  void * p = pagedir_get_page (pd, uaddr);
  return p==NULL;}
