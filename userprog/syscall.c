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

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* User Memory Access */
void check_address (void *addr);

/* System Call */
void halt (void);
void exit (int status);
// int fork (const char *thread_name, struct intr_frame *f);
// int exec (const char *file);
// int wait (tid_t tid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
// int open (const char *file);
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned size);
// int write (int fd, const void *buffer, unsigned size);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);
// int dup2 (int oldfd, int newfd);

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
			break ;
		case SYS_EXIT:
			exit (f->R.rdi);
		// case SYS_FORK:
		// 	f->R.rax = fork (f->R.rdi, f);
		// 	break ;
		// case SYS_EXEC:
		// 	// exec (f->R.rdi);
		// 	break ;
		// case SYS_WAIT:
		// 	// f->R.rax = wait (f->R.rdi);
		// 	break ;
		case SYS_CREATE:
			create (f->R.rdi, f->R.rsi);
			break ;
		case SYS_REMOVE:
			remove (f->R.rdi);
			break ;
		// 	break ;
		// case SYS_OPEN:
		// 	f->R.rax = open (f->R.rdi);
		// 	break ;
		// case SYS_FILESIZE:
		// 	// f->R.rax = filesize (f->R.rdi);
		// 	break ;
		// case SYS_READ:
		// 	// f->R.rax = read (f->R.rdi, f->R.rsi, f->R.rdx);
		// 	break ;
		// case SYS_WRITE:
		// 	f->R.rax = write (f->R.rdi, f->R.rsi, f->R.rdx);
		// 	break ;
		// case SYS_SEEK:
		// 	// seek (f->R.rdi, f->R.rsi);
		// 	break ;
		// case SYS_TELL:
		//  	// f->R.rax = tell (f->R.rdi);
		// 	break ;
		// case SYS_CLOSE:
		// 	// close (f->R.rdi);
		// 	break ;
		// case SYS_DUP2:
		// 	// dup2 (f->R.rdi, f->R.rsi);
		// 	break;
		default :
			thread_exit ();
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
/* Pint OS를 종료시키는 함수 */
void
halt (void) {
	power_off ();
}

/* System Call */
/* 쓰레드를 종료시키는 함수 */
void
exit (int status) {
	struct thread *curr = thread_current ();
	curr->status = status;
	printf ("%s: exit(%d)\n", thread_name (), status);
	thread_exit ();
}

// int
// fork (const char *thread_name, struct intr_frame *f) {
// 	// check_address (thread_name);
// 	// return process_fork (thread_name, f);
// }

// int
// exec (const char *file) {
	// check_address (file);

	// int file_size = strlen (file) + 1;
	// char *file_copy = palloc_get_page (PAL_ZERO);

	// if (file_copy == NULL) {
	// 	exit (-1);
	// }

	// strlcpy (file_copy, file, file_size);

	// if (process_exec (file_copy) == -1) {
	// 	return -1;
	// }

	// NOT_REACHED ();
	// return 0;
// }


// int
// wait (tid_t tid) {
// 	process_wait (tid);
// }

/* System Call */
/* 파일 이름과 사이즈를 인자 값으로 받아 파일을 생성하는 함수*/
bool
create(const char *file, unsigned initial_size) {
	check_address (file);
	return filesys_create (file, initial_size);
}

/* System Call */
/* 파일 이름에 해당하는 파일을 제거하는 함수*/
bool
remove (const char *file) {
	check_address (file);
	return filesys_remove (file);
}

// int
// open (const char *file) {
// 	check_address (file);

// 	struct file *file_name = filesys_open (file);

// 	if (file_name == NULL) {
// 		return -1;
// 	}

	
// }

// int
// filesize (int fd) {

// }

// int
// read (int fd, void *buffer, unsigned size) {

// }

// int
// write (int fd, const void *buffer, unsigned size) {

// }

// void
// seek (int fd, unsigned position) {

// }

// unsigned
// tell (int fd) {

// }

// void
// close (int fd) {

// }

// int
// dup2 (int oldfd, int newfd) {

// }