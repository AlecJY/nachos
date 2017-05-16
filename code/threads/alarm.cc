// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
    threadList = new ThreadList();
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    threadList->ThreadCheck();
    
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();

    // cout << threadList->threadList.empty() << endl;
    // cout << threadList->hasReadyToRun << endl;
    
    if (status == IdleMode && threadList->threadList.empty() && !threadList->hasReadyToRun) {	// is it time to quit?
        if (!interrupt->AnyFutureInterrupts()) {
	    // cout << "!!!Disabled!!!\n";
        timer->Disable();	// turn off the timer
	}
    } else {			// there's someone to preempt
	interrupt->YieldOnReturn();
    }
}

void Alarm::WaitUntil(int x) {
    if (x > 0) {
        ThreadList::ThreadInfo* info = new ThreadList::ThreadInfo();
        info->thread = kernel->currentThread;
        info->x = x+1;
        threadList->threadList.push_back(info);
        (void) kernel->interrupt->SetLevel(IntOff);
        info->thread->Sleep(false);
    }
}

void ThreadList::ThreadCheck() {
    hasReadyToRun = false;
    for (auto ci = threadList.begin(); ci != threadList.end();) {
        (*ci)->x--;
        if ((*ci)->x < 1) {
            kernel->scheduler->ReadyToRun((*ci)->thread);
            ThreadInfo* info = *ci;
            ci = threadList.erase(ci);
            delete info;
            hasReadyToRun = true;
        } else {
            DEBUG(dbgThread, "Thread " << (*ci)->thread->getName() << " will be waked up after " << (*ci)->x << " times timer interrupts");
            ci++;
        }
    }
}
