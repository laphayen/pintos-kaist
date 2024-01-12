#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

/* System Call */
#include "threads/init.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* User Memory Access */
void check_address (void *addr);

/* System Call */
void halt (void);
void exit (int status);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);

/* Hierarchical Process Structure */
int exec (const char *file);
int wait (int pid);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	/* System Call */
	int syscall_number = f->R.rax;
	switch (syscall_number) {
		case SYS_HALT:
			halt ();
			break;
		case SYS_EXIT:
			exit (f->R.rdi);
			break;
		case SYS_EXEC:
			if (exec (f->R.rdi) == -1) {
				exit (-1);
			}
			break;
		case SYS_WAIT:
			f->R.rax = wait (f->R.rdi);
			break;
		case SYS_CREATE:
			f->R.rax = create (f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove (f->R.rdi);
			break;
		default:
			break;
	}
}

/* User Memory Access */
/* Check if the address value is within the range of addresses used by the user space. */
/* If the address is outside the user space, terminate the process. */
void
check_address (void *addr) {
	struct thread *curr = thread_current ();
	if (addr == NULL || is_kernel_vaddr(addr) || pml4_get_page(curr->pml4, addr) == NULL) {
		exit(-1);
	}
}

/* System Call */
/* A system call to shut down Pint OS. */
void
halt (void) {
	power_off ();
}

/* System Call */
/* A system call to terminate a thread. */
void
exit (int status) {
	struct thread *curr = thread_current ();
	curr->exit_status = status;
	printf ("%s: exit(%d)\n", thread_name (), status);
	thread_exit ();
}

/* System Call */
/* "A system call that takes a file name and size as arguments to create a file. */
bool
create (const char *file, unsigned initial_size) {
	check_address (file);
	return filesys_create (file, initial_size);
}

/* System Call */
/* A function to remove a file corresponding to the file name. */
bool
remove (const char *file) {
	check_address (file);
	return filesys_remove (file);
}

/* Hierarchical Process Structure */
/* A system call to create a child process and execute a program. */
int
exec (const char *file) {
	check_address (file);
	
	struct thread *curr = thread_current ();
	tid_t pid = process_create_initd (file);

	struct thread *child = get_child_process (pid);

	int result = process_wait (child);

	list_push_back(&curr->child_list, &child->child_elem);

	if (result == 1) {
		return pid;
	}
	else {
		return -1;
	}

}

/* Hierarchical Process Structure */
/* A system call to wait until a child process exits. */
int
wait (int pid) {
	return process_wait (pid);
}