#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
struct thread* get_child_process(int tid);

#endif /* userprog/syscall.h */
