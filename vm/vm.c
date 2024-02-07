/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"

/* Memory Management */
#include "hash.h"
#include "threads/vaddr.h"

/* Memory management */
struct list frame_table;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();

	/* Memory Management */
	// hash_init (vm_hash_func(vm_less_func()));

#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
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
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */

		/* TODO: Insert the page into the spt. */
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
		return page;
	}

	page = hash_entry (spt_elem, struct page, hash_elem);

	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;

	/* Memory Management */
	struct hash_elem *e = hash_find (&spt->hash_page, &page->hash_elem);
	if (e != NULL) {
		return succ;
	}

	hash_insert (&spt->hash_page, &page->hash_elem);

	succ = true;

	return succ;
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
	
	/* TODO: The policy for eviction is up to you. */
	/* Memeory Management */
	victim = list_entry (list_pop_front (&frame_table), struct frame, frame_elem);
	
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	/* Memory Management */
	if (victim->page != NULL) {
		swap_out (victim->page);
	}

	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	/* Memory Management */
	struct frame *frame = NULL;
	void *kva = palloc_get_page (PAL_USER);

	if (kva == NULL) {
		frame = vm_evict_frame ();
	}
	else {
		frame = malloc (sizeof (struct frame));
		frame->kva = kva;
	}

	ASSERT (frame != NULL);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */

	return vm_do_claim_page (page);
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

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* Memory Management */
	struct thread *curr = thread_current ();
	// pml4_set_page (curr->pml4, page->va, frame->kva, page->writable);
	
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
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
}

/* Memory Management */
/* vm_entry의 vaddr을 인자값으로 hash_int ()함수를 사용해서 해시 값 반환 */
static unsigned
vm_hash_func (const struct hash_elem *e, void *aux) {
	struct page *page = hash_entry (e, struct page, hash_elem);
	
	return hash_bytes (&page->va, sizeof (page->va));
}

/* Memory Management */
/* 입력된 hash_elem의 vaddr을 비교해주는 함수 */
static bool
vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED) {
	const struct page *page_a = hash_entry (a, struct page, hash_elem);
	const struct page *page_b = hash_entry (b, struct page, hash_elem);

	return page_a->va < page_b->va;
}

/* Memory Management */
/* supplemental_page_table에 page insert */
bool
insert_vm_page (struct hash *pages, struct page *p) {
	if (!hash_insert (pages, &p->hash_elem)) {
		return true;
	}
	else {
		return false;
	}
}

/* Memory Management */
/* supplemental_page_table에 page delete */
bool
delete_vm_page (struct hash *pages, struct page *p) {
	if (!hash_delete (pages, &p->hash_elem)) {
		return true;
	}
	else {
		return false;
	}
}

