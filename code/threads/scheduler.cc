// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

#define LEVEL_SJF 2
#define LEVEL_PRIORITY 1 
#define LEVEL_RR 0

//---------------------------------------------------------------------
// CompareThreadID
//--------------------------------------------------------------------
int CompareThreadID(Thread *t1, Thread *t2)
{
    int t1ID = t1->getID();
    int t2ID = t2->getID();

    if(t1ID < t2ID)
        return -1;
    else if(t1ID > t2ID)
        return 1;
    else
        return 0;
}

//----------------------------------------------------------------------
// ComaprePriority
//----------------------------------------------------------------------
int ComparePriority(Thread *t1, Thread *t2)
{
    ASSERT(t1 != NULL && t2 != NULL);
    
    // Mult -1 since sorted list always return smallest element
    // but we want higher prirority return first
    int p1 = -t1->getPriority();
    int p2 = -t2->getPriority();
    
    if(p1 < p2)
        return -1;
    else if(p1 > p2)
        return 1;
    else
        return CompareThreadID(t1, t2);
}

//----------------------------------------------------------------------
// ComapreSJF
//----------------------------------------------------------------------
int CompareSJF(Thread *t1, Thread *t2)
{
    ASSERT(t1 != NULL && t2 != NULL);
    
    int g1 = t1->getGuessCPUBurst();
    int g2 = t2->getGuessCPUBurst();
    
    if(g1 < g2)
        return -1;
    else if(g1 > g2)
        return 1;
    else
        return CompareThreadID(t1, t2);
}

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{
    readyList = new List<Thread *>;
    priorityList = new SortedList<Thread *>(ComparePriority);
    sjfList = new SortedList<Thread *>(CompareSJF);

    toBeDestroyed = NULL;
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{
    delete readyList;
    delete priorityList;
    delete sjfList;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

int
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
    
    ASSERT(thread->priority >= 0 && thread->priority < 150);

    int tlevel = thread->priority / LEVEL_GAP;
    thread->lastCPUTick = kernel->stats->totalTicks;

    switch(tlevel)
    {
        case LEVEL_RR:
            readyList->Append(thread);
            break;
        case LEVEL_PRIORITY:
            priorityList->Insert(thread);
            break;
        case LEVEL_SJF:
            sjfList->Insert(thread);
            break;
        default:
            ASSERTNOTREACHED();
            break;
    }

    fprintf(logFile, "Tick %d: Thread %d is inserted into queue L%d (EST: %lf, PRI: %d)\n",
      kernel->stats->totalTicks,
      thread->getID(),
      3 - tlevel,
      thread->getGuessCPUBurst(),
      thread->getPriority());

    thread->setStatus(READY);
    
    // only preempt when we are in interrupt handler
    if(kernel->currentThread != thread && isPreempted(kernel->currentThread, thread))
    {
        kernel->interrupt->YieldOnReturn();
    } 

    return tlevel;
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    // Choose a queue
    int printLevel = 0;
    List<Thread *> *selectedList = NULL;
    if(!sjfList->IsEmpty())
    {
        printLevel = 1;
        selectedList = sjfList;  
    }
    else if(!priorityList->IsEmpty())
    {
        printLevel = 2;
        selectedList = priorityList;
    }
    else if(!readyList->IsEmpty())
    {
        printLevel = 3;
        selectedList = readyList;
    }

    // Pop element
    if (selectedList == NULL)
    {
        return NULL;
    }
    else
    {
        Thread *thread = selectedList->RemoveFront();
        fprintf(logFile, "Tick %d: Thread %d is removed from queue L%d (EST: %lf, PRI: %d)\n",
          kernel->stats->totalTicks,
          thread->getID(),
          printLevel,
          thread->getGuessCPUBurst(),
          thread->getPriority());

        return thread;
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;

    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing)  	// mark that we need to delete current thread
    {
        ASSERT(toBeDestroyed == NULL);
        toBeDestroyed = oldThread;
    }

    if (oldThread->space != NULL)  	// if this thread is a user program,
    {
        oldThread->SaveUserState(); 	// save the user's CPU registers
        oldThread->space->SaveState();
    }

    oldThread->CheckOverflow();		    // check if the old thread
                                            // had an undetected stack overflow
    kernel->currentThread = nextThread;  // switch to the next thread
    
    nextThread->setStatus(RUNNING);      // nextThread is now running
    nextThread->lastCPUTick = kernel->stats->totalTicks; // Mark IN CPU Tick

    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread 
    
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    DEBUG(dbgThread, "Now in thread: " << oldThread->getName() << " Last Tick: " << kernel->currentThread->lastCPUTick);

    CheckToBeDestroyed();		// check if thread we were running
    
    // before this one has finished
    // and needs to be cleaned up

    if (oldThread->space != NULL)  	    // if there is an address space
    {
        oldThread->RestoreUserState();     // to restore, do it.
        oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL)
    {
        DEBUG(dbgThread, "Destroy thread: " << toBeDestroyed->getName());
        delete toBeDestroyed;
        toBeDestroyed = NULL;
    }
}

//----------------------------------------------------------------------
// Scheduler::Aging
//----------------------------------------------------------------------
void
Scheduler::Aging()
{
    List<Thread *> *totalList[] = {readyList, priorityList, sjfList};

    for(int i = 0; i < 3; i++)
    {
        for(ListIterator<Thread *> it(totalList[i]); !it.IsDone();)
        {
            Thread *t = it.Item();
            ASSERT(t != NULL);
             
            if(kernel->stats->totalTicks - t->lastCPUTick >= 1500)
            {
                int p = t->getPriority();
                int oldPriority = p;
                t->setPriority((p = p + 10) < 150 ? p : (p = 149));
                
                it.Next();

                fprintf(logFile, "Tick %d: Thread %d changes its priority from %d to %d\n",
                      kernel->stats->totalTicks,
                      t->getID(),
                      oldPriority,
                      p);
                
                // Ignore thread which priority is less than 50
                if(t->getPriority() >= LEVEL_GAP)
                {
                    totalList[i]->Remove(t);
                    ReadyToRun(t);
                }
                else
                {
                    // No reinsert to queue, so we reset lastCPUTick here
                    t->lastCPUTick = kernel->stats->totalTicks;
                }
            }
            else
            {
                it.Next();
            }
        }
    }    
}

//----------------------------------------------------------------------
// Scheduler::Demote
//
//----------------------------------------------------------------------
void
Scheduler::Demote()
{
    int burst;

    // if cpu burst larger than the demote limit, lower its priority
    if((burst = kernel->stats->totalTicks - kernel->currentThread->lastCPUTick) >= DEMOTE_LIMIT_TICK)
    {
        kernel->currentThread->lastCPUTick = kernel->stats->totalTicks;
        kernel->currentThread->cpuBurst += burst;
        
        int tlevel = kernel->currentThread->priority / LEVEL_GAP;
        if(tlevel > 0)
        {
            int oldPriority = kernel->currentThread->priority;
            kernel->currentThread->priority = (tlevel) * LEVEL_GAP - 1;
            kernel->interrupt->YieldOnReturn();
            
            fprintf(logFile, "Tick %d: Thread %d changes its priority from %d to %d\n",
                      kernel->stats->totalTicks,
                      kernel->currentThread->getID(),
                      oldPriority,
                      kernel->currentThread->priority);
        }
    }
}


//----------------------------------------------------------------------
// Scheduler::isPreempted
//
//----------------------------------------------------------------------
bool Scheduler::isPreempted(Thread *cur, Thread *preempt)
{
    ASSERT(cur != NULL && preempt != NULL);

    static int L1_LOWERBOUND = LEVEL_GAP * (3 - 1);

    if(cur->getPriority() >= L1_LOWERBOUND && preempt->getPriority() >= L1_LOWERBOUND)
        return CompareSJF(preempt, cur) < 0; // true: preempt guess < cur guess or preempt id < cur id    
    else
        return ComparePriority(preempt, cur) < 0; // true: preeempt pri > cur pri or preempt id < cur id
    
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

