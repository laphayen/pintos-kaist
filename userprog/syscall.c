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

/* File Descriptor */
#include "userprog/process.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "filesys/file.h"

/* Denying Write To Executable */
const int STDIN = 1;
const int STDOUT = 2;

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
int exec (const char *file_name);
int wait (tid_t pid);

/* File Descriptor */
struct file *process_get_file (int fd);
int process_add_file (struct file *f);
void process_close_file (int fd);

/* File Descriptor */
struct lock filesys_lock;

/* File Descriptor */
int open (const char *file);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
tid_t fork (const char *thread_name, struct intr_frame *f);

/* Dup2 */
int dup2 (int oldfd, int newfd);

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
			f->R.rax = process_wait (f->R.rdi);
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
			break;
		case SYS_READ:
			f->R.rax = read (f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write (f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek (f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell (f->R.rdi);
			break;
		case SYS_CLOSE:
			close (f->R.rdi);
			break;
		case SYS_DUP2:
			f->R.rax = dup2 (f->R.rdi, f->R.rsi);
			break;
		default:
			exit (-1);
			break;
	}
}

/* User Memory Access */
/* Check if the address value is within the range of addresses used by the user space. */
/* If the address is outside the user space, terminate the process. */
void
check_address (void *addr) {
	struct thread *curr = thread_current ();
	
	if (!is_user_vaddr (addr) || addr == NULL) {
		exit (-1);
	}

	#ifdef VM
	if (pml4_get_page (&curr->pml4, addr) == NULL) {
		if (spt_find_page (&curr->spt, addr) == NULL) {
			exit (-1);
		}
	}
	#endif
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
/* A system call to duplicate a new process from the currently executing process. */
tid_t
fork (const char *thread_name, struct intr_frame *f) {
	return process_fork (thread_name, f);
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
exec (const char *file_name) {
	check_address (file_name);

	int size = strlen (file_name) + 1;
	char *fn_copy = palloc_get_page (PAL_ZERO);

	if (fn_copy == NULL) {
		exit (-1);
	}

	strlcpy (fn_copy, file_name, size);

	if (process_exec (fn_copy) == -1) {
		return -1;
	}

	NOT_REACHED ();

	return 0;
}

/* Hierarchical Process Structure */
/* A system call to wait until a child process exits. */
int
wait (tid_t pid) {
	process_wait (pid);
}

/* File Descriptor */
/* A system call used to open a file. */
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
	
	lock_release (&filesys_lock);

	return fd;
}

/* File Descriptor */
/* A system call that provides information about the size of a file. */
int
filesize (int fd) {
	struct file *file_obj = process_get_file (fd);

	if (file_obj == NULL) {
		return -1;
	}

	return file_length (file_obj);
}

/* File Descriptor */
/* A system call for reading data from an open file. */
int
read (int fd, void *buffer, unsigned size) {
	check_address(buffer);

	unsigned char *read_buffer = buffer;
	struct thread *curr = thread_current ();
	struct file *file_obj = process_get_file (fd);
	int read_count;

	if (file_obj == NULL || file_obj == STDOUT) {
		return -1;
	}	

	if (file_obj == STDIN) {
		/* Dup2 */
		if (curr->stdin_count == 0) {
			NOT_REACHED ();
			process_close_file (fd);
			read_count = -1;
		}
		else {
			char key;
			int i;
			for (i = 0; i < size; i++) {
				key = input_getc ();
				*read_buffer++ = key;
				if (key == '\0') {
					break;
				}
			}
			read_count = i;
		}
	}
	else {
		lock_acquire (&filesys_lock);
		read_count = file_read (file_obj, buffer, size);
		lock_release (&filesys_lock);
	}

	return read_count;
}

/* File Descriptor */
/* A system call for writing data to an open file. */
int
write (int fd, const void *buffer, unsigned size) {
	check_address (buffer);

	struct thread *curr = thread_current ();
	struct file *file_obj = process_get_file (fd);
	int write_count;

	if (file_obj == NULL || file_obj == STDIN) {
		return -1;
	}

	if (file_obj == STDOUT) {
		/* Dup2 */
		if (curr->stdout_count == 0) {
			NOT_REACHED ();
			process_close_file (fd);
			write_count = -1;
		}
		else {
			putbuf (buffer, size);
			write_count = size;
		}
	}
	else {
		lock_acquire (&filesys_lock);
		write_count = file_write (file_obj, buffer, size);
		lock_release (&filesys_lock);
	}
	return write_count;
}

/* File Descriptor */
/* A system call for moving the position within an open file. */
void
seek (int fd, unsigned position) {
	if (fd < 2) {
		return;
	}

	struct file *file_obj = process_get_file (fd);

	if (file_obj == NULL) {
		return;
	}

	file_seek (file_obj, position);
}

/* File Descriptor */
/* A system call for closing an open file. */
unsigned
tell (int fd) {
	struct file *file_obj = process_get_file (fd);

	if (fd < 2) {
		return;
	}

	return file_tell (file_obj);
}

/* File Descriptor */
/* A system call for closing an open file. */
void
close (int fd){
	if (fd < 2) {
		return;
	}

	struct thread *curr = thread_current ();
	struct file *file_obj = process_get_file (fd);

	if (file_obj == NULL) {
		return;
	}
	
	if (fd == 0 || file_obj == STDIN) {
		curr->stdin_count--;
	}
	else if (fd == 1 || file_obj == STDOUT) {
		curr->stdout_count--;
	}

	process_close_file (fd);

	if (fd <= 1 || file_obj <= 2) {
		return;
	}

	if (file_obj->dup2_count == 0) {
		file_close (file_obj);
	}
	else {
		file_obj->dup2_count--;
	}
}

/* File Descriptor*/
/* Add a file descriptor pointing to a file object to the File Descriptor table. */
int
process_add_file (struct file *file) {
	struct thread *curr = thread_current ();
	struct file **fdt = curr->fd_table;

	while (curr->fd_idx < FDCOUNT_LIMIT && fdt[curr->fd_idx]) {
		curr->fd_idx++;
	}

	if (curr->fd_idx >= FDCOUNT_LIMIT) {
		return -1;
	}

	fdt[curr->fd_idx] = file;
	
	return curr->fd_idx;
}

/* File Descriptor */
/* Return the file object corresponding to the file descriptor. */
struct file
*process_get_file (int fd) {
	struct thread *curr = thread_current ();

	if (fd < 0 || fd >= FDCOUNT_LIMIT) {
		return NULL;
	}
	
	return curr->fd_table[fd];
}

/* File Descriptor */
/* Initialize the entry in the file descriptor table corresponding to the file descriptor. */
void
process_close_file (int fd) {
	struct thread *curr =  thread_current ();

	if (fd < 0 || fd >= FDCOUNT_LIMIT) {
		return NULL;
	}

	curr->fd_table[fd] = NULL;
}

/* Dup2 */
/* A system call that duplicates file descriptors. */
int
dup2 (int oldfd, int newfd) {
	if (oldfd == newfd) {
		return newfd;
	}

	struct file *file_obj = process_get_file (oldfd);
	struct thread *curr = thread_current ();
	struct file **curr_fdt = curr->fd_table;

	if (file_obj == NULL) {
		return -1;
	}

	if (file_obj == STDIN) {
		curr->stdin_count++;
	}
	else if (file_obj == STDOUT) {
		curr->stdout_count++;
	}
	else {
		file_obj->dup2_count++;
	}

	close (newfd);
	curr_fdt[newfd] = file_obj;

	return newfd;
}