/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

/* Memory Mapped Files */
#include "threads/vaddr.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	struct uninit_page *uninit =  &page->uninit;

	memset(uninit, 0, sizeof (struct uninit_page));

	/* Memory Mapped Files */
	struct load_aux *load_aux = (struct load_aux*)page->uninit.aux;
	if (load_aux != NULL) {
		file_page->file = load_aux->file;
		file_page->ofs = load_aux->ofs;
		file_page->read_bytes = load_aux->read_bytes;
		file_page->zero_bytes = load_aux->zero_bytes;
		file_page->writable = load_aux->writable;
	}

	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
	
	return lazy_load_segment_file (page, file_page);
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	struct thread *curr = thread_current ();

	if (page == NULL) {
		return false;
	}

	struct load_aux *load_aux = (struct load_aux*)page->uninit.aux;
	struct file *file_obj = load_aux->file;

	if (pml4_is_dirty (thread_current ()->pml4, page->va)) {
		file_write_at (file_obj, page->va, load_aux->read_bytes, load_aux->ofs);

		pml4_set_dirty (thread_current ()->pml4, page->va, false);
	}

	pml4_clear_page (thread_current ()->pml4, page->va);

	return true;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	struct load_aux *load_aux = (struct load_aux*)page->uninit.aux;
	struct file *file_obj = load_aux->file;
	struct thread *curr = thread_current ();

	if (pml4_is_dirty (&curr->pml4, page->va)) {
		file_write_at (file_page->file, page->va, file_page->read_bytes, file_page->ofs);
		pml4_set_dirty (&curr->pml4, page->va, 0);
	}
	
	pml4_clear_page (&curr->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	struct thread *curr = thread_current ();
	struct file *file_obj = file_reopen (file);

	if (file_obj == NULL) {
		return NULL;
	}

	void *init_addr = addr;

	size_t read_bytes = length > file_length(file) ? file_length (file) : length;
	size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;

	ASSERT (pg_ofs (addr) == 0);
	ASSERT (offset % PGSIZE == 0);
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0) {
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		struct load_aux *aux = (struct load_aux *) malloc (sizeof (struct load_aux));

		aux->file = file_obj;
		aux->ofs = offset;
		aux->read_bytes = page_read_bytes;
		aux->zero_bytes = page_zero_bytes;
		aux->writable = writable;

		struct page *p = spt_find_page(&thread_current()->spt, init_addr);

		if (!vm_alloc_page_with_initializer (VM_FILE, addr, writable, lazy_load_segment_file, aux)) {
			return NULL;
		}

		struct page *page = spt_find_page (&thread_current ()->spt, init_addr);

		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		addr += PGSIZE;
		offset += page_read_bytes;
	}

	return init_addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct thread *curr =  thread_current ();
	struct supplemental_page_table *spt = &curr->spt;
	struct page *page = spt_find_page (&curr->spt, addr);

	int count = page->page_count;
	for (int i = 0; i < count; i++) {
		if (page) {
			destroy (page);
		}
		addr += PGSIZE;
		page = spt_find_page (spt, addr);
	}

}

static bool
lazy_load_segment_file (struct page *page, void *aux) {
	struct load_aux *load_aux = (struct load_aux*)aux;
	struct file *file_obj = load_aux->file;
	size_t page_read_bytes = load_aux->read_bytes;
	size_t page_zero_bytes = load_aux->zero_bytes;
	off_t ofs = load_aux->ofs;

	file_seek (file_obj, ofs);

	void *kva = page->frame->kva;

	file_seek (file_obj, ofs);

	if (file_read (file_obj, kva, page_read_bytes) != (int)page_read_bytes) {
		return false;
	}

	memset (kva + page_read_bytes, 0, page_zero_bytes);

	return true;
}