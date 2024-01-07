#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* User Memory Access */
void check_address (void *addr);

/* System Call */
void halt (void);
void exit (int status);
tid_t fork (const char *thread_name, struct intr_frame *f);
int exec (const char *file);
int wait (tid_t tid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);

#endif /* userprog/syscall.h */
