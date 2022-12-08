/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "vm/uninit.h"
#include "kernel/hash.h"
#include "include/threads/mmu.h"
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
	// printf("%p  이게 upage 주소입니다\n",upage);
	
	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page(spt,upage)== NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. 
		 * page를 생성하고, VM type에 맞는 initilier를 가져온다
		 * 그리고, uninit_new를 call하여 "uninit 페이지 구조체를 만든다
		 * uninit_new를 call한 후에 field를 수정한다."*/
		/* Create page */
		struct page *page = (struct page*)malloc(sizeof(struct page));
		/* fetch the initializer according to the VM type */
		switch(VM_TYPE(type)){
			case VM_ANON:
				uninit_new (page, upage, init, type, aux,anon_initializer);
				break;
			case VM_FILE:
				uninit_new (page, upage, init, type, aux,file_backed_initializer);
				break;
		}
		// printf("page->va = %p     upage = % p  \n", page->va, upage);
		
		/* TODO: Insert the page into the spt. 
		 * spt에 페이지를 삽입한다*/
		page->writable = writable;
		spt_insert_page(spt,page);
		return true;

		// if(spt_insert_page(spt,page)==NULL){
		// 	goto err;
		// }
		// else{
		// 	return true;
		// }

	}
err:
	// printf("이번 내리실 역은 vm_alloc_page_with_iniaiaiaiai eeerrrooor\n");
	return false;
}


	// /* page not initialized */
	// VM_UNINIT = 0,
	// /* page not related to the file, aka anonymous page */
	// VM_ANON = 1,
	// /* page that realated to the file */
	// VM_FILE = 2,

/* Find VA from spt and return page. On error, return NULL. */
/* Project 3 Virtual Memory */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	// !! free중요
	struct page *page = (struct page*)malloc(sizeof(struct page));
	/* TODO: Fill this function. */
	struct hash_elem *hash_find_elem;
	// struct page *page;
	page->va = pg_round_down(va);
	hash_find_elem = hash_find(&spt->hash, &page->hash_elem);
	free(page);
	if (hash_find_elem ==NULL){
		return NULL;
	}
	else{
		return hash_entry(hash_find_elem,struct page, hash_elem);
	}

	// struct list *page_list = &spt->page_list;
	// struct list_elem *find_elem = list_begin(page_list);

	// while(find_elem != list_end(page_list)){
	// 	struct page_table_node *find_page = list_entry(find_elem,struct page_table_node,page_list_elem);	
	// 	if (find_page->page->va == va){
	// 		// printf("이번 내리실 역은 spt_find_page 입니다 \n");
	// 		return find_page->page;
	// 	}	
	// 	find_elem = list_next(find_elem);
	// }

	// printf("이번 내리실 역은 spt_find_page 입니다 \n");
	// return NULL;
}

/* Insert PAGE into spt with validation. */
/* Project 3 Virtual Memory */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	// printf("spt_insert_page가 되고 있어요\n");
	// struct hash_elem *insert_elem = &page->hash_elem;
	if (!hash_insert(&spt->hash, &page->hash_elem)){
		succ = true;
		return succ;
	}
	else{
		return succ;
	}




	// struct page_table_node *insert_page = (struct page_table_node *)malloc(sizeof(struct page_table_node));
	// insert_page->page = page;
	// list_push_front(&spt->page_list,&insert_page->page_list_elem);


	// printf("이것은 page가 맞긴 한건가요?  %p\n", page->va);
	// return succ;
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

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	// !! free중요
	struct frame *frame = (struct frame *)malloc(sizeof(struct frame));
	/* TODO: Fill this function. */
	// struct frame *new_frame = palloc_get_page(PAL_USER);

	frame->page=NULL;
	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);
	// printf("kva 주소값 %p\n", frame->kva);
	/* user pool memory가 꽉찼을 때  frame을 하나 swap으로 보내주고, disk에서 그만큼의 메모리를 가져와
	   frame에 가져온다 우선 PANIC("todo")로 구현*/
	// PANIC("todo");
	if (frame == NULL || frame->kva == NULL){
		PANIC("todo");
	}

	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	
	return frame;
}

// /* The representation of "frame" */
// struct frame {
// 	void *kva;
// 	struct page *page;
// };

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
	// !! free 중요
	/* 인풋으로 들어온 fault address와 똑같은 페이지가 있는지 확인한다*/
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	struct hash_elem hash_elem_fault;

	if(spt_find_page(spt,addr)==NULL){
		// printf("spt == null 로 빠졌을 경우\n");
		return;
	}
	else{
		page = spt_find_page(spt,addr);
		// printf("vm_try_handle_fault에서 page->va  =  %p\n",page->va);
		return vm_do_claim_page (page);
	}
	// struct hash_elem *fault_hash_elem;
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
	printf("vm_cliam_page 으로 들어와서 무언가를 수정해야하나\n");
	/* TODO: Fill this function */
	page = spt_find_page(&thread_current()->spt,va);
	if (page == NULL){
		return false;
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
/* 아래 함수는 인자로 주어진 page에 물리 메모리 프레임을 할당합니다. 
 * 당신은 먼저 vm_get_frame 함수를 호출함으로써 프레임 하나를 얻습니다
 * (이 부분은 스켈레톤 코드에 구현되어 있습니다). 그 이후 당신은 MMU를 
 * 세팅해야 하는데, 이는 가상 주소와 물리 주소를 매핑한 정보를 페이지 테이블에
 * 추가해야 한다는 것을 의미합니다.
 * 위의 함수는 앞에서 말한 연산이 성공적으로 수행되었을 경우에 true를 반환하고
 *  그렇지 않을 경우에 false를 반환합니다. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	struct thread *curr = thread_current();
	/* Set links */
	frame->page = page;
	page->frame = frame;
	// printf_hash(&thread_current()->spt);	
	// printf("page -> va : %X\n", page->va);
	// printf("page -> frame ->kva : %p\n", frame->kva); 

	// printf("네가 원하는 것이 do_claim_page 인것이냐?? 마구니가 꼈구나\n");
	// printf("page->va 값을 알고 싶다  %p\n",page->va);
	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* 페이지 테이블 엔트리를 삽입해라,, 페이지의 가상주소를 프레임의 물리주소에 매핑시켜주기위해 */
	if(pml4_set_page(curr->pml4,page->va,frame->kva,true)){
		// printf("page -> va : %X\n", page->va);
		// printf("page -> frame ->kva : %X\n", page->frame->kva); 
		// printf_hash(&thread_current()->spt);	
		// printf("plm4setpage가 제대로 됬는지 확인 필요\n");
		return swap_in (page, frame->kva);
		// return true;
	}
	else{
		return false;
	}
		
// 	bool
// pml4_set_page (uint64_t *pml4, void *upage, void *kpage, bool rw)
}

/* Initialize new supplemental page table */
/* Project 3 Virtual Memory */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->hash,page_hash,page_less, NULL);
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

void
printf_hash(struct supplemental_page_table *spt){
	struct hash *h = &spt->hash;
	struct hash_iterator i;
   	hash_first (&i, h);
	printf("===== hash 순회시작 =====\n");
   	while (hash_next (&i))
   	{
		struct page *p = hash_entry(hash_cur(&i), struct page, hash_elem);
		if (p->frame == NULL){
			printf("va: %X, p_addr : %X\n",p->va, p);
		}
		else {
			printf("va: %X, kva : %X, p_addr : %X\n",p->va,p->frame->kva, p);
		}
   	}
	printf("===== hash 순회종료 =====\n");
}