#include "stdafx.h"

#include "CPU.h"


CPU_Opcodes::CPU_Opcodes()
{
	opcodes.resize(NUM_OF_OPCODES);
	reg_list = { CPU::B, CPU::C, CPU::D, CPU::E, CPU::H, CPU::L, CPU::HL, CPU::A };

	init();
}

CPU_Opcodes::~CPU_Opcodes()
{

}

// http://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
void CPU_Opcodes::init()
{
	/*
		Init LDs 0x40 - 0x7F
	*/
	op currOp;
	int reg1Counter, reg2Counter;
	reg1Counter = reg2Counter = CPU::B;
	bool skip = false;
	for (int i = 0x40; i < 0x80; i++)
	{

		// Set opcode
		currOp.operand = LD;
		currOp.flags = 0;

		if (i == 0x76)
		{
			continue;
		}

		reg1Counter = reg_list[(i / 0x08) - 0x08];
		reg2Counter = reg_list[(i & 0x0F) % 0x08];

		currOp.reg1 = (CPU::REGISTERS) reg1Counter;
		currOp.reg2 = (CPU::REGISTERS) reg2Counter;

		currOp.durationCompleted = 4;

		opcodes.at(i) = currOp;
	}



	/*
		Init ADDs 0x80 - 0x8F
	*/



	/*
		Init SUBs 0x90 - 0x9F
	*/


	/*
		Init ANDs 0xA0 - 0xAF
	*/


	/*
		Init ORs 0xB0 - 0xBF
	*/



}
