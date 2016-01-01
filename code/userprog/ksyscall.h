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

void SysHalt()
{
    kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
    return op1 + op2;
}

int SysCreate(char *filename)
{
    // return value
    // 1: success
    // 0: failed
    return kernel->interrupt->CreateFile(filename);
}

int SysOpen(char *filename)
{
    // 1: success
    // 0: failed
    return kernel->interrupt->OpenFile(filename);
}

int SysWrite(char *buffer, int size, OpenFileId id)
{
    // return value: number of byte written
    return kernel->interrupt->WriteFile(buffer, size, id);    
}

int SysRead(char *buffer, int size, OpenFileId id)
{
    return kernel->interrupt->ReadFile(buffer, size, id);
}

int SysClose(OpenFileId id)
{
    return kernel->interrupt->CloseFile(id);
}

void SysPrintInt(int num)
{
    kernel->interrupt->PrintInt(num);
}

void SysYield()
{
    kernel->interrupt->Yield();
}

void SysSleep(int tick)
{
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
