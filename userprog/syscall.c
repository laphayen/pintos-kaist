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
#include "threads/palloc.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

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
	int call_number = f->R.rax;

	switch (call_number) {
		case SYS_HALT:
			halt ();
			break ;
		case SYS_EXIT:
			exit (f->R.rdi);
			break ;
		case SYS_FORK:
			fork (f->R.rdi, f);
			break ;
		case SYS_EXEC:
			exec (f->R.rdi);
			break ;
		case SYS_WAIT:
			wait (f->R.rdi);

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
void
halt (void) {
	power_off ();
}

/* System Call */
void
exit (int status) {
	struct thread *curr = thread_current ();
	curr->status = status;
	printf ("%s: exit(%d)\n", thread_name (), status);
	thread_exit ();
}

pid_t
fork (const char *thread_name, struct intr_frame *f) {
	return process_fork (thread_name, f);
}

int
exec (const char *file) {
	check_address (file);

	int file_size = strlen (file) + 1;
	char *file_copy = palloc_get_page (PAL_ZERO);

	if (file_copy == NULL) {
		exit (-1);
	}

	strlcpy (file_copy, file, file_size);

	if (process_exec (file_copy) == -1) {
		return -1;
	}

	NOT_REACHED ();
	return 0;
}

int
wait (pid_t pid) {
	process_wait (pid);
}

bool
create (const char *file, unsigned initial_size) {
	check_address (file);

	return filesys_create (file, initial_size);
}

bool
remove (const char *file) {
	check_address (file);

	return filesys_remove (file);
}