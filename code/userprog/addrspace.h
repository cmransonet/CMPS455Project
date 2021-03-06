// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "noff.h"


#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    
    // Begin code changes by Chet Ransonet
    void loadPage(int badVAddrReg);
    int getNumPages()
    	{return numPages;};
    bool * swapped;
    // End code changes by Chet Ransonet
    
    //Begin code changes by Ryan Mazerole
    void Swap(int pageNum);
    void DestroySwapFile();

	bool Swapin(int page, int frame);
	
   	bool Swapout(int frame);
   	
   	void savePageTableEntry(TranslationEntry entry, int virtualPage){
   		pageTable[virtualPage] = entry;
   	};
   	
   	TranslationEntry pageTableEntry(int physPage)
   	{
   		for(unsigned int i=0; i < numPages; i++)
   		{
   			if(pageTable[i].physicalPage == physPage)
   				return pageTable[i];
   		}
   		ASSERT(false); // we should not be reaching this assert
   	};
   	
   	int getPageNumber(int frame){
		//printf("Running get page number in addrspace.h \nFrame = %d\n numPages = %d\n ", frame);
   		for(unsigned int i = 0; i < numPages; i++){
   				if(pageTable[i].physicalPage == frame && pageTable[i].valid == true){
   					return i;
   				}
   				else
   					return -1;
   		}
   		return -1;
   	};
   	
   	void setDirty(int vpage, bool set){
   		pageTable[vpage].dirty = set;
   	};
   	
   	void setValidity(int vpage, bool set){
   		pageTable[vpage].valid = set;
   	};
   	//End code changes by Ryan Mazerole

  private:
  	// Begin code changes by Chet Ransonet
    OpenFile * file;
    OpenFile * swapFile;
    NoffHeader noffH;
    // End code changes by Chet Ransonet
  
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
	unsigned int startPage;		//Page number that the program starts at
								//in physical memory
	bool space;		//Boolean to remember if there was enough space or not
	
	//Begin code changes by Ryan Mazerole
    char swapfilename[8];
    //end code changes by Ryan Mazerole
};

#endif // ADDRSPACE_H
