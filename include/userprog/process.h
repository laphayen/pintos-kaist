#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);

/* Argument Passing */
void argument_stack (char **pasre, int count, void **rsp);

/* Hierarchical Process Structure */
struct thread *get_child_process (int pid);
bool remove_child_process (struct thread *cp);

#endif /* userprog/process.h */
