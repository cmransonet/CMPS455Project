
#ifndef INDIRECT_H
#define INDIRECT_H

#define NumIndirect (int) (SectorSize / sizeof(int))

class Indirect
{
	public:
		int dataSect[NumIndirect];
		int sectors;
};

#endif
