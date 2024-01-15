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

/* Hierarchical Process Structure */
int exec (const char *file);
int wait (int pid);

/* File Descriptor */
struct file *process_get_file (int fd);
int process_add_file (struct file *f);
void process_close_file (int fd);

/* File Descriptor */
struct lock filesys_lock;

/* File Descriptor */
int open (const char *file);
int read (int fd, void *buffer, unsigned size);
int write (int fd, void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
tid_t fork (const char *thread_name, struct intr_frame *f);


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

	/* File Descriptor */
	lock_init (&filesys_lock);
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
		case SYS_FORK:
			f->R.rax = fork (f->R.rdi, f);
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
		case SYS_OPEN:
			f->R.rax = open (f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize (f->R.rdi);
		case SYS_READ:
			f->R.rax = read (f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write (f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			f->R.rax = seek (f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell (f->R.rdi);
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

	/* Hierarchical Process Structure */
	curr->exit_status = status;

	printf ("%s: exit(%d)\n", thread_name (), status);
	thread_exit ();
}

/* File Descriptor */
tid_t
fork (const char *thread_name, struct intr_frame *f) {

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

	list_push_back (&curr->child_list, &child->child_elem);

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

/* File Descriptor */
int
open (const char *file) {
	check_address (file);

	lock_acquire (&filesys_lock);

	struct file *file_obj = filesys_open (file);

	if (file_obj == NULL) {
		return -1;
	}

	int fd = process_add_file (file_obj);

	if (fd == -1) {
		file_close (file_obj);
	}

	lock_acquire (&filesys_lock);

	return fd;
}

/* File Descriptor */
int
filesize (int fd) {
	check_address (fd);
	
	struct file *file = process_get_file (fd);

	if (file == NULL) {
		return -1;
	}

	lock_acquire (&filesys_lock);

	if (file) {
		lock_release (&filesys_lock);
		return file_length (file);
	}

	lock_release (&filesys_lock);

	return -1;
}

/* File Descriptor */
int
read (int fd, void *buffer, unsigned size) {
	check_address (buffer);

	struct file *file = process_get_file (fd);
	int read_byte;
	int fd_byte;

	if (fd == 1) {
		return -1;
	}

	if (fd == 0) {
		lock_acquire (&filesys_lock);
		fd_byte = input_getc ();
		lock_release (&filesys_lock);
		return fd_byte;
	}

	if (file) {
		lock_acquire (&filesys_lock);
		int read_byte = file_read (file, buffer, size);
		lock_release (&filesys_lock);
		return read_byte;
	}

	return -1;
}

/* File Descriptor */
int write (int fd, void *buffer, unsigned size) {
	check_address (buffer);

	struct file *file = process_get_file (fd);
	int write_byte;

	if (fd == 0) {
		return -1;
	}


	if (fd == 1) {
		lock_acquire (&filesys_lock);
		putbuf (buffer, size);
		lock_release (&filesys_lock);
		return size;
	}

	if (file) {
		lock_acquire (&filesys_lock);
		write_byte = file_write (file, buffer, size);
		lock_release (&filesys_lock);
		return write_byte;
	}

	return -1;
}

/* File Descriptor */
void
seek (int fd, unsigned position) {
	struct file *file = process_get_file (fd);

	if (fd < 2) {
		return;
	}

	if (file) {
		file_seek (file, position);
	}
}

/* File Descriptor */
void
close (int fd) {
	struct file *file = process_get_file (fd);
	
	if (file == NULL) {
		return;
	}
	
	process_close_file (fd);
}

/* File Descriptor */
unsigned
tell (int fd) {
	struct file *file = process_get_file (fd);

	if (fd < 2) {
		return;
	}

	if (file) {
		return file_tell (file);
	}
}




/* File Descriptor*/
int
process_add_file (struct file *f) {
	struct thread *curr = thread_current ();
	struct file **fdt = curr->fd_table;

	while (curr->fd_idx < FDTABLE_MAX && fdt[curr->fd_idx]) {
		curr->fd_idx++;
	}

	if (curr->fd_idx >= FDTABLE_MAX) {
		return -1;
	}

	fdt[curr->fd_idx] = f;
	
	return curr->fd_idx;
}

/* File Descriptor */
/* 파일 디스크립터에 해당하는 파일 객체를 리턴 */
struct file
*process_get_file (int fd) {
	struct thread *curr = thread_current ();

	if (fd >=0 && fd < FDTABLE_MAX) {
		return NULL;
	}
	
	return curr->fd_table[fd];
}

/* File Descriptor */
/* 파일 디스크립터 테이블 해당 엔트리 초기화 */
void
process_close_file (int fd) {
	struct thread *curr =  thread_current ();

	if (fd >=0 && fd < FDTABLE_MAX) {
		return NULL;
	}

	curr->fd_table[fd] = NULL;
}