/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "vm/uninit.h"
#include "kernel/hash.h"
#include "include/threads/mmu.h"
#include "userprog/process.h"
/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */

struct list frame_table;
struct list_elem *start;

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

	list_init(&frame_table);
	start = list_begin(&frame_table);
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
		// printf("find == nULL \n\n");
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
	// int succ = false;
	/* TODO: Fill this function. */
	// printf("spt_insert_page가 되고 있어요\n");
	// struct hash_elem *insert_elem = &page->hash_elem;
	if (hash_insert(&spt->hash, &page->hash_elem) == NULL){
		// printf("insert true\n");
		// succ = true;
		return true;
	}
	else{
		// printf("insert false\n");
		return false;
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
	 struct thread *curr = thread_current();
	 struct list_elem *e =start;

	for(start = e; start != list_end(&frame_table);start=list_next(start)){
		victim = list_entry(start, struct frame, frame_elem);
		if(pml4_is_accessed(curr->pml4,victim->page->va)){
			pml4_set_accessed(curr->pml4, victim->page->va,0);
		}
		else{
			return victim;
		}
	}

	 for(start = list_begin(&frame_table); start != e; start = list_next(start)){
		victim = list_entry(start, struct frame, frame_elem);
		if(pml4_is_accessed(curr->pml4,victim->page->va)){
			pml4_set_accessed(curr->pml4,victim->page->va,0);
		}
		else{
			return victim;
		}
	 }
	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	if(victim !=NULL){
		swap_out(victim->page);
	}

	return victim;
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

	frame->kva = palloc_get_page(PAL_USER | PAL_ZERO);
	// printf("kva 주소값 %p\n", frame->kva);
	/* user pool memory가 꽉찼을 때  frame을 하나 swap으로 보내주고, disk에서 그만큼의 메모리를 가져와
	   frame에 가져온다 우선 PANIC("todo")로 구현*/
	// PANIC("todo");
	// if (frame == NULL || frame->kva == NULL){
	// 	PANIC("todo");
	// }
	if(frame->kva == NULL){
		frame = vm_evict_frame();
		frame->page = NULL;
		return frame;
	}

	list_push_back(&frame_table,&frame->frame_elem);

	frame->page=NULL;

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
	/* stack growth를 구현해야하는 이유 
	 * userprog에서의 stack은 | USER_STACK 에서 시작하는 단일 페이지
	 * 프로그램의 크기는 4KB 로 제한이었지만 --> 스택이 현재 크기를 추과하면 필요에 따라 페이지 할당
	 * 1. 추가 페이지는 스택에 접근하는 경우에만 할당
	 * 2. 스택에 접근하는 경우와 아닌경우 구별하여 구현
	 * 3. userprogram의 스택 포인터의 현재 값을 얻을 수 있어야함
	 * 4. struct intr_frame구조체의 멤버 rsp를 통해 알 수 있다
	 * 5. 커널에서의 page fault가 일어날 때도 처리*/
	if(vm_alloc_page(VM_ANON | VM_MARKER_0, addr, 1)){
		vm_claim_page(addr);
		thread_current()->stack_bottom -= PGSIZE;
	}


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
	/* 인풋으로 들어온 fault address와 똑같은 페이지가 있는지 확인한다*/
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	struct hash_elem hash_elem_fault;

	/* 유저쪽의 주소여야 한다 */
	if(is_kernel_vaddr(addr)){
		return false;
	}
	void *rsp_stack = is_kernel_vaddr(f->rsp) ? thread_current()->stack_rsp : f->rsp;


	if(not_present){
		if(spt_find_page(spt,addr)==NULL){
			if(rsp_stack - 8 <= addr && USER_STACK - 0x100000 <= addr && addr <= USER_STACK){
				vm_stack_growth(thread_current()->stack_bottom - PGSIZE);
				return true;
			}
			return false;		
		}
		else{
			page = spt_find_page(spt,addr);
			return vm_do_claim_page (page);
			// return true;
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
	// printf("vm_cliam_page 으로 들어와서 무언가를 수정해야하나\n");
	/* TODO: Fill this function */
	page = spt_find_page(&thread_current()->spt,va);
	if (page == NULL){
		// printf("get frame false\n\n");
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
	if(pml4_set_page(curr->pml4,page->va,frame->kva,page->writable)){
		// printf("page -> va : %X\n", page->va);
		// printf("page -> frame ->kva : %X\n", page->frame->kva); 
		// printf_hash(&thread_current()->spt);	
		// printf("plm4setpage가 제대로 됬는지 확인 필요\n");
		return swap_in (page, frame->kva);
		// return true;
	}
	else{
		PANIC("fail pml4 set page !!!!!!!!");
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
	/* src의 spt를 dst로 복사해준다. 
	 * 부모 쓰레드의 spt에 있는 page들을 다 돌아서 복사해줘야한다
	 * 기존에 spt테이블에 썼던것 처럼 uninit 페이지를 할당해주어야 한다
	 * copy that*/
	struct hash *src_hash = &src->hash;
	struct hash *dst_hash = &dst->hash;  
	struct hash_iterator i;
	hash_first(&i,src_hash);
	while(hash_next(&i)){
		struct page *src_page = hash_entry(hash_cur(&i),struct page, hash_elem);
		enum vm_type src_type = page_get_type(src_page);	//src_page의 type을 불러온다. 즉, 부모의 페이지 타입을 불러온다
		void *src_page_va = src_page->va;				// src_page의 가상주소. 즉, 부모 페이지의 가상주소
		bool src_writable = src_page->writable;
		vm_initializer *src_init = src_page->uninit.init;		// src_page의 uninit의 init 은 lazy_load_segment 이다.
		// struct send_data_via *aux = (struct send_data_via *)malloc(sizeof(struct send_data_via)); 
		// struct send_data_via *src_aux = (struct send_data_via *)src_page->uninit.aux;
		void *aux = src_page->uninit.aux;
		// printf("copy에 들어오나요?? \n");
		// if(aux ==NULL){
		// 	return false;
		// }
		// if(src_aux != NULL){
		// 	aux->file = src_aux->file;
		// 	aux->position = src_aux->position;
		// 	aux->page_read_bytes = src_aux->page_read_bytes;
		// 	aux->page_zero_bytes = src_aux->page_zero_bytes;
		// }	
		if(src_page->uninit.type & VM_MARKER_0){
			setup_stack(&thread_current()->tf);
		}

		else if(src_page->operations->type == VM_UNINIT){
			if(!vm_alloc_page_with_initializer(src_type,src_page_va,src_writable,src_init,aux)){
				return false;
			}			
		}
		else{
			if(!vm_alloc_page(src_type,src_page_va,src_writable)){
				return false;
			}
			if(!vm_claim_page(src_page_va)){
				return false;
			}
		}


		if(src_page->operations->type != VM_UNINIT){
			struct page *dst_page = spt_find_page(dst,src_page_va);
			// if(dst_page == NULL){
			// 	return false;
			// }
			memcpy(dst_page->frame->kva,src_page->frame->kva,PGSIZE);
		}
	
		// printf("copy안에 들어오는지 확인해봅니다\n");

		// struct page *src_page = hash_entry(hash_cur(&i), struct page, hash_elem);
		// vm_initializer *init = NULL;


		// switch (VM_TYPE(src_page->uninit.type))
		// {
		// case VM_UNINIT:
		// 	init = src_page->uninit.init;
		// 	struct send_data_via *send_aux = (struct send_data_via *)malloc(sizeof(struct send_data_via));
		// 	memcpy(send_aux,src_page->uninit.aux,sizeof(struct send_data_via));
		// 	if(!vm_alloc_page_with_initializer(src_page->uninit.type,src_page->va,src_page->writable,init,send_aux)){
		// 		return false;
		// 	}
		// 	break;
		// case VM_ANON:
		
		// 	vm_alloc_page(VM_ANON,src_page->va,true);
		// 	vm_claim_page(src_page->va);
		// 	struct page *dst_page = spt_find_page(dst,src_page->va);
		// 	memcpy(dst_page->frame->kva,src_page->frame->kva,PGSIZE);
		// }
		// 	break;
		
		}
	
	return true;
}

	


/* Free the resource hold by the supplemental page table 김지수 멍청이 */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	// struct hash_iterator i;
	// hash_first(&i,&spt->hash);
	// while(hash_next(&i)){
	// 	struct page *kill_page = hash_entry(hash_cur(&i), struct page, hash_elem);
	// 	if(kill_page->operations->type == VM_FILE){
	// 		do_munmap(kill_page->va);
	// 	}
	// }
		
	hash_destroy(&spt->hash,spt_free_destroy);

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