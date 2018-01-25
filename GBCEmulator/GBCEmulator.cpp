// GBCEmulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CPU.h"

#include "Memory.h"

#include <fstream>
#include <iterator>
#include <vector>

#include "MBC.h"

int main()
{

	CPU *cpu = new CPU();
	Memory *memory = new Memory();
	MBC *mbc;


	/*
		Read in BIOS
	*/
	std::string filename = "bios.gb";
	memory->cartridgeReader->setRomDestination(filename);
	memory->cartridgeReader->readRom(true);
	mbc = new MBC(memory->cartridgeReader->cartridgeType.mbc);
	memory->mbc = mbc;

	cpu->memory = memory;


	// Run bios
	std::int16_t i = 0;
	while (i < 256)
	{
		cpu->runInstruction(cpu->getInstruction());
		cpu->printRegisters();
		i++;
	}



	/*
		Read in rom
	*/
	filename = "pokemon-blue.gb";
	memory->cartridgeReader->setRomDestination(filename);
	memory->cartridgeReader->readRom(false);

	//mbc = new MBC(memory->cartridgeReader->cartridgeType.mbc);

	//memory->mbc = mbc;



	//while (1)
	//{

	//	std::int16_t regAddr = cpu->get_register(CPU::REGISTERS::HL);
	//	//char c = cpu->memory->memoryMap[regAddr];

	//	//cpu->runInstruction(romBuffer[cpu->get_register(CPU::PC)]);
	//}






    return 0;
}

