#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
struct thread* get_child_process(int tid);

struct file_fd{
  int fd;
  struct file *file;
  struct list_elem elem;
};




#endif /* userprog/syscall.h */
