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
    unsigned int i, size, /*pAddr,*/ counter;
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
	
	DEBUG('a', "%i contiguous blocks found for %i pages\n", counter, numPages);

	//If no memory available, terminate
	if(counter < numPages)
	{
		printf("Not enough contiguous memory for new process; terminating!.\n");
		currentThread->killNewChild = true;
		return;
	}

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
	
	//memMap->Print();	// Useful!


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

// Begin code changes by Chet Ransonet

// called in the case of a PageFaultException
// loads code and data into a free physical page if there is one
void AddrSpace::loadPage(int badVAddr)
{	
	int virtualPage = badVAddr / PageSize, physPage;
	unsigned int pageStart, offset, size; //modifiers for copying data into memory
	
	physPage = memMap->Find(); // select a physical page not in use
	if (physPage == -1) //if no page was found, terminate. 
	//TODO: here's where we would use swap files!
	{
		printf("ERROR: out of memory, exiting...\n");
		ASSERT(FALSE);
	}	
	pageTable[virtualPage].physicalPage = physPage;
	//pageTable[virtualPage].readOnly = false;
	
	memMap->Print();
	
	//debugging
	printf("page that faulted: %i\nphysical page selected: %i\n", virtualPage, physPage);
	
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
	}
	
	pageTable[virtualPage].valid = true;
	pageTable[virtualPage].dirty = false;
    
    return;
    //pseudo code for swap files
    /*if (fileSystem->Create("testfile.swap", 200)) //creates a blank testfile.swap
    	//printf("file created\n");
    	
    swapfile = fileSywstem->Open(testfile.swap);
    
    swapfile->WriteAt(stuff, 0);*/
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
	
	// Begin code changes by Chet Ransonet
	/*if(space)
	{
		for(unsigned int i = startPage; i < numPages + startPage; i++)	// We need an offset of startPage + numPages for clearing.
			memMap->Clear(i);

		delete pageTable;

		memMap->Print();
	}*/
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

bool AddrSpace::Swapin(int page, int frame){
	
	ASSERT(page >= 0);
	ASSERT(frame >= 0 && frame < NumPhysPages);

	AddrSpace * space = currentThread->space;

	int characterRead;
	char *position = machine->mainMemory + frame * PageSize;

	characterRead = swapFile->ReadAt(position, PageSize, page * PageSize);

	bool check = (characterRead == PageSize);

	if(check){
		space->setValidity(page, true);
		space->setDirty(page, true);
	}

	return check;

}

bool AddrSpace::Swapout(int frame){
	
	ASSERT(frame >= 0 && frame < NumPhysPages);
	
	AddrSpace * space = currentThread->space;
	
	int page = space->getPageNumber(frame);
	
	if(page == -1){
		ASSERT(false);
		return false;
	}
	
	TranslationEntry entry = space->pageTableEntry(page);
	
	if(entry.dirty){
		int charactersWritten;
		char * written = machine->mainMemory + frame * PageSize;
	
		charactersWritten = swapFile->WriteAt(written, PageSize, page * PageSize);
	
		ASSERT(charactersWritten == PageSize);
	}
	
	entry.valid = false;
	entry.dirty = false;
	entry.physicalPage = -1;
	
	space->savePageTableEntry(entry, entry.virtualPage);
	
	return true;
}
//End code changes by Ryan Mazerole