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

#include <stdlib.h>

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
/* Project 2 User Memory Access 추가 */
bool
address_check(struct intr_frame *f, char *ptr);
int add_file_to_fd_table(struct file *file);
void kern_exit(struct intr_frame *f, int status);
struct file *fd_to_struct_file(int fd);
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

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
    /* project 2 User Program*/
    lock_init(&filesys_lock);
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
    int status = F_ARG1;
    thread_current()->process_status = status;
    thread_exit ();
}

void fork_handler(struct intr_frame *f) {

}

void exec_handler(struct intr_frame *f) {
    char *file = F_ARG1;
    write_handler(f);

}

void wait_handler(struct intr_frame *f) {

}

void create_handler(struct intr_frame *f) {
    char *file = F_ARG1;
    unsigned initial_size = F_ARG2;
    address_check(f,file);
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
    if (filesys_remove(file)){
        F_RAX = true;
    }
    else{
        F_RAX = false;
    }
}

void open_handler(struct intr_frame *f) {
    char *file = F_ARG1;
    address_check(f,file);
    struct file *file_obj =filesys_open(file);
    if (file_obj == NULL){
        F_RAX = -1;
        return F_RAX;
    }
    int fd = add_file_to_fd_table(file_obj);

    // 만약에 파일을 열 수 없으면 -1을 받는다
    if (fd == -1){
        file_close(file_obj);
    }
    F_RAX = fd;
    return F_RAX;
}

void filesize_handler(struct intr_frame *f) {
    int *fd =F_ARG1;
    struct file *fileobj = fd_to_struct_file(fd);
    F_RAX = file_length(fileobj);

}

void read_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    void *buffer = F_ARG2;
    unsigned size = F_ARG3;
    address_check(f,buffer);
    address_check(f,buffer+size-1);
    int read_count;
    struct file *fileobj = fd_to_struct_file(fd);
    if (fileobj ==NULL){
        F_RAX = -1;
        return F_RAX;
    }
    // STDIN 일때
    if (fd == 0){
        char key;
        for (read_count = 0; read_count<size;read_count++){
            key = input_getc();
            if(key == '\0'){
                break;
            }
        }
    }
    else if(fd == 1){
        F_RAX = -1;
        return F_RAX;
    }
    else{
        lock_acquire(&filesys_lock);
        read_count = file_read(fileobj,buffer,size);
        lock_release(&filesys_lock);
    }
    F_RAX = read_count;
}

void write_handler(struct intr_frame *f) {
    int fd = (int)F_ARG1;
    char *buffer =(char *) F_ARG2;
    unsigned size = F_ARG3;
    address_check(f,buffer);
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
        F_RAX = -1;
        return F_RAX;
    }
    // else if (fd < 0){
    //     kern_exit(f,-1);
    // }
    else{
        lock_acquire(&filesys_lock);
        read_count = file_write(fileobj, buffer, size);
        lock_release(&filesys_lock);
        F_RAX = read_count;
    }
    // printf("%s", buffer);
}

void seek_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    unsigned position = F_ARG2;
    struct file *fileobj = fd_to_struct_file(fd);
    file_seek(fileobj, position);
}

void tell_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    struct file *fileobj = fd_to_struct_file(fd);
    F_RAX = file_tell(fileobj);
}

void close_handler(struct intr_frame *f) {
    int fd = F_ARG1;
    struct file *fileobj = fd_to_struct_file(fd);
    struct thread *curr = thread_current();
    struct file **fdt = curr->file_descriptor_table;
    // printf("fdt[fd]를---- %p -----보고 싶어요\n",fdt[fd]);
    // file_close(fileobj);

    // printf("<<<<<<<<<<<<<>>>><<><><><><><><><><<>>>>>>>>>>>\n");
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

bool
address_check(struct intr_frame *f, char *ptr) {
    if(pml4_get_page(thread_current()->pml4, ptr) == NULL) kern_exit(f,-1);
    // return pml4_get_page(thread_current()->pml4, ptr) != NULL;
}

int add_file_to_fd_table(struct file *file){
    struct thread *curr = thread_current();
    struct file **fdt = curr->file_descriptor_table;
    int fd = curr->fdidx;   // fd의 값은 2부터 출발

    while(curr->file_descriptor_table[fd] != NULL && fd < FDT_COUNT_LIMIT){
        fd++;
    }

    if (fd >= FDT_COUNT_LIMIT){
        return -1;
    }
    curr -> fdidx = fd;
    fdt[fd] = file;
    return fd;
}

struct file *fd_to_struct_file(int fd){
    if (fd<0 || fd >= FDT_COUNT_LIMIT){
        return NULL;
    }
    struct thread *curr = thread_current();
    struct file **fdt = curr->file_descriptor_table;

    struct file *file = fdt[fd];
    return file;
}

void kern_exit(struct intr_frame *f, int status){
    F_ARG1 = status;
    exit_handler(f);
    NOT_REACHED();
}
