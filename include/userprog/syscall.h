#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* User Memory Access */
void check_address (void *addr);

/* System Call */
void halt (void);
void exit (int status);

#endif /* userprog/syscall.h */
