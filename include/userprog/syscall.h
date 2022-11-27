#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/interrupt.h"

void syscall_init (void);
bool use_file_read_lock;
int file_read_lock_depth;

/*Project 2 User Program*/
struct lock filesys_lock;
#endif /* userprog/syscall.h */
