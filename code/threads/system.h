// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "bitmap.h"
#include "synch.h"
#include "list.h"

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock
extern int threadChoice;
extern int memChoice;
extern bool pageFlag;
//Begin code changes by Ben Matkin and Stephen Mader and Robert Knott
extern List* PageList; // Swap in and out pages
extern int * invertedPageTable[31]; // 31 = NumPhysPages - 1
extern int iptId;
extern int threadCount;
//End code changes by Ben Matkin and Stephen Mader and Robert Knott
extern BitMap *memMap;				//Bitmap to keep track of memory use


#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
extern List* activeThreads;	// active thread list for process management
extern int threadID;	// unique process id
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
