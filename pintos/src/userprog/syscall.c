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
#include "devices/timer.h"

static void syscall_handler (struct intr_frame *);
void exit(int);
struct file* get_file_from_fd(int);
struct list_elem* get_elem_from_fd(int);
struct list_elem* get_elem_from_mid(int mid);
bool user_memory(void *, int);
bool check_buffer(void *, unsigned);
//bool check_bad_ptr(const void * uaddr);
bool check_bad_ptr(struct intr_frame *f ,const void * uaddr);


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
	//printf("%s %d\n",thread_current()->name, *((int *)(f->esp)));
  if(!is_user_vaddr((const void *)f->esp)){exit(-1);}
  if(check_bad_ptr(f,f->esp)){exit(-1);}
  switch(*((int *)(f->esp))){
    case SYS_HALT:{
      power_off();
      break;}
      
    case SYS_EXIT:{
      if(!user_memory(f->esp, 1)){
        exit(-1);}
    
      int status = *((int *)(f->esp)+1);
      exit(status);
      break;
    }
      
    case SYS_EXEC:{
      if(!user_memory(f->esp, 1)){
        exit(-1);}
      const char * cmd_line = *((char **)(f->esp)+1);
      if(!user_memory((void *)cmd_line, 0)){f->eax = -1; break;}
      if(check_bad_ptr(f,(const void *)cmd_line)){
        exit(-1);
      }
      /**lock_acquire(&sys_lock);**/
      tid_t pid = process_execute(cmd_line);
      /**lock_release(&sys_lock);**/
      f->eax = pid;
      break;
    }
      
    case SYS_WAIT:{
      if(!user_memory(f->esp,1)){
        exit(-1);}
     
      f->eax =  process_wait((tid_t)*((int *)(f->esp)+1));
      break;
    }
      
    case SYS_CREATE:{
      if(!user_memory(f->esp,2)){
        exit(-1);}
      
      const char *file = *((char **)(f->esp)+1);
      unsigned initial_size = *((unsigned *)(f->esp)+2);
      
      if(!user_memory((void *)file, 0)){
        exit(-1);}
      if(check_bad_ptr(f,(const void *)file)) {exit(-1);}
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
      if(!user_memory(f->esp,1)){
        exit(-1);}
      
      const char *file = *((char **)(f->esp)+1);
      if(check_bad_ptr(f,(const void *)file)) {exit(-1);}
      lock_acquire(&sys_lock);
      f->eax = filesys_remove (file);
      lock_release(&sys_lock);
      break;
     }
      
    case SYS_OPEN:{
      if(!user_memory(f->esp,1)){
        
        exit(-1);
        break;
      }
      
      const char *name= *((char **)(f->esp)+1);
      
      if(!user_memory((void *)name, 0)) {exit(-1);break;}
      if(!is_user_vaddr(name)) {f->eax = -1;break;}
      else{
        if(check_bad_ptr(f,(const void *)name)){
          exit(-1);
          break;
        }
    
        char *e = "";
        if(name == NULL || strcmp(name, e)==0) {
          f->eax = -1;
        }
        else{
          lock_acquire(&sys_lock);
          struct file *ff = filesys_open(name);
          lock_release(&sys_lock);
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
          
        }
      }
      break;
     }
      
    case SYS_FILESIZE:{
      if(!user_memory(f->esp,1)){
        exit(-1);}
      int fd = *((int *)(f->esp)+1);
      struct file * ff=get_file_from_fd(fd);
      f->eax = (int) file_length(ff);
      break;
     }
      
    case SYS_READ:{
      if(!user_memory(f->esp,3)){
        exit(-1);}
      
      int fd = *((int *)(f->esp)+1);
      const void *buffer = *((void **)(f->esp)+2);
      unsigned size = *((unsigned *)(f->esp)+3);

      unsigned buffer_size = size;
      void * buffer_tmp = buffer;
      
      while(buffer_tmp!=NULL){
        if(!is_user_vaddr(buffer_tmp)) exit(-1);
        
        if(pagedir_get_page(thread_current()->pagedir, buffer_tmp)==NULL){
          struct spt_entry *spte = spte_find(pg_round_down(buffer_tmp));
          if(spte!=NULL){
    	    if(spte->lazy){
	      file_frame_alloc(spte);
	    }
            else if(spte->fe->in_swap){
              swap_in(spte->fe, spte->flags);
            }
          }
          else{
            if(buffer_tmp>=f->esp-32){
              uint8_t *frame = frame_spt_alloc( &thread_current()->spt,pg_round_down(buffer_tmp), true,6);
              install_page(pg_round_down(buffer_tmp), frame, true);
            }
          }
          
        }
        if (buffer_size == 0){
	    buffer_tmp = NULL;
	}
        else if (buffer_size > PGSIZE){
	     buffer_tmp += PGSIZE;
	     buffer_size -= PGSIZE;
	} 
        else{
	    buffer_tmp = buffer + size - 1;
	    buffer_size = 0;
	 }
      }
      
      struct spt_entry *spte = spte_find(pg_round_down(buffer));
      if(!spte->writable){
	   exit(-1);
      }
      
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

      unsigned buffer_size = size;
      void * buffer_tmp = buffer;
      
      while(buffer_tmp!=NULL){
        if(!is_user_vaddr(buffer_tmp)) exit(-1);
        
        if(pagedir_get_page(thread_current()->pagedir, buffer_tmp)==NULL){
          struct spt_entry *spte = spte_find(pg_round_down(buffer_tmp));
          if(spte!=NULL){
	    if(spte->lazy){
	      file_frame_alloc(spte);
	    }
            if(spte->fe->in_swap){
              swap_in(spte->fe, spte->flags);
            }
          }
          else{
            if(buffer_tmp>=f->esp-32){
              uint8_t *frame = frame_spt_alloc(&thread_current()->spt,pg_round_down(buffer_tmp), true, 6);
              install_page(pg_round_down(buffer_tmp), frame, true);
            }
          }
        }
	
        if (buffer_size == 0){
	        buffer_tmp = NULL;
	      }
        else if (buffer_size > PGSIZE){
	        buffer_tmp += PGSIZE;
	        buffer_size -= PGSIZE;
	      } 
        else{
	        buffer_tmp = buffer + size - 1;
	        buffer_size = 0;
	      }
      }
      
      
      if(fd==1){
        putbuf(buffer, size);
        f->eax= size;
      }
      else if(fd == 0){
        f->eax = -1;
      }
      else{
        if(!user_memory(f->esp, 3)){
          exit(-1);}
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
      if(!user_memory(f->esp,2)) 
        exit(-1);
      int fd = *((int *)(f->esp)+1);
      unsigned position = *((unsigned *)(f->esp)+2);
      struct file *ff = get_file_from_fd(fd);
      lock_acquire(&sys_lock);
      file_seek(ff, position);
      lock_release(&sys_lock);
      break;
    }
      
    case SYS_TELL:{
      if(!user_memory(f->esp,1)) 
        exit(-1);
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
      if(!user_memory(f->esp,1)) {
        exit(-1);}
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
		  
    case SYS_MMAP:{
	if(!user_memory(f->esp,2)){exit(-1);}
	int fd = *((int *)(f->esp)+1);
	void *addr = *((void **)(f->esp)+2);
	
	struct file *file = get_file_from_fd(fd);
	if(file==NULL){
		f->eax = -1;
		break;
	}
	printf("2\n");
	struct file *mfile = file_reopen(file);
	uint32_t size = file_length(mfile);
	uint32_t read_bytes = size;
	uint32_t zero_bytes = size%PGSIZE;
	uint32_t ofs = 0;
	uint32_t page_read_bytes;
	uint32_t page_zero_bytes;
	void* daddr = addr;
	    
	    printf("3\n");
	while(read_bytes>0){
		page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		page_zero_bytes = PGSIZE - page_read_bytes;
		
		spt_alloc_lazy(&thread_current()->spt, addr, true, PAL_USER|PAL_ZERO, page_read_bytes, page_zero_bytes, mfile, ofs);
		
		read_bytes-=PGSIZE;
		ofs+=page_read_bytes;
		addr+=PGSIZE;
	}
	    printf("4\n");
	    
	struct mmapped *m = malloc(sizeof(struct mmapped));
	    printf("5\n");
	m->addr = daddr;
	m->file = mfile;
	m->mid = fd;
	m->size = size;
	    
	    printf("6\n");
	list_push_front(&thread_current()->mapped_list, &m->elem);
	f->eax = fd;
	    printf("7\n");
	break;
    }
    
    case SYS_MUNMAP:{
	if(!user_memory(f->esp, 1)){ exit(-1);}
	int mid = *((int *)(f->esp)+1);
	
	struct list_elem *elem = get_elem_from_mid(mid);
	struct mmapped *mapped = list_entry(elem, struct mmapped, elem);
	
	uint32_t write_bytes = 0;
	void *addr = mapped->addr;
	    
	while(write_bytes<mapped->size){
		struct spt_entry *spte = spte_find(addr);
		if(!spte->lazy){
			if(pagedir_is_dirty(spte->t->pagedir, addr)){
				file_write_at(mapped->file, spte->page, spte->read_bytes, spte->ofs);
			}
		}
		free(spte);
		addr+=PGSIZE;
		write_bytes+=PGSIZE;
	}
	list_remove(&mapped->elem);
	free(mapped);
	
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
}struct file* get_file_from_fd(int fd){
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
}struct list_elem* get_elem_from_fd(int fd){
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

struct list_elem* get_elem_from_mid(int mid){
      struct thread * curr=thread_current();
      if(list_empty(&(curr->mapped_list))){
         return NULL;
      }
      struct list_elem * melem = list_front(&(thread_current()->mapped_list));
      struct mmapped * mapped;
      while(list_entry(melem, struct mmapped, elem)->mid != mid){
          melem = melem->next;
          if(melem->next==NULL){
             return NULL;
          }
      }
      return melem;
}
bool user_memory(void *esp, int n){
  int * p;
  p = (int *)esp + n;
  if(!is_user_vaddr((const void *) p)) {return 0;}
  else return 1;
}


bool check_buffer(void *buffer, unsigned size){
  unsigned i=0;
  char * b = (char *) buffer;
  for(;i++;i<size){
    if(!is_user_vaddr((const void *) b)){return 0;}
    b++;
  }
  return 1;
}
bool check_bad_ptr(struct intr_frame *f, const void * uaddr){
  
    void * p = pagedir_get_page (thread_current()->pagedir, pg_round_down(uaddr));
    return p==NULL ;
}
