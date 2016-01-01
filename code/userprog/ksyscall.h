/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"
#include "syscall.h"

void SysHalt()
{
    kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
    return op1 + op2;
}

#ifndef FILESYS_STUB
int SysCreate(char *filename, int size)
{
    // return value
    // 1: success
    // 0: failed
    return kernel->interrupt->IntCreateFile(filename, size);
}

OpenFileId SysOpen(char *filename)
{
    // > 0: ok
    // other: fail
    return kernel->interrupt->IntOpenFile(filename);
}

int SysRead(char *buf, int size, OpenFileId id)
{
    // num of bytes read
    return kernel->interrupt->IntReadFile(buf, size, id);
}

int SysWrite(char *buf, int size, OpenFileId id)
{
    // num of bytes written
    return kernel->interrupt->IntWriteFile(buf, size, id);
}

int SysClose(OpenFileId id)
{
    // always 1
    return kernel->interrupt->IntCloseFile(id);
}

#endif

#endif /* ! __USERPROG_KSYSCALL_H__ */
