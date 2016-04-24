// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
  	numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    
    // Begin code changes by Chet Ransonet
	
	printf("FileHeader Allocate!\n");
	printf("File size = %i bytes\n", numBytes);
	printf("Number of sectors: %i\n", numSectors);
	
	if (freeMap->NumClear() < numSectors)
    {
    	printf("Error: not enough space!\n");
		return FALSE;		// not enough space
	}
	
	if (numBytes <= MaxFileSize) //small file - only use direct blocks
	{
		printf("Small file size: using direct pointers only\n");
		printf("\nBefore allocation: \n"); freeMap->Print();
		
		//allocate direct pointers
    	for (int i = 0; i < numSectors; i++)
			dataSectors[i] = freeMap->Find();
			
		printf("\nAfter allocation: \n"); freeMap->Print();
		
		return true;
	}
	
	else if (numBytes <= MaxFileSizeMed) //medium file - use direct and single
	{
		printf("Medium file size: using direct and single indirect pointers\n");
		printf("Before allocation: \n"); 
		freeMap->Print();
		
		int done = NumDirect - 1;
		int singleIndirect[NumIndirect];
		
		//allocate direct pointers
		for (int i = 0; i < NumDirect; i++)
			dataSectors[i] = freeMap->Find();
		
		//allocate single indirect pointer
		for (int j = 0; j < NumIndirect; j++)
		{
			if(done > numSectors)
				break;
			singleIndirect[j] = freeMap->Find();
			done++;
		}
		
		//write single indirect pointer to disk sector
		synchDisk->WriteSector(dataSectors[NumDirect-1], (char*)singleIndirect);
		
		dataSectors[NumDirect-1] *= -1;
		
		printf("After allocation: \n"); 
		freeMap->Print();		
		return true;
	}
	
	else //big file - use direct, single, and double
	{
		printf("Big file size: Using direct, single indirect, and double indirect pointers\n");
		printf("Before allocation: \n"); 
		freeMap->Print();
		
		int done = NumDirect - 2;
		int indirect[NumIndirect];
		int doubleIndirect[NumIndirect];
		
		//allocate direct pointers
		for (int i = 0; i < NumDirect; i++) 
	        dataSectors[i] = freeMap->Find();

		//allocate single indirect pointer
        for (int j = 0; j < NumIndirect; j++) 
        {
	        indirect[j] = freeMap->Find();
			done++;
		}
		
		//write single indirect pointer to disk sector
        synchDisk->WriteSector(dataSectors[NumDirect-2], (char*)indirect);
        dataSectors[NumDirect-2] *= -1;

		//allocate double indirect pointer
		for (int k = 0; k < NumIndirect; k++) 
		{
			if(done > numSectors)
				break;
				
			doubleIndirect[k] = freeMap->Find();
			done++;
			
			int singleIndirect2[NumIndirect];
       		for (int l = 0; l < NumIndirect; l++) 
       		{
				if(done > numSectors)
					break;
				singleIndirect2[l] = freeMap->Find();
				done++;
			}
			
			//write single indirect pointer to disk sector pointed to by the
			//	double indirect pointer
			synchDisk->WriteSector(doubleIndirect[k], (char*)singleIndirect2);
			doubleIndirect[k] *= -1;
		}

		//write double indirect pointer to disk sector
		synchDisk->WriteSector(dataSectors[NumDirect-1], (char*)doubleIndirect);
		
        dataSectors[NumDirect-1] *= -1;
		
		printf("After allocation: \n"); 
		freeMap->Print();	
		return true;
	}
	// End code changes by Chet Ransonet
	
	return false;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    //Begin code changes by Chet Ransonet	
	int done = 0;

	printf("FileHeader Deallocate!\nBefore Deallocation: \n");
    freeMap->Print();
    
	if(numBytes <= MaxFileSize) // small file - direct only
	{
		//clear direct pointers
		for(int i = 0; i <= numSectors; i++) 
		{
			if(done >= numSectors)
				break;
			done++;
    		freeMap->Clear(dataSectors[i]);
		}
	}
	else if(numBytes <= MaxFileSizeMed) // medium file
	{
		int indirect[NumIndirect];
		
		//clear direct pointers
		for(int i = 0; i < NumDirect-1; i++) 
			freeMap->Clear(dataSectors[i]);

		done = NumDirect-2;

		//get the single indirect pointer array
		synchDisk->ReadSector(dataSectors[NumDirect-1] * -1, (char*)indirect);

		//clear single indirect pointer
		for(int i = 0; i < NumIndirect; i++) 
		{
			if(done >= numSectors)
				break;
			done++;
			freeMap->Clear(indirect[i]);
		}
		freeMap->Clear(dataSectors[NumDirect-1] * -1);
	}
	else // big file
	{
		int indirect[NumIndirect];
		
		//clear direct pointers
		for(int i = 0; i < NumDirect-2; i++) 
		{
			done++;
			freeMap->Clear(dataSectors[i]);
		}
		
		//get the single indirect pointer array
		synchDisk->ReadSector(dataSectors[NumDirect-2] * -1, (char*)indirect);

		//clear single indirect pointer
		for(int i = 0; i < NumIndirect; i++) 
		{
			done++;
			freeMap->Clear(indirect[i]);
		}
		freeMap->Clear(dataSectors[NumDirect-2] * -1);
		
		//get the double indirect pointer array
		synchDisk->ReadSector(dataSectors[NumDirect-1] * -1, (char*)indirect);
		
		//clear double indirect pointer
		for(int i = 0; i < NumIndirect; i++) 
		{
			if(indirect[i] >= 0) 
			{
				if(done >= numSectors)
					break;
				done++;
				freeMap->Clear(indirect[i]);
			} 
			else 
			{
				int indirect2[NumIndirect];
				
				//get the single indirect pointer array
				synchDisk->ReadSector(indirect[i] * -1, (char*)indirect2);
				
				//clear a single indirect pointer pointed to by double indirect pointer
				for(int j = 0; j < NumIndirect; j++) 
				{
					if(done >= numSectors)
						break;
					done++;
					freeMap->Clear(indirect2[j]);
				}
				freeMap->Clear(indirect[i] * -1);
			}
			freeMap->Clear(dataSectors[NumDirect-1] * -1);
		}
	}
	
	printf("After deallocation: \n");
    freeMap->Print();
    return;
	
	//End code changes by Chet Ransonet
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
	//Begin code changes by Chet Ransonet	
	int sectorNum = offset / SectorSize;

	if(sectorNum < NumDirect-1 && dataSectors[sectorNum] >= 0)  //direct
	 	return(dataSectors[sectorNum]);
	 	
	int indirect[NumIndirect];
	
 	if(sectorNum < NumDirect - 1)  //inside dataSectors
 	{
		synchDisk->ReadSector(dataSectors[sectorNum] * -1, (char*)indirect);
		
		if(indirect[0] >= 0)  //single block location
		{
			return indirect[0];
		}
		else //double block location
		{
			int doubleIndirect[NumIndirect];
			synchDisk->ReadSector(indirect[sectorNum-NumDirect-2] * -1, (char*)doubleIndirect);
			return doubleIndirect[(sectorNum-NumDirect-2-NumDirect) % NumIndirect];
		}
	}
	else //outside dataSectors
	{
		if(sectorNum < (NumDirect - 2 + NumIndirect)) //in single block
		{
			if(dataSectors[NumDirect-2] >= 0)
			{	
				synchDisk->ReadSector(dataSectors[NumDirect-1] * -1, (char*)indirect);
				return indirect[sectorNum-NumDirect+1];
			}
			else
			{
				synchDisk->ReadSector(dataSectors[NumDirect-2] * -1, (char*)indirect);
				return indirect[sectorNum-NumDirect+2];
			}
		}
		else //in double block
		{
			int doubleIndirect[NumIndirect];
			
			synchDisk->ReadSector(dataSectors[NumDirect-1] * -1, (char*)indirect);
			synchDisk->ReadSector(indirect[(sectorNum-(NumDirect-2+NumIndirect))/NumIndirect] * -1, (char*) doubleIndirect);
			return doubleIndirect[(sectorNum-(NumDirect-2+NumIndirect))%NumIndirect];
		}
		
	}
	
	ASSERT(false);
	//End code changes by Chet Ransonet
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
		printf("%d ", dataSectors[i]);
		
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) 
    {
		synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) 
        {
	    	if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
				printf("%c", data[j]);
            else
				printf("\\%x", (unsigned char)data[j]);
		}
        printf("\n"); 
    }
    delete [] data;
}

//End code changes by Chet Ransonet
