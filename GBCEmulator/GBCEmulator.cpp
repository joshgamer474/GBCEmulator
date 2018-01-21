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

	//printf("register[A] = %i\n", cpu->get_register(CPU::A));
	//cpu->set_register(CPU::A, (int8_t)100);
	//printf("register[A] = %i\n", cpu->get_register(CPU::A));

	//cpu->printRegisters();


	//cpu->runInstruction((std::int8_t) 0x47);
	//cpu->printRegisters();


	MBC *mbc = new MBC(1);


	/*
		Read in rom
	*/
	Memory *memory = new Memory();
	std::string filename = "pokemon-blue.gb";
	memory->cartridgeReader->setRomDestination(filename);
	memory->cartridgeReader->readRom();



	//while (1)
	//{

	//	std::int16_t regAddr = cpu->get_register(CPU::REGISTERS::HL);
	//	//char c = cpu->memory->memoryMap[regAddr];

	//	//cpu->runInstruction(romBuffer[cpu->get_register(CPU::PC)]);
	//}






    return 0;
}

