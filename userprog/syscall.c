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

#include <stdlib.h>

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
/* Project 2 User Memory Access 추가 */
void check_address(void *addr);

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

#define F_RAX  f->R.rax
#define F_ARG1  f->R.rdi
#define F_ARG2  f->R.rsi
#define F_ARG2  f->R.rdx
#define F_ARG3  f->R.r10
#define F_ARG4  f->R.r8
#define F_ARG5  f->R.rd9


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

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
    

    struct system_call call = syscall_list[F_RAX];
    if(call.syscall_num == F_RAX) {
        call.function(f);
    }
}


void halt_handler(struct intr_frame *f) {
    power_off();
}

void exit_handler(struct intr_frame *f) {
    int status = args->arg1;
    thread_current()->process_status = status;
    thread_exit ();
}

void fork_handler(struct intr_frame *f) {

}

void exec_handler(struct intr_frame *f) {
    char *file = args->arg1;

}

void wait_handler(struct intr_frame *f) {

}

void create_handler(struct intr_frame *f) {
    char *file = args->arg1;
    unsigned initial_size = args->arg2;
    check_address(file);
    if (filesys_create(file, initial_size)){
        return true;
    }
    else{
        return false;
    }
}

void remove_handler(struct intr_frame *f) {
    char *file = args->arg1;
    check_address(file);
    if (filesys_remove(file)){
        return true;
    }
    else{
        return false;
    }
}

void open_handler(struct intr_frame *f) {
    char *file = args->arg1;
    check_address(file);
    if (filesys_open(file)){
        return true;
    }
    else{
        return false;
    }
}

void filesize_handler(struct intr_frame *f) {

}

void read_handler(struct intr_frame *f) {

}

void write_handler(struct intr_frame *f) {
    int fd = args->arg1;
    char *buffer = args->arg2;
    unsigned size = args->arg3;
    printf("%s", buffer);
}

void seek_handler(struct intr_frame *f) {

}

void tell_handler(struct intr_frame *f) {

}

void close_handler(struct intr_frame *f) {

}

void mmap_handler(struct intr_frame *f) {

}

void mnumap_handler(struct intr_frame *f) {

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

void check_address(void *addr){
    struct thread *curr = thread_current();

    if (!is_user_vaddr(addr) || addr == NULL ||
        pml4_get_page(curr->pml4, addr) == NULL)
    {
        exit_handler(-1);
    }
}