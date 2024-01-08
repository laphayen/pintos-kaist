#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

// /* User Memory Access */
// void check_address (void *addr);

// /* System Call */
// void halt (void);
// void exit (int status);
// int fork (const char *thread_name, struct intr_frame *f);
// int exec (const char *file);
// int wait (tid_t tid_t);
// bool create (const char *file, unsigned initial_size);
// bool remove (const char *file);
// int open (const char *file);
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned size);
// int write (int fd, const void *buffer, unsigned size);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);
// int dup2 (int oldfd, int newfd);

#endif /* userprog/syscall.h */
