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
	GPU *gpu = new GPU();



	/*
	Read in rom
	*/
	std::string filename = "pokemon-blue.gb";
	memory->cartridgeReader->setRomDestination(filename);
	memory->cartridgeReader->readRom();

	mbc = new MBC(memory->cartridgeReader->cartridgeType.mbc);
	memory->mbc = mbc;
	memory->gpu = gpu;
	memory->initROMBanks();

	cpu->memory = memory;


	// Run bios
	std::int16_t i = 0;
	std::uint16_t pc = 0;
	while (cpu->get_register_16(CPU::REGISTERS::PC) <= 256)
	{
		pc = cpu->get_register_16(CPU::REGISTERS::PC);
		printf("PC = %#04x\n", pc);
		cpu->runInstruction(cpu->getInstruction());
		cpu->printRegisters();
		i++;
	}



	/*
		Read in rom
	*/
	//filename = "pokemon-blue.gb";
	//memory->cartridgeReader->setRomDestination(filename);
	//memory->cartridgeReader->readRom(false);

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

