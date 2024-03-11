/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* Memory Management */
#include "hash.h"
#include "threads/vaddr.h"

/* Anonymous Page */
#include "userprog/process.h"
#include "threads/vaddr.h"

/* Memory management */
struct list frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init (&frame_table);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* Anonymous Page */
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/* TODO: Insert the page into the spt. */
		struct page *page = (struct page*)malloc (sizeof (struct page));

		bool (*page_initailizer) (struct page *, enum vm_type, void *);

		switch (VM_TYPE (type)) {
			case VM_ANON:
				page_initailizer = anon_initializer;
				break;
			case VM_FILE:
				page_initailizer = file_backed_initializer;
				break;
		}

		uninit_new (page, upage, init, type, aux, page_initailizer);

		page->writable = writable;

		return spt_insert_page (spt, page);;
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	/* Memory Management */
	struct page *page = (struct page*)malloc (sizeof (struct page));
	struct hash_elem *spt_elem;

	page->va = pg_round_down (va);

	spt_elem = hash_find (&spt->hash_page, &page->hash_elem);

	if (spt_elem == NULL) {
		page = NULL;
	}
	else {
		page = hash_entry (spt_elem, struct page, hash_elem);
	}

	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	/* Memory Management */
	return vm_insert_page (&spt->hash_page, page);
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	
	/* Memeory Management */
	/* TODO: The policy for eviction is up to you. */
	victim = list_entry (list_pop_front (&frame_table), struct frame, frame_elem);

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();

	/* Memory Management */
	/* TODO: swap out the victim and return the evicted frame. */
	swap_out (victim->page);

	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/* Memory Management */
	struct frame *frame = (struct frame *)malloc (sizeof (struct frame));
	
	frame->kva = palloc_get_page (PAL_USER | PAL_ZERO);

	if (frame == NULL) {
		PANIC ("todo");
	}

	if (frame->kva == NULL) {
		frame = vm_evict_frame ();
		frame->page = NULL;
		return frame;
	}

	list_push_back (&frame_table, &frame->frame_elem);
	frame->page = NULL;

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	/* Stack Growth */
	vm_alloc_page (VM_ANON | VM_MARKER_0, pg_round_down(addr), true);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Stack Growth */
/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	void *rsp = NULL;
	void *rd_page = pg_round_down(addr);
	
	void *stack_start = USER_STACK;
	void *stack_end = stack_start - (1 << 20);

	if (is_kernel_vaddr (addr)) {
		return false;
	}
	
	rsp = is_kernel_vaddr (f->rsp) ? thread_current ()->rsp : (void *)f->rsp;
	
	if (not_present) {
		if (!vm_claim_page (addr)) {
			if (rsp - 8 <= addr && USER_STACK - (1<<20) <= addr && addr <= USER_STACK) {
				vm_stack_growth (rd_page);
				return true;
			}
			return false;
		}
		else {
			return true;
		}
	}

	return false;
}


/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;

	/* Memory Management */
	/* TODO: Fill this function */
	struct supplemental_page_table *spt = &thread_current ()->spt;
	page = spt_find_page (spt, va);
	if (page == NULL) {
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	
	/* Memory Management */
	struct thread *curr = thread_current ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* Memory Management */
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	pml4_set_page (curr->pml4, page->va, frame->kva, page->writable);

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init (&spt->hash_page, vm_hash_func, vm_less_func, NULL);	
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	
	/* Anonymous Page */
	struct thread *curr = thread_current ();
	struct hash_iterator iter;

	hash_first (&iter, &src->hash_page);

	while (hash_next (&iter)) {
		struct page *parent_page = hash_entry (hash_cur (&iter), struct page, hash_elem);
		enum vm_type parent_type = parent_page->operations->type;
		void *upage = parent_page->va;
		bool writable = parent_page->writable;

		if (parent_type == VM_UNINIT) {
			vm_initializer *init = parent_page->uninit.init;
			void *aux = parent_page->uninit.aux;
			vm_alloc_page_with_initializer (VM_ANON, upage, writable, init, aux);
			continue;
		}
		
		if (!vm_alloc_page (parent_type, upage, writable)) {
			return false;
		}

		if (!vm_claim_page (upage)) {
			return false;
		}

		struct page *child_page = spt_find_page (dst, upage);
		memcpy (child_page->frame->kva, parent_page->frame->kva, PGSIZE);
	}

	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* Anonymous Page */
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	hash_clear (&spt->hash_page, hash_page_destroy);
}

/* Memory Management */
/* Returning the hash value using the hash_int() function with vm_entry's vaddr as an argument. */
static unsigned
vm_hash_func (const struct hash_elem *e, void *aux) {
	struct page *page = hash_entry (e, struct page, hash_elem);
	
	return hash_bytes (&page->va, sizeof (page->va));
}

/* Memory Management */
/* A function that compares the vaddr of the input hash_elem. */
static bool
vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
	const struct page *page_a = hash_entry (a, struct page, hash_elem);
	const struct page *page_b = hash_entry (b, struct page, hash_elem);

	return page_a->va < page_b->va;
}

/* Memory Management */
/* Inserting a page into the supplemental page table */
bool
vm_insert_page (struct hash *pages, struct page *p) {
	if (!hash_insert (pages, &p->hash_elem)) {
		return true;
	}
	else {
		return false;
	}
}

/* Memory Management */
/* Deleting a page from the supplemental page table */
bool
vm_delete_page (struct hash *pages, struct page *p) {
	if (!hash_delete (pages, &p->hash_elem)) {
		return true;
	}
	else {
		return false;
	}
}

/* Anonymous Page */
void 
hash_page_destroy (struct hash_elem *elem, void *aux) {
	struct page *page = hash_entry (elem, struct page, hash_elem);
	destroy (page);
	free (page);
}