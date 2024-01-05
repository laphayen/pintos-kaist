#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* User Memory Access */
void check_address (void *addr);

#endif /* userprog/syscall.h */
