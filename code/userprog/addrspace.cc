// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
//#include "noff.h" //moved to addrspace.h - Chet

extern int swapChoice;

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
	// Begin code changes by Chet Ransonet
	file = executable;
	
    //NoffHeader noffH;
	// End code changes by Chet Ransonet
    unsigned int i, size, counter;
	space = false;
	
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize); 
    size = numPages * PageSize;
    
	Swap(size + 6000);
	//swapFile = fileSystem->Open(swapfilename);
	swapped = new bool[numPages];

	//Change this to reference the bitmap for free pages
	//instead of total amount of pages
	//This requires a global bitmap instance
	
	counter = 0;
	for(i = 0; i < NumPhysPages && counter < numPages; i++)
	{
		if(!memMap->Test(i))
		{
			if(counter == 0)
				startPage = i;	//startPage is a class data member
								//Should it be public or private? (Currently private)
			counter++;
		}
		else
			counter = 0;
	}
/*	
	DEBUG('a', "%i contiguous blocks found for %i pages\n", counter, numPages);

	//If no memory available, terminate
	if(counter < numPages)
	{
		printf("Not enough contiguous memory for new process; terminating!.\n");
		currentThread->killNewChild = true;
		return;
	}
*/
	//If we get past the if statement, then there was sufficient space
	space = true;

	//This safeguards against the loop terminating due to reaching
	//the end of the bitmap but no contiguous space being available

    DEBUG('a', "Initializing address space, numPages=%d, size=%d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		//pageTable[i].physicalPage = i;	//Replace with pageTable[i].physicalPage = i + startPage;
		pageTable[i].physicalPage = -1; //i + startPage;
		// Begin code changes by Chet Ransonet
		pageTable[i].valid = FALSE;//TRUE;
		// End code changes by Chet Ransonet
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
						// a separate page, we could set its 
						// pages to be read-only

		//Take the global bitmap and set the relevant chunks
		//to indicate that the memory is in use
		//memMap->Mark(i + startPage);
    }
	
	//memMap->Print();


/* Don't load into physical memory yet!    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
//    bzero(machine->mainMemory, size); rm for Solaris
	//Edited version adds startPage * PageSize to the address. Hopefully this is proper.
	//Either way, it appears to zero out only however much memory is needed,
	//so zeroing out all memory doesn't seem to be an issue. - Devin
	
	pAddr = startPage * PageSize;
	
    memset(machine->mainMemory + pAddr, 0, size);

// then, copy in the code and data segments into memory
//Change these too since they assume virtual page = physical page
	  //Fix this by adding startPage times page size as an offset
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr + (startPage * PageSize), noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr + pAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr + (startPage * PageSize), noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr + pAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
*/
}

//Begin code changes by Ben Matkin
void loadThreadIntoIPT(int virtualPageNum)
{

/* //The problem with this function is that it's looking for an empty physical page and when the physical memory fills up, it does not load the thread into the ipt.  We already know which page the currentThread needs to take up, we don't need to look for a new, empty page.
 	//Load page into IPT
	int processID = currentThread->getID();
	//int * threadPointer = (int *) currentThread;
	bool loadedIPT = FALSE;
	//Thread * testThread = (Thread *) threadPointer;
	for (int i = 0; i < NumPhysPages - 1; i++) {
		//printf("Iterator - %d\n", i);
		if ( ipt[i] == NULL) {
			ipt[i] = currentThread;
			loadedIPT = TRUE;
			break;
		}
	}

	if (loadedIPT)
		printf("Process %i request VPN %i.\n", processID, virtualPageNum );
	else 
		ASSERT(false);
*/
}
//End code changes by Ben Matkin


// Begin code changes by Chet Ransonet

// called in the case of a PageFaultException
// loads code and data into a free physical page if there is one



void AddrSpace::loadPage(int badVAddr)
{	
	printf("\nPage Fault: \n");

	stats->numPageFaults++;

	int virtualPage = badVAddr / PageSize, physPage;
	unsigned int pageStart, offset, size; //modifiers for copying data into memory
	bool lock = false;
	
   	//loadThreadIntoIPT(virtualPage);

	printf("Page availability before adding the process: \n");
	memMap->Print();	

	physPage = memMap->Find(); // select a physical page not in use
	//printf("Phys page initial = %d\n", physPage);
	if (physPage == -1) //if no page was found, swap out a page
	{
		if(swapChoice == 1) // FIFO
		{
			//Begin code changes by Ben Matkin and Stephen Mader
			printf("Out of memory, swapping pages using FIFO page replacement\n");
			physPage = (int)pageList->Remove(); // Take one page off front of list
			printf("physPage = %d \n", physPage);
			//End code changes by Ben Matkin and Stephen Mader
			
		}
		else if (swapChoice == 2) // Random
		{
			printf("Out of memory, swapping pages using Random page replacement\n");
			physPage = Random() % 32;
			//printf("physPage = %d \n", physPage);	
		}
		else // default
		{
			printf("Out of memory, virtual memory scheme not selected, exiting...\n");
			ASSERT(false); 
		}
		
		
	
		if(!ipt[physPage]->space->Swapout(physPage))
			return;
			
		printf("Process %i request VPN %i.\n", currentThread->getID(), virtualPage);
		
	}
		if(swapChoice == 1)
		{
			pageList->Append((int *) physPage); // Store virtual page, though mainly just maintaining index cue
			lock = true;
			pageLock[physPage]->P();
		}
	ipt[physPage] = currentThread;
		
	//printf("Assigning frame %i \n", physPage);
	pageTable[virtualPage].physicalPage = physPage;
	
	printf("Page availability after adding the process: \n");
	memMap->Print();
	
	//debugging
	printf("Page that faulted: %i\nPhysical page selected: %i\n", virtualPage, physPage);
	
	if(!swapped[virtualPage])
	{
		bzero(machine->mainMemory + PageSize * physPage, PageSize);
		
		pageStart = 0; //starting place to load in code/data
		offset = 0;	//modifies the location we're reading from

		// code segment
		if (noffH.code.size > 0)
		{
			if (noffH.code.virtualAddr <= virtualPage * PageSize)
				offset = virtualPage * PageSize - noffH.code.virtualAddr;
			else 
				pageStart = noffH.code.virtualAddr - virtualPage * PageSize;		
			
			// set size of data to be copied
			size = PageSize - pageStart;
			if (noffH.code.virtualAddr + noffH.code.size < (virtualPage+1) * PageSize)
				size -= (virtualPage) * PageSize - (noffH.code.virtualAddr + noffH.code.size);
				
			// copy data to memory	
			file->ReadAt(&(machine->mainMemory[PageSize * physPage + pageStart]),
			 	size, 
			 	noffH.code.inFileAddr + offset);
			 		 
			//file->ReadAt(&data + pageStart,size,noffH.code.inFileAddr + offset);
			 
		}
		
		pageStart = 0;
		offset = 0;
		
		// initData segment
		if (noffH.initData.size > 0)
		{
			if (noffH.initData.virtualAddr <= virtualPage * PageSize)
				offset = virtualPage * PageSize - noffH.initData.virtualAddr;
			else
				pageStart = noffH.initData.virtualAddr - virtualPage * PageSize;
			
			//set size of data to be copied
			size = PageSize - pageStart;
			if (noffH.initData.virtualAddr + noffH.initData.size < (virtualPage+1) * PageSize)
				size -= (virtualPage) * PageSize - (noffH.initData.virtualAddr + noffH.initData.size);
				
			//copy data into memory
			file->ReadAt(&(machine->mainMemory[PageSize * physPage+pageStart]) , 
				size, 
				noffH.initData.inFileAddr + offset);
			//file->ReadAt(data, size, noffH.initData.inFileAddr + offset);
		}
		
		char * data = machine->mainMemory + physPage * PageSize;
		swapFile->WriteAt(data, PageSize, virtualPage * PageSize);

		pageTable[virtualPage].valid = true;
		pageTable[virtualPage].dirty = false;
		
		
	}
	else
		Swapin(virtualPage, physPage);
	if(lock)
	{
		pageLock[physPage]->V();
		lock = false;
	}
    return;
}

// End code changes by Chet Ransonet

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

//Because the initialization already zeroes out the memory to be used,
//is it even necessary to clear out any garbage data during deallocation?

AddrSpace::~AddrSpace()
{
	// Only clear the memory if it was set to begin with
	// which in turn only happens after space is set to true

	if(space)
	{
		for(unsigned int i = 0; i < numPages; i++)	
		{
			if(pageTable[i].physicalPage != -1)
				memMap->Clear(pageTable[i].physicalPage);
		}
		delete pageTable;
		
		memMap->Print();
	}
	
	delete file;
	
	
	//Begin code changes by Ryan Mazerole
	DestroySwapFile();
	//End code changes by Ryan Mazerole
	// End code changes by Chet Ransonet
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

//Begin code changes by Ryan Mazerole
void AddrSpace::Swap(int pageNum){

	sprintf(swapfilename, "%d.swap", currentThread->getID());

	fileSystem->Create(swapfilename, pageNum * PageSize);
	swapFile = fileSystem->Open(swapfilename);
	
}

void AddrSpace::DestroySwapFile(){
	
	delete swapFile;
	fileSystem->Remove(swapfilename);
}

bool AddrSpace::Swapin(int page, int frame)
{
		
	ASSERT(page >= 0);
	ASSERT(frame >= 0 && frame < NumPhysPages);

	//AddrSpace * space = currentThread->space;

	int characterRead;
	char *position = machine->mainMemory + frame * PageSize;

	characterRead = swapFile->ReadAt(position, PageSize, page * PageSize);

	bool check = (characterRead == PageSize);

	if(check){
		setValidity(page, true);
		setDirty(page, false);
	}
	
	swapped[page] = true;

	return check;

}

bool AddrSpace::Swapout(int frame)
{
	int virtPage = -1;
	for(unsigned int i=0; i < numPages; i++)
	{
		if(pageTable[i].physicalPage == frame)
		{
			virtPage = i; //printf("virtPage = %i\n", virtPage);
			break;
		}
	}
	if(virtPage == -1)
	{
		printf("\n\nERROR: Page could not be swapped!\n\n\n");
		ASSERT(false);
		//return false;
	}

	if(pageTable[virtPage].dirty)
	{
		char * data = machine->mainMemory + frame * PageSize;
		swapFile->WriteAt(data, PageSize, virtPage * PageSize);
	}
	
	pageTable[virtPage].valid = false;
	pageTable[virtPage].dirty = false;
	pageTable[virtPage].physicalPage = -1;
	
	swapped[virtPage] = false;
	
	//savePageTableEntry(pageTable[frame], pageTable[frame].virtualPage);

	return true;
}
//End code changes by Ryan Mazerole
