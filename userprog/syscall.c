#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/palloc.h"
#include "lib/user/syscall.h"
#include "vm/vm.h"

#include <stdlib.h>

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
/* Project 2 User Memory Access 추가 */
bool
address_check(struct intr_frame *f, char *ptr);
int add_file_to_fd_table(struct file *file);
void kern_exit(struct intr_frame *f, int status);
struct file *fd_to_struct_file(int fd);
tid_t
process_fork (const char *name, struct intr_frame *if_ UNUSED);
void remove_file_from_fdt(int fd);
void close_handler_usefd(int fd);
bool is_writable;
void check_valid_buffer(void *buffer, unsigned size, struct intr_frame *f, bool is_writable);
void kernel_writable_check(struct intr_frame *f, void *addr);
struct page *check_address(struct intr_frame *f, char *ptr);

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

#define SYSCALL_CNT 25

#define F_RAX f->R.rax
#define F_ARG1 f->R.rdi
#define F_ARG2 f->R.rsi
#define F_ARG3 f->R.rdx
#define F_ARG4 f->R.r10
#define F_ARG5 f->R.r8
#define F_ARG6 f->R.r9


struct system_call {
    uint64_t syscall_num;
    void (*function) (struct intr_frame *f);
};

 
void halt_handler(struct intr_frame *f);
void exit_handler(struct intr_frame *f);
void fork_handler(struct intr_frame *f);
void exec_handler(struct intr_frame *f);
void wait_handler(struct intr_frame *f);
void create_handler(struct intr_frame *f);
void remove_handler(struct intr_frame *f);
void open_handler(struct intr_frame *f);
void filesize_handler(struct intr_frame *f);
void read_handler(struct intr_frame *f);
void write_handler(struct intr_frame *f);
void seek_handler(struct intr_frame *f);
void tell_handler(struct intr_frame *f);
void close_handler(struct intr_frame *f);
void mmap_handler(struct intr_frame *f);
void mnumap_handler(struct intr_frame *f);
void chdir_handler(struct intr_frame *f);
void mkdir_handler(struct intr_frame *f);
void readdir_handler(struct intr_frame *f);
void isdir_handler(struct intr_frame *f);
void inumber_handler(struct intr_frame *f);
void symlink_handler(struct intr_frame *f);
void dup2_handler(struct intr_frame *f);
void mount_handler(struct intr_frame *f);
void umount_handler(struct intr_frame *f);


struct system_call syscall_list[] = {
        {SYS_HALT, halt_handler}, 
        {SYS_EXIT, exit_handler},
        {SYS_FORK, fork_handler},
        {SYS_EXEC, exec_handler},
        {SYS_WAIT, wait_handler},
        {SYS_CREATE, create_handler},
        {SYS_REMOVE, remove_handler},
        {SYS_OPEN, open_handler},
        {SYS_FILESIZE, filesize_handler},
        {SYS_READ, read_handler},
        {SYS_WRITE, write_handler},
        {SYS_SEEK, seek_handler},
        {SYS_TELL, tell_handler},
        {SYS_CLOSE, close_handler},
        {SYS_MMAP, mmap_handler},
        {SYS_MUNMAP, mnumap_handler},
        {SYS_CHDIR, chdir_handler},
        {SYS_MKDIR, mkdir_handler},
        {SYS_READDIR, readdir_handler},
        {SYS_ISDIR, isdir_handler},
        {SYS_INUMBER, inumber_handler},
        {SYS_SYMLINK, symlink_handler},
        {SYS_DUP2, dup2_handler},
        {SYS_MOUNT, mount_handler},
        {SYS_UMOUNT, umount_handler}
    };


void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);
    lock_init(&filesys_lock);
    use_file_read_lock = true;

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
    /* project 2 User Program*/
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
    thread_current()->stack_rsp = f->rsp;
    struct system_call call = syscall_list[F_RAX];
    if(call.syscall_num == F_RAX) {
        call.function(f);
    }
}



void halt_handler(struct intr_frame *f) {
    power_off();
}

void exit_handler(struct intr_frame *f) {
    int status = F_ARG1;
    struct thread *curr = thread_current();

    thread_current()->process_status = status;
    printf("%s: exit(%d)\n",curr->name, curr->process_status);
    thread_exit ();
}

void fork_handler(struct intr_frame *f) {
    const char *thread_name = F_ARG1;
    F_RAX = process_fork(thread_name,f);
    
}

void exec_handler(struct intr_frame *f) {
    char *file = F_ARG1;
    // address_check(f,file);
    check_address(f, file);
    int size = strlen(file) + 1;
    char *fn_copy = palloc_get_page(PAL_ZERO);
    if (fn_copy == NULL){
        kern_exit(f,-1);
        // F_RAX = -1;
        // return F_RAX;
    }
    strlcpy(fn_copy,file,size);
    if(process_exec(fn_copy) == -1){
        kern_exit(f,-1);
        // F_RAX = -1;
        // return F_RAX;
    }
}

void wait_handler(struct intr_frame *f) {
    pid_t pid = F_ARG1;
    F_RAX = process_wait(pid);
    return;
}

void create_handler(struct intr_frame *f) {
    char *file = F_ARG1;
    unsigned initial_size = F_ARG2;
    // address_check(f,file);
    check_address(f, file);
    if (file == NULL){
        kern_exit(f,-1);
    }
    if (filesys_create(file, initial_size)){
        F_RAX = true;
    }
    else{
        F_RAX = false;
    }
}

void remove_handler(struct intr_frame *f) {
    char *file = F_ARG1;
    address_check(f,file);
    if(file ==NULL ) return;
    if(strlen(file) == 0) return;

    if (filesys_remove(file)){
        F_RAX = true;
    }
    else{
        F_RAX = false;
    }
}

void open_handler(struct intr_frame *f) {
    // printf("open_handler에 들어왔어요\n");
    char *file = F_ARG1;
    // address_check(f,file);
    check_address(f, file);
    lock_acquire(&filesys_lock);
    struct file *file_obj =filesys_open(file);
    if (strcmp(thread_current()->name,file)==0){
        file_deny_write(file_obj);
    }
    lock_release(&filesys_lock);
    if (file_obj == NULL){
        F_RAX = -1;
        return F_RAX;
    }
    int fd = add_file_to_fd_table(file_obj);
     
    if (fd == -1){
        lock_acquire(&filesys_lock);
        file_close(file_obj);
        lock_release(&filesys_lock);
    }
    F_RAX = fd;
    return F_RAX;
}

void filesize_handler(struct intr_frame *f) {
    int *fd =F_ARG1;
    struct file *fileobj = fd_to_struct_file(fd);
    lock_acquire(&filesys_lock);
    F_RAX = file_length(fileobj);
    lock_release(&filesys_lock);
}

void read_handler(struct intr_frame *f) {
    // printf("read_handler에 들어왔어요\n");
    int fd = F_ARG1;
    void *buffer = F_ARG2;
    unsigned size = F_ARG3;
    struct file *fileobj = fd_to_struct_file(fd);
    struct page *page;
    check_valid_buffer(buffer, size, f, 0);
    kernel_writable_check(f,buffer);
    // address_check(f,buffer); check_valid_buffer(void *buffer, unsigned size, struct intr_frame *f, bool is_writable)
    // address_check(f,buffer+size-1);
    int read_count;

    if (fileobj ==NULL){
        F_RAX = -1;
        return F_RAX;
    }
    if(fd == STDOUT_FILENO){
        F_RAX = -1;
        return F_RAX;
    }
    lock_acquire(&filesys_lock);
    read_count = file_read(fileobj,buffer,size);
    lock_release(&filesys_lock);
    F_RAX = read_count;
}

void write_handler(struct intr_frame *f) {
    int fd = (int)F_ARG1;
    char *buffer =(char *) F_ARG2;
    unsigned size = F_ARG3;
    check_valid_buffer(buffer, size, f, 0);

    // address_check(f,buffer);
    struct file *fileobj = fd_to_struct_file(fd);
    int read_count;

    if (fileobj ==NULL){
        F_RAX = -1;
        return F_RAX;
    }
    if(fd == 1){
        putbuf(buffer, size);
        F_RAX = size;
    }
    else if (fd ==0){
        F_RAX = 0;
        return F_RAX;
    }
    else{
        lock_acquire(&filesys_lock);
        read_count = file_write(fileobj, buffer, size);
        lock_release(&filesys_lock);
    }
    F_RAX = read_count;
}

void seek_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    unsigned position = F_ARG2;
    struct file *fileobj = fd_to_struct_file(fd);
    file_seek(fileobj, position);
}

void tell_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    if(fd<2){
        return;
    }
    struct file *fileobj = fd_to_struct_file(fd);
    // address_check(f,fileobj);
    check_address(f, fileobj);
    F_RAX = file_tell(fileobj);
}

void close_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    struct file *fileobj = fd_to_struct_file(fd);
    struct thread *curr = thread_current();
    struct file **fdt = curr->file_descriptor_table;
    if(fileobj == NULL){
        return ;
    }

    lock_acquire(&filesys_lock);
    file_close(fileobj);
    lock_release(&filesys_lock);
    remove_file_from_fdt(fd);
}

void mmap_handler(struct intr_frame *f) {
    // printf("mmap_handler로 들어옵니댜\n");
    void *addr = F_ARG1;
    size_t length = F_ARG2;
    int writable = F_ARG3;
    int fd = F_ARG4;
    off_t offset = F_ARG5;

    struct file *file = fd_to_struct_file(fd);
    // printf(" file  1 =   %X   \n",file);
    if(file == NULL){
        // printf(" file  2 =   %X   \n",file);
        kern_exit(f,-1);
        // F_RAX = NULL;
    }
    if(addr + length == 0){
        F_RAX = NULL;
        return F_RAX;
    }

    if(offset % PGSIZE !=0){
        // printf(" file  3 =   %X   \n",file);
        // kern_exit(f,-1);
        F_RAX = NULL;
        return F_RAX;
    }

    if(pg_round_down(addr) != addr || is_kernel_vaddr(addr)){
        // printf(" file  4 =   %X   \n",file);
        // kern_exit(f,-1);
        F_RAX = NULL;
        return F_RAX;
    }
    if(spt_find_page(&thread_current()->spt,addr)){
        // printf(" file  5 =   %X   \n",file);
        // kern_exit(f,-1);
        F_RAX = NULL;
        return F_RAX;
    }

    if(addr == NULL || (long long)length ==0){
        // printf(" file  6 =   %X   \n",file);
        // kern_exit(f,-1);
        F_RAX = NULL;
        return F_RAX;
    }

    if(fd == STDIN_FILENO || fd == STDOUT_FILENO){
        // printf(" file  7 =   %X   \n",file);
        kern_exit(f,-1);
    }
    // printf(" file  8 =   %X   \n",file);
    void *ret = do_mmap(addr,length,writable,file, offset);
    // printf(" file  9 =   %X   \n",file);
    F_RAX = ret;
}

void mnumap_handler(struct intr_frame *f) {
    void *addr = F_ARG1;
    do_munmap(addr);
}

void chdir_handler(struct intr_frame *f) {

}

void mkdir_handler(struct intr_frame *f) {

}
void readdir_handler(struct intr_frame *f) {
    
}

void isdir_handler(struct intr_frame *f) {
    
}
void inumber_handler(struct intr_frame *f) {
    
}
void symlink_handler(struct intr_frame *f) {
    
}
void dup2_handler(struct intr_frame *f) {
    
}
void mount_handler(struct intr_frame *f) {
    
}
void umount_handler(struct intr_frame *f) {
}

bool
address_check(struct intr_frame *f, char *ptr) {
    // struct thread *curr = thread_current();
    // if(spt_find_page(&curr->spt,ptr)->va == ptr){
    //     return;
    // }
    // if(ptr ==NULL || !(is_user_vaddr(ptr))){
    //     kern_exit(f,-1);
    // } 
    // if(spt_find_page(&thread_current()->spt,ptr) == NULL){
    //     kern_exit(f,-1);
    // }

    if(ptr ==NULL || !(is_user_vaddr(ptr)) || pml4_get_page(thread_current() -> pml4, ptr) == NULL){
        // printf("address_check 안으로 들어옵니다\n\n\n");
        kern_exit(f,-1);
    } 
        
}


struct page *check_address(struct intr_frame *f, char *ptr) {
    struct thread *curr = thread_current();
    if(is_kernel_vaddr(ptr) || ptr == NULL || !spt_find_page(&curr->spt, ptr) || !(&curr->pml4)){
        kern_exit(f,-1);
    } 
    return spt_find_page(&curr->spt, ptr);
    // if(ptr ==NULL || !(is_user_vaddr(ptr))){
    //     kern_exit(f,-1);
    // } 
    // return spt_find_page(&thread_current()->spt, ptr);
}

void check_valid_buffer(void *buffer, unsigned size, struct intr_frame *f, bool is_writable){
    for(int i=0; i < size; i++){
        struct page *page = check_address(f,buffer+i);

        if(page ==NULL)
            kern_exit(f,-1);
    
        if(is_writable == true && &page->writable ==false)
            kern_exit(f,-1);
        #ifdef VM
        if( thread_current()->pml4 == &page->frame){
            kern_exit(f,-1);
        }
        #endif
    }
}


int add_file_to_fd_table(struct file *file){
    int fd = 2;   // fd의 값은 2부터 출발
    struct thread *curr = thread_current();

    while(curr->file_descriptor_table[fd] != NULL && fd < FDT_COUNT_LIMIT){
        fd++;
    }

    if (fd >= FDT_COUNT_LIMIT){
        return -1;
    }
    curr->file_descriptor_table[fd] =file;
    return fd;
}

struct file *fd_to_struct_file(int fd){
    if (fd<0 || fd >= FDT_COUNT_LIMIT){
        return NULL;
    }

    struct thread *curr = thread_current();
    return curr->file_descriptor_table[fd];
}

void kern_exit(struct intr_frame *f, int status){
    F_ARG1 = status;
    exit_handler(f);
    NOT_REACHED();
}
void remove_file_from_fdt(int fd){
    struct thread *curr = thread_current();
    if (fd <0 || fd >= FDT_COUNT_LIMIT){
        return;
    }
    curr->file_descriptor_table[fd] = NULL;
}


void close_handler_usefd(int fd) {
	struct file *close_file = fd_to_struct_file(fd);
	if (close_file == NULL){
		return ;
	}

	lock_acquire(&filesys_lock);
	file_close(close_file);
	lock_release(&filesys_lock);
	remove_file_from_fdt(fd);
}


void kernel_writable_check(struct intr_frame *f, void *addr){
    void *find_va = pg_round_down(addr);
    struct page *find_page = spt_find_page(&thread_current()->spt,find_va);
    if (find_page->writable == false){
        kern_exit(f,-1);
    }
}