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
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);

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
			break ;
		case SYS_CREATE:
			create (f->R.rdi, f->R.rsi);
			break ;
		case SYS_REMOVE:
			remove (f->R.rdi);
			break ;
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

/* System Call */
/* 파일 이름과 사이즈를 인자 값으로 받아 파일을 생성하는 함수*/
bool
create (const char *file, unsigned initial_size) {
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