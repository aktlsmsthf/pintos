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

static void syscall_handler (struct intr_frame *);
void exit(int);
struct file* get_file_from_fd(int);
struct list_elem* get_elem_from_fd(int);
bool user_memory(void *, int);
bool check_buffer(void *, unsigned);
bool check_bad_ptr(uint32_t * pd,const void * uaddr);
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
  if(!is_user_vaddr((const void *)f->esp)) exit(-1);
  if(check_bad_ptr(thread_current()->pagedir,f->esp)) exit(-1);
  if(f->esp<0x08048000) exit(-1);
  switch(*((int *)(f->esp))){
    case SYS_HALT:{
      power_off();
      break;}
      
    case SYS_EXIT:{
      if(!user_memory(f->esp, 1)){
        exit(-1);}
      else{
      int status = *((int *)(f->esp)+1);
      exit(status);}
      break;}
      
    case SYS_EXEC:{
      if(!user_memory(f->esp, 1)) {
        exit(-1);}
      else{
        const char * cmd_line = *((char **)(f->esp)+1);
        user_memory((void *)cmd_line, 0);
        if(check_bad_ptr(thread_current()->pagedir,(const void *)cmd_line)){exit(-1);}
        /**lock_acquire(&sys_lock);**/
        tid_t pid = process_execute(cmd_line);
        /**lock_release(&sys_lock);**/
        f->eax = pid;}
      break;
    }
      
    case SYS_WAIT:{
      if(!user_memory(f->esp,1)) { exit(-1);}
      else{
        f->eax =  process_wait((tid_t)*((int *)(f->esp)+1));}
      break;
    }
      
    case SYS_CREATE:{
      if(!user_memory(f->esp,2)) {exit(-1);}
      else{
        const char *file = *((char **)(f->esp)+1);
        unsigned initial_size = *((unsigned *)(f->esp)+2);
      
        if(!user_memory((void *)file, 0)) exit(-1);
        if(check_bad_ptr(thread_current()->pagedir,(const void *)file)) {exit(-1);}
        if(file==NULL){
          f->eax =-1;
          /**exit(-1);**/
      }
      else{
        f->eax = filesys_create (file,initial_size);
      }}
      break;
      }
      
    case SYS_REMOVE:{
      if(!user_memory(f->esp,1)) {exit(-1);}
      else{
      const char *file = *((char **)(f->esp)+1);
      if(check_bad_ptr(thread_current()->pagedir,(const void *)file)) {exit(-1);}
      f->eax = filesys_remove (file); }
      break;
     }
      
    case SYS_OPEN:{
      if(!user_memory(f->esp,1)) {exit(-1);}
      else{
      const char *name= *((char **)(f->esp)+1);
      
      /**if(!user_memory((void *)name, 0)) {f->eax = -1; break;}**/
      /**if(!is_user_vaddr(name)) {f->eax = -1;}**/
      /**else{**/
        
        /**if(check_bad_ptr(thread_current()->pagedir,(const void *)name)) {exit(-1);}**/
        char *e = "";
        if(name == NULL || strcmp(name, e)==0) {
          f->eax = -1;
          printf("2\n");
        }
        else{
          /**lock_acquire(&sys_lock);**/
          struct file *ff = filesys_open(name);
          /**lock_release(&sys_lock);**/
        
          if(ff==NULL) {
            f->eax = -1;
            printf("3\n");
          }
          else{
            struct thread *t = thread_current();
            struct file_fd *ffd = malloc(sizeof(struct file_fd));
            if(ffd==NULL){
              f->eax =-1;
               printf("4\n");
            }
            else{
              ffd -> fd = t->num_file+2;
              ffd -> file = ff;
              list_push_front(&(t->file_list),&ffd->elem);
              t->num_file++;
              f->eax = ffd->fd;
            }
          }
        /**}**/
      }}
      break;
     }
      
    case SYS_FILESIZE:{
      if(!user_memory(f->esp,1)) { exit(-1);}
      else{
        int fd = *((int *)(f->esp)+1);
        struct file * ff=get_file_from_fd(fd);
        f->eax = (int) file_length(ff);
      break;}
     }
      
    case SYS_READ:{
      if(!user_memory(f->esp,3)) {exit(-1);}
      else{
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);
      
      if(!user_memory((void *)buffer, 0)) {exit(-1);}
      if(check_bad_ptr(thread_current()->pagedir,(const void *)buffer)){ exit(-1);}
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
      }}
      break;}
      
    case SYS_WRITE:{
      
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);

      if(!user_memory((void *)buffer, 0)) {exit(-1);}
      if(check_bad_ptr(thread_current()->pagedir,(const void *)buffer)) {exit(-1);}
      check_buffer(buffer, size);
      if(fd==1){
        putbuf(buffer, size);
        f->eax= size;
      }
      else if(fd == 0){
        f->eax = -1;
      }
      else{
        if(!user_memory(f->esp, 3)) {exit(-1);}
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
      if(!user_memory(f->esp,2)) {exit(-1);}
      int fd = *((int *)(f->esp)+1);
      unsigned position = *((unsigned *)(f->esp)+2);
      struct file *ff = get_file_from_fd(fd);
      file_seek(ff, position);
      break;
      }
      
    case SYS_TELL:{
      if(user_memory(f->esp,1)) {exit(-1);}
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
      if(!user_memory(f->esp,1)) { exit(-1);}
      int fd = *((int *)(f->esp)+1);
      if(fd>1){
        struct list_elem *flm = get_elem_from_fd(fd);
        struct file_fd *ffd = list_entry(flm, struct file_fd, elem);
       
        if(ffd->file!=NULL){
           file_close(ffd->file);
        }
         if(flm!=NULL){
           list_remove(flm);
        }
        if(ffd!=NULL)
           free(ffd);
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

/**void user_memory(void *esp, int n){
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
}**/

bool user_memory(void *esp, int n){
  int * p;
  p = (int *)esp + n;
  if(!is_user_vaddr((const void *) p)) return 0;
  else return 1;
}

bool check_buffer(void *buffer, unsigned size){
  unsigned i=0;
  char * b = (char *) buffer;
  for(;i++;i<size){
    if(!is_user_vaddr((const void *) b)) return 0;
    b++;
  }
  return 1;
}

bool check_bad_ptr(uint32_t * pd,const void * uaddr){
  void * p = pagedir_get_page (pd, uaddr);
  return p==NULL;}
