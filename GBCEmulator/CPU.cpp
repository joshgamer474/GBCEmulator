#include "stdafx.h"
#include "CPU.h"
#include "Memory.h"
#include "CartridgeReader.h"
#include "Debug.h"

CPU::CPU()
{
	memory = new Memory();

	registers.resize(NUM_OF_REGISTERS);
	ticks = 0;
	interrupts_enabled = false;
	is_halted = false;
	is_stopped = false;
}

CPU::~CPU()
{
	delete memory;
}


/*
	Register getter

	16-bit Registers

	16bit Hi   Lo   Name/Function
	AF    A    -    Accumulator & Flags
	BC    B    C    BC
	DE    D    E    DE
	HL    H    L    HL
	SP    -    -    Stack Pointer
	PC    -    -    Program Counter/Pointer
*/


std::uint8_t CPU::get_register_8(REGISTERS reg)
{
	if (reg < B)
	{
		logger->error("Error - Using get_register_8() to get register: %s", REGISTERS_STR[reg]);
		return 0;
	}
	else
	{
		if (reg % 2 == 0)	// Return upper 8
		{
			return static_cast<std::uint8_t> ((get_register_16((CPU::REGISTERS) ((reg - B) / 2)) & 0xFF00) >> 8);
		}
		else				// Return lower 8
		{
			return static_cast<std::uint8_t> (get_register_16((CPU::REGISTERS) ((reg - B) / 2)) & 0x00FF);
		}
	}
}


std::uint16_t CPU::get_register_16(REGISTERS reg) 
{ 
	if (reg == CPU::REGISTERS::SP || reg == CPU::REGISTERS::PC)	// Return full 16 bits
	{
		return registers[reg];
	}
	if (reg < B)			// Return little endian converted back to big endian
	{
		std::uint16_t littleEndian = registers[reg];
		std::uint8_t lowerByte = static_cast<std::uint8_t> ((littleEndian >> 8) & 0xFF);
		std::uint8_t upperByte = static_cast<std::uint8_t> (littleEndian & 0x00FF);
		std::uint16_t bigEndian = 0x0000;
		bigEndian |= (upperByte << 8);
		bigEndian |= lowerByte;
		return bigEndian;
	}
	else
	{
		if (reg % 2 != 0)	// Return upper 8
		{
			return (registers[(reg - B) / 2] & 0xFF00) >> 8;
		}
		else				// Return lower 8
		{
			return (registers[(reg - B) / 2] & 0x00FF);
		}
	}
}


/*
	Register setters
*/
void CPU::set_register(REGISTERS reg, std::uint16_t val)
{
	if (reg == CPU::REGISTERS::SP || reg == CPU::REGISTERS::PC)	// Set full 16 bits
	{
		registers[reg] = val;
	}
	else if (reg < B)	// Set full 16 bits in little endian
	{
		std::uint8_t upperByte = static_cast<std::uint8_t> ((val >> 8) & 0xFF);
		std::uint8_t lowerByte = static_cast<std::uint8_t> (val & 0x00FF);
		std::uint16_t littleEndian = 0x0000;
		littleEndian |= (lowerByte << 8);
		littleEndian |= upperByte;
		registers[reg] = littleEndian;
	}
	else			// Set only 8 bits
	{
		set_register(reg, (std::uint8_t) val);
	}
}

void CPU::set_register(REGISTERS reg, std::uint8_t val)
{
	if (reg >= B)
	{
		std::uint16_t *reg_ptr = &registers[(reg - B) / 2];

		if (reg % 2 != 0)	// Is upper 8 bits
		{
			*reg_ptr &= 0x00FF;
			*reg_ptr |= (((std::uint16_t) val) & 0x00FF) << 8;
		}
		else				// Is lower 8 bits
		{
			*reg_ptr &= 0xFF00;
			*reg_ptr |= ((std::uint16_t) val) & 0x00FF;
		}
	}
	else
	{
		logger->error("Error in set_register(uint8_t) - reg < B, reg: %s", REGISTERS_STR[reg]);
	}
}

void CPU::set_register(REGISTERS reg, std::int8_t val)
{
	if (reg >= B)
	{
		std::uint16_t *reg_ptr = &registers[(reg - B) / 2];

		if (reg % 2 != 0)	// Is upper 8 bits
		{
			*reg_ptr &= 0x00FF;
			*reg_ptr |= (((std::int16_t) val) & 0x00FF) << 8;
		}
		else				// Is lower 8 bits
		{
			*reg_ptr &= 0xFF00;
			*reg_ptr |= ((std::int16_t) val) & 0x00FF;
		}
	}
	else
	{
		logger->error("Error in set_register(int8_t) - reg < B, reg: %s", REGISTERS_STR[reg]);
	}
}




/*
	Flag methods

	8-bit Register

	Bit  Name  Set Clr  Expl.
	7    zf    Z   NZ   Zero Flag
	6    n     -   -    Add/Sub-Flag (BCD)
	5    h     -   -    Half Carry Flag (BCD)
	4    cy    C   NC   Carry Flag
	3-0  -     -   -    Not used (always zero)



	Flag register (F) bits:

	7 	6 	5 	4 	3 	2 	1 	0
	Z 	N 	H 	C 	0 	0 	0 	0

	Z - Zero Flag
	N - Subtract Flag
	H - Half Carry Flag
	C - Carry Flag
	0 - Not used, always zero
*/

bool CPU::get_flag_zero()			{ return (get_register_8(F) >> 7) & 0x0001; }
bool CPU::get_flag_subtract()		{ return (get_register_8(F) >> 6) & 0x0001; }
bool CPU::get_flag_half_carry()		{ return (get_register_8(F) >> 5) & 0x0001; }
bool CPU::get_flag_carry()			{ return (get_register_8(F) >> 4) & 0x0001; }

void CPU::set_flag_zero()			{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) | 0x0080)); }
void CPU::set_flag_subtract()		{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) | 0x0040)); }
void CPU::set_flag_half_carry()		{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) | 0x0020)); }
void CPU::set_flag_carry()			{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) | 0x0010)); }

void CPU::clear_flag_zero()			{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) & 0xFF7F)); }
void CPU::clear_flag_subtract()		{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) & 0xFFBF)); }
void CPU::clear_flag_half_carry()	{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) & 0xFFDF)); }
void CPU::clear_flag_carry()		{ set_register((REGISTERS)AF, static_cast<std::uint16_t>(get_register_16((REGISTERS)AF) & 0xFFEF)); }




/*
	Debug Methods
*/
void CPU::printRegisters()
{
	//std::string s;

	//printf("------------------------------\n");
	//printf("\tRegisters\n");
	//printf("------------------------------\n");
	//for (int i = 0; i < NUM_OF_REGISTERS; i++)
	//{
	//	printf("%s: %#04x\n", getRegisterString((CPU::REGISTERS) i).c_str(), get_register_16((CPU::REGISTERS) i));
	//}
	//printf("\n");

	logger->info("Registers - BC: 0x{0:x}\tDE: 0x{1:x}\tHL: 0x{2:x}\tAF: 0x{3:x}\tSP: 0x{4:x}\tPC: 0x{5:x}",
		get_register_16(BC),
		get_register_16(DE),
		get_register_16(HL),
		get_register_16(AF),
		get_register_16(SP),
		get_register_16(PC));
}


std::string CPU::getRegisterString(CPU::REGISTERS reg)
{
	switch (reg)
	{
	case CPU::REGISTERS::BC: return "BC";
	case CPU::REGISTERS::DE: return "DE";
	case CPU::REGISTERS::HL: return "HL";
	case CPU::REGISTERS::AF: return "AF";
	case CPU::REGISTERS::SP: return "SP";
	case CPU::REGISTERS::PC: return "PC";

	default: return "Unknown";
	}
}



/*
	Instruction Methods
*/

// Get instruction from Ram[PC]
std::uint8_t CPU::getInstruction()
{
	// Check for interrupts
	if (interrupts_enabled & memory->interrupt_flag)
	{
		interrupts_enabled = false;

		std::uint8_t mask = 0x01;

		// Find out which interrupt should be processed
		for (int i = 0; i < 5; i++)
		{
			if ((memory->interrupt_flag & mask) && (memory->interrupt_enable & mask))
			{
				mask = ~mask;
				memory->interrupt_flag &= mask;	// clear bit in interrupt_flag
				PUSH(PC);
				registers[PC] = interrupt_table[i];
				break;
			}
			mask = mask << 1;
		}
	}

	return getByteFromMemory(get_register_16(PC));
}

bool CPU::runInstruction(std::uint8_t instruc)
{
	std::uint8_t a8, d8, parenA8, flagType;
	std::int8_t r8;
	std::uint16_t a16, d16, addr, hlVal;

	int regPattern1, regPattern2;
	regPattern1 = (instruc / 0x08) - 0x08;	// B, B, B, B, B, B, B, B, C, C, C, C, C, C, C, C, D, D, etc.
	regPattern2 = (instruc & 0x0F) % 0x08;	// B, C, D, E, H, L, HL, A, B, C, D, etc.

	// Check if bios is done running
	if (registers[PC] >= 0x100 && memory->cartridgeReader->is_bios)
	{
		memory->cartridgeReader->is_bios = false;
		printRegisters();
		logger->info("Done running bootstrap, moving on to cartridge");
		startLogging = true;
	}


	//// Check for interrupts
	//if (interrupts_enabled & memory->interrupt_flag)
	//{
	//	interrupts_enabled = false;

	//	std::uint8_t mask = 0x01;

	//	// Find out which interrupt should be processed
	//	for (int i = 0; i < 5; i++)
	//	{
	//		if ((memory->interrupt_flag & mask) && (memory->interrupt_enable & mask))
	//		{
	//			mask = ~mask;
	//			memory->interrupt_flag &= mask;	// clear bit in interrupt_flag
	//			PUSH(PC);
	//			registers[PC] = interrupt_table[i];
	//			break;
	//		}
	//		mask = mask << 1;
	//	}
	//}

	if (startLogging)
	{
		//printRegisters();
		//logger->info("PC: 0x{0:x}, instruction: 0x{1:x}", registers[PC], instruc);
		int a = 0;
	}

	registers[PC]++;

	switch (instruc)
	{

		/*
			LD
		*/

		// LD X, Y
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45:			 case 0x47:
	case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D:			 case 0x4F:
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:			 case 0x57:
	case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D:			 case 0x5F:
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65:			 case 0x67:
	case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D:			 case 0x6F:
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D:			 case 0x7F:

		LD((CPU::REGISTERS) reg_list[regPattern1], (CPU::REGISTERS) reg_list[regPattern2]);
		break;


		// LD A, (Y)
	case 0x0A: case 0x1A:

		LD(A, getByteFromMemory((CPU::REGISTERS) ((instruc & 0xF0) >> 4)), false);	// (BC), (DE)
		break;

		// LD A, (HL+-)
	case 0x2A: case 0x3A:

		LD(A, getByteFromMemory(HL), false);

		if (instruc == 0x2A)
			set_register(HL, static_cast<std::uint16_t> (get_register_16(HL) + 1));	// HL+
		else
			set_register(HL, static_cast<std::uint16_t> (get_register_16(HL) - 1));	// HL-
		break;

		// LD X, (HL)
	case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x7E:

		LD((CPU::REGISTERS) reg_list[regPattern1], getByteFromMemory(HL), false);
		break;


		// LD (HL), Y
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:			case 0x77:

		LD_reg_into_memory(HL, (CPU::REGISTERS) reg_list[regPattern2]);
		break;

		// LD [(BC), (DE)], A
	case 0x02: case 0x12:

		LD_reg_into_memory((CPU::REGISTERS) ((instruc >> 4) & 0x0F), A);
		break;

		//LD (HL+-), Y
	case 0x22: case 0x32:

		LD_reg_into_memory(HL, A);

		if (instruc == 0x22)
			set_register(HL, static_cast<std::uint16_t> (get_register_16(HL) + 1));	// HL+
		else
			set_register(HL, static_cast<std::uint16_t> (get_register_16(HL) - 1));	// HL-
		break;


		// LD X, d8
	case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E:		case 0x3E:

		if ((instruc & 0x0F) == 0x06)
			LD((CPU::REGISTERS) reg_list[(instruc >> 4) * 2], getByteFromMemory(PC), false);	// B, D, H
		else
			LD((CPU::REGISTERS) reg_list[((instruc >> 4) * 2) + 1], getByteFromMemory(PC), false);	// C, E, L, A

		registers[PC]++;
		break;

		// LD (HL), d8
	case 0x36:

		LD(HL, getByteFromMemory(PC), true);
		registers[PC]++;
		break;

		// LD (a16), A
	case 0xEA:

		a16 = getNextTwoBytes();
		LD(a16, get_register_8(A));
		break;

		// LD A, (a16)
	case 0xFA:

		a16 = getNextTwoBytes();
		LD_INDIRECT_A16(A, a16);
		break;

		// LD XY, d16
	case 0x01: case 0x11: case 0x21: case 0x31:

		d16 = getNextTwoBytes();

		if (instruc == 0x31)
			LD((CPU::REGISTERS) SP, d16);
		else
			LD((CPU::REGISTERS) ((instruc & 0xF0) >> 4), d16);	// BC, DE, HL

		break;

		// LD SP, HL
	case 0xF9:

		LD(SP, HL);
		break;

		// LD (a16), SP
	case 0x08:

		a16 = getNextTwoBytes();
		LD(a16, get_register_16(SP));
		break;

		// LD (C), A
	case 0xE2:

		LD_reg_into_memory(C, A);
		break;

		// LD A, (C)
	case 0xF2:

		//LD(A, getByteFromMemory(get_register_16(C)), false);
		LD(A, getByteFromMemory(0xFF00 + get_register_16(C)), false);
		break;

		// LDH A, (a8)  = LD A, (0xFF00 + a8)
	case 0xF0:

		a8 = getByteFromMemory(PC);
		registers[PC]++;
		parenA8 = getByteFromMemory(static_cast<std::uint16_t> (0xFF00 + a8));
		LDH(A, parenA8);
		break;

		// LDH (a8), A = LD (0xFF00 + a8), A
	case 0xE0:

		a8 = getByteFromMemory(PC);
		registers[PC]++;
		LDH_INDIRECT(static_cast<std::uint16_t> (0xFF00 + a8), get_register_8(A));
		break;

		// LD HL, SP+r8
	case 0xF8:

		r8 = static_cast<std::int8_t>(getByteFromMemory(PC));
		registers[PC]++;
		LD_HL_SPPLUSR8(HL, r8);
		break;


		/*
			ADD
		*/


		// ADD A, Y
	case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:		case 0x87:

		ADD(CPU::REGISTERS::A, get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// ADD A, (HL)
	case 0x86:

		ADD(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// ADD A, d8
	case 0xC6:

		ADD(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;

		// ADD HL, XY
	case 0x09: case 0x19: case 0x29: case 0x39:

		if (instruc == 0x39)
			ADD_HL((CPU::REGISTERS) SP);
		else
			ADD_HL((CPU::REGISTERS) ((instruc & 0xF0) >> 4));	// BC, DE, HL
		break;

		// ADD SP, r8
	case 0xE8:

		r8 = static_cast<std::int8_t>(getByteFromMemory(get_register_16(PC)));
		registers[PC]++;
		ADD_SP_R8(SP, r8);
		break;


		/*
			ADC
		*/

		// ADC A, Y
	case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D:		 case 0x8F:

		ADC(CPU::REGISTERS::A, get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// ADC A, (HL)
	case 0x8E:

		ADC(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// ADC A, d8
	case 0xCE:

		ADC(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;



		/*
			SUB
		*/

		// SUB X
	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:		 case 0x97:

		SUB(get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// SUB (HL)
	case 0x96:

		SUB(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// SUB d8
	case 0xD6:

		SUB(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;



		/*
			SBC
		*/

		// SBC X
	case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D:		 case 0x9F:

		SBC(get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// SBC A, (HL)
	case 0x9E:

		SBC(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// SBC d8
	case 0xDE:

		SBC(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;




		/*
			AND
		*/

		// AND X
	case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5:		 case 0xA7:

		AND(get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// AND (HL)
	case 0xA6:

		AND(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// AND d8
	case 0xE6:

		AND(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;




		/*
			XOR
		*/

		// XOR X
	case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD:		 case 0xAF:

		XOR(get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// XOR (HL)
	case 0xAE:

		XOR(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// XOR d8
	case 0xEE:

		XOR(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;



		/*
			OR
		*/

		// OR X
	case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5:		 case 0xB7:

		OR(get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// OR (HL)
	case 0xB6:

		OR(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// OR d8
	case 0xF6:

		OR(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;


		/*
			CP
		*/

		// CP X
	case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD:		 case 0xBF:

		CP(get_register_8((CPU::REGISTERS) reg_list[regPattern2]), false);
		break;

		// CP X
	case 0xBE:

		CP(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// CP d8
	case 0xFE:

		CP(memory->readByte(get_register_16((CPU::REGISTERS) PC)), false);
		registers[PC]++;
		break;



		/*
			IND
		*/

		// INC XY
	case 0x03: case 0x13: case 0x23: case 0x33:

		if (instruc == 0x33)
			INC((CPU::REGISTERS) SP, false);
		else
			INC((CPU::REGISTERS) ((instruc & 0xF0) >> 4), false);	// INC BC, DE, HL
		break;

		// INC X
	case 0x04: case 0x14: case 0x24: case 0x34:
	case 0x0C: case 0x1C: case 0x2C: case 0x3C:

		if (instruc == 0x34)
			INC((CPU::REGISTERS) HL, true);						// INC (HL)
		else
		{
			int evenReg = 0x06 + (2 * ((instruc & 0xF0) >> 4));	// B, D, H

			if ((instruc & 0x0F) == 0x04)
				INC((CPU::REGISTERS) evenReg, false);			// INC B, D, H
			else if (instruc == 0x3C)
				INC((CPU::REGISTERS) A, false);					// INC A
			else
				INC((CPU::REGISTERS) (evenReg + 1), false);		// INC C, E, L
		}
		break;



		/*
			DEC
		*/

		// DEC XY
	case 0x0B: case 0x1B: case 0x2B: case 0x3B:

		if (instruc == 0x3B)
			DEC((CPU::REGISTERS) SP, false);
		else
			DEC((CPU::REGISTERS) ((instruc & 0xF0) >> 4), false);	// DEC BC, DE, HL
		break;

		// DEC X
	case 0x05: case 0x15: case 0x25: case 0x35:
	case 0x0D: case 0x1D: case 0x2D: case 0x3D:

		if (instruc == 0x35)
			DEC((CPU::REGISTERS) HL, true);						// DEC (HL)
		else
		{
			int evenReg = 0x06 + (2 * ((instruc & 0xF0) >> 4));

			if ((instruc & 0x0F) == 0x05)
				DEC((CPU::REGISTERS) evenReg, false);			// DEC B, D, H
			else if (instruc == 0x3D)
				DEC((CPU::REGISTERS) A, false);					// DEC A
			else
				DEC((CPU::REGISTERS) (evenReg + 1), false);		// DEC C, E, L
		}
		break;


		/*
			Jumps
		*/

		// JP a16
	case 0xC2: case 0xC3: case 0xCA: case 0xD2: case 0xDA:

		addr = getNextTwoBytes();
		if (instruc == 0xC3)
			JP(CPU::FLAGTYPES::NONE, addr);						// JP a16
		else
		{
			std::int8_t flagType = ((instruc & 0xF0) >> 4) - 0x0C;	// 0, 1
			if ((instruc & 0x0F) == 0x0A)
				JP((CPU::FLAGTYPES) (flagType + 2), addr);		// JP [Z, C], a16
			else
				JP((CPU::FLAGTYPES) flagType, addr);			// JP [NZ, NC], a16

		}
		break;

		// JP (HL)
	case 0xE9:

		JP_INDIRECT(get_register_16(HL));
		break;


		/*
			JR
		*/
		// JR r8		JR [NZ, NC, Z, C], r8
	case 0x18: case 0x20: case 0x28: case 0x30: case 0x38:

		r8 = static_cast<std::int8_t>(getByteFromMemory(CPU::REGISTERS::PC));
		registers[PC]++;
		if (instruc == 0x18)
			JR(CPU::FLAGTYPES::NONE, r8);						// JR r8
		else
		{
			std::int8_t flagType = ((instruc & 0xF0) >> 4) - 0x02;	// 0, 1
			if ((instruc & 0x0F) == 0x08)
				JR((CPU::FLAGTYPES) (flagType + 2), r8);		// JR [Z, C], r8
			else
				JR((CPU::FLAGTYPES) flagType, r8);				// JR [NZ, NC], r8

		}
		break;



		/*
			Returns
		*/

		// RET		RET [NZ, NC, Z, C]
	case 0xC0: case 0xC8: case 0xC9: case 0xD0: case 0xD8:

		if (instruc == 0xC9)
			RET(CPU::FLAGTYPES::NONE);								// RET
		else
		{
			std::int8_t flagType = ((instruc & 0xF0) >> 4) - 0x0C;	// 0, 1
			if ((instruc & 0x0F) == 0x08)
				RET((CPU::FLAGTYPES) (flagType + 2));				// RET [Z, C]
			else
				RET((CPU::FLAGTYPES) flagType);						// RET [NZ, NC]
		}

		break;


		// RETI
	case 0xD9:

		RETI();
		break;




		/*
			Interrupts
		*/
	case 0xF3:
		
		disable_interrupts();
		break;

	case 0xFB:

		enable_interrupts();
		break;



		/*
			Restarts
		*/

		// RST
	case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF:

		RST(instruc);
		break;


		/*
			Calls
		*/

		// CALL a16		CALL [NZ, NC, Z, C], a16
	case 0xC4: case 0xCC: case 0xCD: case 0xD4: case 0xDC:

		a16 = getNextTwoBytes();
		if (instruc == 0xCD)
			CALL(CPU::FLAGTYPES::NONE, a16);						// CALL a16
		else
		{
			std::int8_t flagType = ((instruc & 0xF0) >> 4) - 0x0C;	// 0, 1
			if ((instruc & 0x0F) == 0x0C)
				CALL((CPU::FLAGTYPES) (flagType + 2), a16);			// CALL [Z, C], a16
			else
				CALL((CPU::FLAGTYPES) flagType, a16);				// CALL [NZ, NC], a16
		}
		break;


		/*
			DAA
		*/
		
		// DAA
	case 0x27:

		DAA();
		break;



		/*
			SCF (Set carry flag), CCF (Complement carry flag), and CPL (Complement A)
		*/
	case 0x37:

		SCF();
		break;

	case 0x2F:

		CPL();
		break;

	case 0x3F:

		CCF();
		break;




		/*
			HALT, NOP, STOP
		*/
	case 0x00:

		NOP();
		break;

	case 0x10:

		STOP();
		break;

	case 0x76:

		HALT();
		break;



		/*
			RLCA, RLA, RRCA, and RRA
		*/
	case 0x07:

		RLCA();
		break;

	case 0x0F:

		RRCA();
		break;

	case 0x17:

		RLA();
		break;

	case 0x1F:

		RRA();
		break;


		/*
			PUSH and POP
		*/

		// PUSH [BC, DE, HL, AF]
	case 0xC5: case 0xD5: case 0xE5: case 0xF5:

		if (instruc == 0xF5)
			PUSH(AF);
		else
			PUSH((CPU::REGISTERS) (((instruc & 0xF0) >> 4) - 0x0C));	// PUSH [BC, DE, HL]
		break;


		// POP [BC, DE, HL, AF]
	case 0xC1: case 0xD1: case 0xE1: case 0xF1:

		if (instruc == 0xF1)
			POP(AF);
		else
			POP((CPU::REGISTERS) (((instruc & 0xF0) >> 4) - 0x0C));	// POP [BC, DE, HL]
		break;



		/*
			Prefix CB
		*/
	case 0xCB:

		handle_CB(getByteFromMemory(get_register_16(PC)));
		break;



	default:
		logger->error("Error - Do not know how to handle opcode 0x{0:x}", instruc);

	}// end switch()

	

	return false;
}










/*
	Memory methods
*/

// Perform (reg)
uint8_t CPU::getByteFromMemory(CPU::REGISTERS reg)
{
	return memory->readByte(get_register_16(reg));
}

// Perform (addr)
uint8_t CPU::getByteFromMemory(std::uint16_t addr)
{
	return memory->readByte(addr);
}

void CPU::setByteToMemory(uint16_t addr, uint8_t val)
{
	memory->setByte(addr, val);
}



uint16_t CPU::getNextTwoBytes()
{
	std::uint16_t d16 = 0x0000;
	d16 |= getByteFromMemory(PC);
	registers[PC]++;
	d16 |= ((static_cast<std::uint16_t>(getByteFromMemory(PC)) << 8) & 0xFF00);
	registers[PC]++;
	return d16;
}




/*
	LD methods
*/

// LD X, Y
void CPU::LD(CPU::REGISTERS reg1, CPU::REGISTERS reg2)
{
	// Load reg2 into reg1
	if (reg1 == CPU::REGISTERS::SP)
		set_register(reg1, get_register_16(reg2));	// Support LD SP, HL
	else
		set_register(reg1, get_register_8(reg2));

	// Add to ticks
	if (reg1 == CPU::REGISTERS::HL || reg2 == CPU::REGISTERS::HL)
		ticks += 8;
	else
		ticks += 4;
}

// LD (C), A
void CPU::LD_reg_into_memory(CPU::REGISTERS reg1, CPU::REGISTERS reg2)
{
	// Get val from reg2
	uint8_t val = get_register_8(reg2);

	// Get address from reg1
	uint16_t addr = get_register_16(reg1);

	// Cover LD (C), A
	if (reg1 == C)
	{
		addr += 0xFF00;
	}

	setByteToMemory(addr, val);

	// Add to ticks
	ticks += 8;
}

// LD X, d8 and LD (X), d8 when indirect == true
void CPU::LD(CPU::REGISTERS reg, uint8_t val, bool indirect=false)
{
	if (!indirect)
		set_register(reg, val);
	else
	{
		// Get address to write to
		uint16_t addr = get_register_16(reg);
		setByteToMemory(addr, val);
	}

	// Add to ticks
	if (!indirect)
		ticks += 8;
	else
		ticks += 12;
}


// LD XY, d16
void CPU::LD(CPU::REGISTERS reg, std::uint16_t val)
{
	set_register(reg, val);

	// Add to ticks
	ticks += 12;
}

// LD (a16), val
void CPU::LD(std::uint16_t addr, std::uint8_t val)
{
	setByteToMemory(addr, val);

	// Add to ticks
	ticks += 16;
}

// LD (a16), SP
void CPU::LD(std::uint16_t addr, std::uint16_t val)
{
	std::uint8_t upperByte = static_cast<std::int8_t> ((val >> 8) & 0xFF);
	std::uint8_t lowerByte = static_cast<std::int8_t> (val & 0xFF);

	setByteToMemory(addr, lowerByte);
	setByteToMemory(addr + 1, upperByte);

	// Add to ticks
	ticks += 20;
}


// LD A, (a16)
void CPU::LD_INDIRECT_A16(CPU::REGISTERS reg, std::uint16_t addr)
{
	// Read in addr->val
	std::uint8_t val = getByteFromMemory(addr);

	set_register(reg, val);

	// Add to ticks
	ticks += 16;
}

// LDH A, (a8)
void CPU::LDH(CPU::REGISTERS reg, std::uint8_t val)
{
	set_register(reg, val);

	// Add to ticks
	ticks += 12;
}

// LDH (a8), A
void CPU::LDH_INDIRECT(std::uint16_t addr, std::uint8_t val)
{
	setByteToMemory(addr, val);

	// Add to ticks
	ticks += 16;
}

// LD HL, SP+r8
void CPU::LD_HL_SPPLUSR8(CPU::REGISTERS reg, std::int8_t r8)
{
	std::int16_t spVal = get_register_16(SP);
	std::uint16_t result = spVal + r8;
	set_register(reg, result);


	// Clear flag zero
	clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if ((spVal & 0x0100) == 0 && (result & 0x0100) > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if ((spVal & 0x8000) == 0 && (result & 0x8000) > 0)
		set_flag_carry();
	else
		clear_flag_carry();

	// Add to ticks
	ticks += 12;
}


/*
	ADD methods
*/

// ADD A, d8
void CPU::ADD(CPU::REGISTERS reg, std::uint8_t d8, bool indirect=false)
{
	// Get reg->value
	std::uint8_t r, result;
	r = get_register_8(reg);

	result = r + d8;

	set_register(reg, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if (((r & 0x0F) + (d8 & 0x0F)) > 0x0F)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (((std::uint16_t)(r) + (std::uint16_t)(d8)) > 0xFF)
		set_flag_carry();
	else
		clear_flag_carry();


	// Add to ticks
	if (!indirect)
		ticks += 8;
	else
		ticks += 4;
}


// ADC A, X
// ADC A, d8
void CPU::ADC(CPU::REGISTERS reg, std::uint8_t val, bool indirect=false)
{
	// Get reg->value
	std::uint8_t r, result, carryFlag;
	r = get_register_8(reg);
	carryFlag = get_flag_carry();

	result = r + val + carryFlag;

	set_register(reg, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if (((r & 0x0F) + (val & 0x0F) + carryFlag) > 0x0F)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (((std::uint16_t)(r) + (std::uint16_t)(val) + (std::uint16_t)(carryFlag)) > 0xFF)
		set_flag_carry();
	else
		clear_flag_carry();


	// Add to ticks
	if (!indirect)
		ticks += 8;
	else
		ticks += 4;
}


// ADD HL, XY
void CPU::ADD_HL(CPU::REGISTERS reg)
{
	std::uint16_t hlVal, regVal, result;
	hlVal = get_register_16(HL);
	regVal = get_register_16(reg);

	result = hlVal + regVal;

	set_register(reg, static_cast<std::uint16_t> (result & 0xFFFF));

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	//if ((hlVal & 0x0100) == 0 && (result & 0x0100) > 0)
	if ((hlVal & 0x0800) == 1 && (result & 0x0800) == 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	//if ((hlVal & 0x8000) == 0 && (result & 0x8000) > 0)
	if ((hlVal & 0x8000) == 1 && (result & 0x8000) == 0)
		set_flag_carry();
	else
		clear_flag_carry();


	// Add to ticks
	ticks += 8;
}


// ADD SP, r8
void CPU::ADD_SP_R8(CPU::REGISTERS reg, std::int8_t r8)
{
	std::int16_t spVal = get_register_16(reg);
	std::uint16_t result = static_cast<std::uint16_t>(spVal + r8);

	registers[SP] = result;

	// Clear flag zero
	clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if ((spVal & 0x0100) == 0 && (result & 0x0100) > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if ((spVal & 0x8000) == 0 && (result & 0x8000) > 0)
		set_flag_carry();
	else
		clear_flag_carry();

	// Add to ticks
	ticks += 16;
}


/*
	SUB methods
*/

// SUB X
void CPU::SUB(std::uint8_t d8, bool indirect=false)
{
	// Get reg->value and regA->value
	std::uint8_t regAValue, result;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue - d8;

	set_register(CPU::REGISTERS::A, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Set flag negative
	set_flag_subtract();

	// Check flag half carry
	if (((std::int16_t)(regAValue & 0x0F) - (std::int16_t)(d8 & 0x0F)) < 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (regAValue < d8)
		set_flag_carry();
	else
		clear_flag_carry();


	// Add to ticks
	if (indirect)
		ticks += 8;
	else
		ticks += 4;
}


// SBC A, X
void CPU::SBC(std::uint8_t d8, bool indirect=false)
{
	// Get reg->value and regA->value
	std::uint8_t regAValue, result, carryFlag;
	regAValue = get_register_8(CPU::REGISTERS::A);
	carryFlag = (std::uint8_t)get_flag_carry();

	result = regAValue - d8 - carryFlag;

	set_register(CPU::REGISTERS::A, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Set flag negative
	set_flag_subtract();

	// Check flag half carry
	if (((std::int16_t)(regAValue & 0x0F) - (std::int16_t)(d8 & 0x0F) - (std::int16_t)(carryFlag) < 0))
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (regAValue < d8 + carryFlag)
		set_flag_carry();
	else
		clear_flag_carry();


	// Add to ticks
	if (indirect)
		ticks += 8;
	else
		ticks += 4;
}


/*
	AND, XOR, OR, CP, INC, DEC methods
*/

// AND X
// AND (HL) when indirect == true
void CPU::AND(std::uint8_t d8, bool indirect=false)
{
	std::uint8_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue & regValue;

	set_register(CPU::REGISTERS::A, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag subtract, set flag half carry, clear flag carry
	clear_flag_subtract();
	set_flag_half_carry();
	clear_flag_carry();


	// Add to ticks
	if (indirect)
		ticks += 8;
	else
		ticks += 4;
}

// XOR X
// XOR (HL) when indirect == true
void CPU::XOR(std::uint8_t d8, bool indirect=false)
{
	std::uint8_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue ^ regValue;

	set_register(CPU::REGISTERS::A, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag subtract, flag half carry, flag carry
	clear_flag_subtract();
	clear_flag_half_carry();
	clear_flag_carry();


	// Add to ticks
	if (indirect)
		ticks += 8;
	else
		ticks += 4;
}

// OR X
void CPU::OR(std::uint8_t d8, bool indirect=false)
{
	std::uint8_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue | regValue;

	set_register(CPU::REGISTERS::A, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag subtract, flag half carry, flag carry
	clear_flag_subtract();
	clear_flag_half_carry();
	clear_flag_carry();


	// Add to ticks
	if (indirect)
		ticks += 8;
	else
		ticks += 4;
}

// CP X
// Like SUB() except don't save result to regA, do set flags though
void CPU::CP(std::uint8_t d8, bool indirect=false)
{
	// Get reg->value and regA->value
	std::uint8_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue - regValue;

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Set flag negative
	set_flag_subtract();

	// Check flag half carry
	if (((std::int16_t)(regAValue & 0x0F) - (std::int16_t)(d8 & 0x0F)) < 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (regAValue < d8)
		set_flag_carry();
	else
		clear_flag_carry();


	// Add to ticks
	if (indirect)
		ticks += 8;
	else
		ticks += 4;
}


// INC X
// INC (HL) when indirect == true
void CPU::INC(CPU::REGISTERS reg, bool indirect=false)
{
	// Get reg->value
	std::uint16_t regValue, result;

	if (!indirect)
		regValue = get_register_16(reg);					// Get reg->value
	else
		regValue = memory->readByte(get_register_16(reg));	// Get memory[reg]

	result = regValue + 1;

	set_register(reg, result);

	if (reg != CPU::REGISTERS::BC && reg != CPU::REGISTERS::DE && (reg != CPU::REGISTERS::HL && !indirect) && reg != CPU::REGISTERS::SP)
	{
		if (reg >= B)
			result &= 0x00FF;

		// Check flag zero
		if (result == 0)
			set_flag_zero();
		else
			clear_flag_zero();

		// Clear flag negative
		clear_flag_subtract();


		// Check flag half carry
		if (((regValue & 0x0F) + 1) > 0x0F)
			set_flag_half_carry();
		else
			clear_flag_half_carry();
	}


	// Add to ticks
	if ((reg == CPU::REGISTERS::HL && !indirect) || reg == CPU::REGISTERS::BC || reg == CPU::REGISTERS::DE || reg == CPU::REGISTERS::SP)
		ticks += 8;
	else if (reg == CPU::REGISTERS::HL && indirect)
		ticks += 12;
	else
		ticks += 4;
}

// DEC X
// DEC (HL) when indirect == true
void CPU::DEC(CPU::REGISTERS reg, bool indirect=false)
{
	std::uint16_t regValue, result;

	if (!indirect)
		regValue = get_register_16(reg);					// Get reg->value
	else
		regValue = memory->readByte(get_register_16(reg));	// Get memory[reg]

	result = regValue - 1;

	set_register(reg, result);

	if (reg != CPU::REGISTERS::BC && reg != CPU::REGISTERS::DE && (reg != CPU::REGISTERS::HL && !indirect) && reg != CPU::REGISTERS::SP)
	{
		if (reg >= B)
			result &= 0x00FF;

		// Check flag zero
		if (result == 0)
			set_flag_zero();
		else
			clear_flag_zero();

		// Set flag negative
		set_flag_subtract();


		// Check flag half carry
		if (((std::int16_t)(regValue & 0x0F) - 1) < 0)
			set_flag_half_carry();
		else
			clear_flag_half_carry();
	}


	// Add to ticks
	if ((reg == CPU::REGISTERS::HL && !indirect) || reg == CPU::REGISTERS::BC || reg == CPU::REGISTERS::DE || reg == CPU::REGISTERS::SP)
		ticks += 8;
	else if (reg == CPU::REGISTERS::HL && indirect)
		ticks += 12;
	else
		ticks += 4;
}


/*
	JP and JR methods
*/

// JP <CPU::FlagType>, a16
void CPU::JP(CPU::FLAGTYPES flagType, std::uint16_t addr)
{
	bool flagWasTrue = false;

	switch (flagType)
	{
	case CPU::FLAGTYPES::NZ:
		if (!get_flag_zero())
			flagWasTrue = true;
		break;
	
	case CPU::FLAGTYPES::NC:
		if (!get_flag_carry())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::Z:
		if (get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::C:
		if (get_flag_carry())
			flagWasTrue = true;
		break;

	default:
		flagWasTrue = true;
	}


	// Add to ticks
	if (flagWasTrue)
	{
		set_register(PC, addr);	// Jump!
		ticks += 16;
	}
	else
	{
		ticks += 12;
	}
}

// JP (HL)
void CPU::JP_INDIRECT(std::uint16_t addr)
{
	set_register(PC, addr);	// Jump!

	ticks += 4;
}


// JR r8	JR [NZ, NC, Z, C], r8
void CPU::JR(CPU::FLAGTYPES flagType, std::int8_t val)
{
	bool flagWasTrue = false;

	switch (flagType)
	{
	case CPU::FLAGTYPES::NZ:
		if (!get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::NC:
		if (!get_flag_carry())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::Z:
		if (get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::C:
		if (get_flag_carry())
			flagWasTrue = true;
		break;

	default:
		flagWasTrue = true;
	}


	// Add to ticks
	if (flagWasTrue)
	{
		set_register(PC, static_cast<std::uint16_t>(get_register_16(PC) + static_cast<std::int16_t>(val)));	// Jump!
		ticks += 12;
	}
	else
	{
		ticks += 8;
	}
}


/*
	RET and RETI
*/

// RET		RET [NZ, NC, Z, C]
void CPU::RET(CPU::FLAGTYPES flagType)
{
	bool flagWasTrue = false;

	switch (flagType)
	{
	case CPU::FLAGTYPES::NZ:
		if (!get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::NC:
		if (!get_flag_carry())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::Z:
		if (get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::C:
		if (get_flag_carry())
			flagWasTrue = true;
		break;

	default:
		flagWasTrue = true;
	}


	// Add to ticks
	if (flagWasTrue)
	{
		std::uint16_t spVal = 0;
		spVal |= getByteFromMemory(get_register_16(SP));
		set_register(SP, static_cast<std::uint16_t> (get_register_16(SP) + 1));
		spVal |= (getByteFromMemory(get_register_16(SP)) << 8);
		set_register(SP, static_cast<std::uint16_t> (get_register_16(SP) + 1));

		set_register(PC, spVal);	// Return!

		if (flagType == CPU::FLAGTYPES::NONE)
			ticks += 16;
		else
			ticks += 20;
	}
	else
	{
		ticks += 8;
	}
}

// RETI
void CPU::RETI()
{
	RET(CPU::FLAGTYPES::NONE);

	enable_interrupts();
}


/*
	Enabling and disabling Interrupts
*/

// EI
void CPU::enable_interrupts()
{
	interrupts_enabled = true;
	ticks += 4;
}

// DI
void CPU::disable_interrupts()
{
	interrupts_enabled = false;
	ticks += 4;
}


/*
	 RST [00H, 10H, 20H, 30H, 08H, 18H, 28H, 38H]
*/

// RST [00H, 10H, 20H, 30H, 08H, 18H, 28H, 38H]
void CPU::RST(std::uint8_t instruc)
{
	int8_t pcLow = 0;

	// Find out what pcLow should be
	pcLow |= ((instruc & 0xF0) - 0xC0);
	pcLow |= ((instruc & 0x0F) - 0x07);

	// (SP - 1) <- PChigh
	setByteToMemory(get_register_16(SP) - 1, static_cast<std::uint8_t> (get_register_16(PC) >> 8));

	// (SP - 2) <- PClow
	setByteToMemory(get_register_16(SP) - 2, static_cast<std::uint8_t> (get_register_16(PC) & 0x0F));

	// PChigh <- 0, PClow <- pcLow
	set_register(PC, static_cast<std::uint16_t> (0x0000 | pcLow));

	// Set SP
	set_register(SP, static_cast<std::uint16_t> (get_register_16(SP) - 2));

	ticks += 16;
}



/*
	CALLs
*/

// CALL a16		CALL [NZ, NC, Z, C], a16
void CPU::CALL(CPU::FLAGTYPES flagType, std::uint16_t a16)
{
	bool flagWasTrue = false;

	switch (flagType)
	{
	case CPU::FLAGTYPES::NZ:
		if (!get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::NC:
		if (!get_flag_carry())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::Z:
		if (get_flag_zero())
			flagWasTrue = true;
		break;

	case CPU::FLAGTYPES::C:
		if (get_flag_carry())
			flagWasTrue = true;
		break;

	default:
		flagWasTrue = true;
	}


	// Add to ticks
	if (flagWasTrue)
	{
		// (SP - 1) <- PChigh
		setByteToMemory(get_register_16(SP) - 1, static_cast<std::int8_t> (get_register_16(PC) >> 8));

		// (SP - 2) <- PClow
		setByteToMemory(get_register_16(SP) - 2, static_cast<std::int8_t> (get_register_16(PC) & 0xFF));

		// PC <- a16
		set_register(PC, a16);

		// Set SP
		set_register(SP, static_cast<std::uint16_t> (get_register_16(SP) - 2));

		ticks += 24;
	}
	else
	{
		ticks += 12;
	}
}


/*
	DAA (Decimal Adjust Accumulator)
*/

// DAA
void CPU::DAA()
{
	std::uint8_t aVal, result;
	aVal = result = get_register_8(A);

	if ((aVal & 0x0F) > 0x09 || get_flag_half_carry())
		result += 0x06;

	if ((aVal & 0xF0) > 0x90 || get_flag_carry())
		result += 0x60;

	set_register(A, result);

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag carry
	if ((aVal & 0x80) == 0 && (result & 0x80) > 0)
		set_flag_carry();
	else
		clear_flag_carry();

	// Clear flag half carry
	clear_flag_half_carry();

	ticks += 4;
}


/*
	SCF (Set carry flag), CCF (Complement carry flag), and CPL (Complement A)
*/

// SCF
void CPU::SCF()
{
	set_flag_carry();
	clear_flag_subtract();
	clear_flag_half_carry();

	ticks += 4;
}

// CCF
void CPU::CCF()
{
	// Complement the carry flag
	if (get_flag_carry())
		clear_flag_carry();
	else
		set_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	ticks += 4;
}

// CPL
void CPU::CPL()
{
	// Complement register A
	std::uint8_t aVal = get_register_8(A);
	aVal = ~aVal;
	set_register(A, aVal);

	set_flag_subtract();
	set_flag_half_carry();

	ticks += 4;
}



/*
	HALT, NOP, STOP
*/
void CPU::HALT()
{
	is_halted = true;
	ticks += 4;
}

void CPU::NOP()
{
	ticks += 4;
}


void CPU::STOP()
{
	is_stopped = true;
	ticks += 4;
}



/*
	RLCA, RLA, RRCA, and RRA
	Rotate [Left, Right] [Circular] Accumulator
*/
void CPU::RLCA()
{
	std::uint8_t aVal, bit7;
	aVal = get_register_8(A);
	bit7 = (aVal >> 7);

	aVal = (aVal << 1) | bit7;

	set_register(A, aVal);

	if (bit7)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_zero();
	clear_flag_subtract();
	clear_flag_half_carry();

	ticks += 4;
}

void CPU::RLA()
{
	std::uint8_t aVal, bit7;
	aVal = get_register_8(A);
	bit7 = (aVal >> 7);

	aVal = (aVal << 1) | (std::uint8_t) (get_flag_carry());

	set_register(A, aVal);

	if (bit7)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_zero();
	clear_flag_subtract();
	clear_flag_half_carry();

	ticks += 4;
}

void CPU::RRCA()
{
	std::uint8_t aVal, bit0;
	aVal = get_register_8(A);
	bit0 = (aVal & 0x01);

	aVal = (aVal >> 1) | (bit0 << 7);

	set_register(A, aVal);

	if (bit0)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	ticks += 4;
}

void CPU::RRA()
{
	std::uint8_t aVal, bit0;
	aVal = get_register_8(A);
	bit0 = (aVal & 0x01);

	aVal = (aVal >> 1) | (static_cast<std::uint8_t> (get_flag_carry()) << 7);

	set_register(A, aVal);

	if (bit0)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	ticks += 4;
}


/*
	PUSH and POP
*/
void CPU::PUSH(CPU::REGISTERS reg)
{
	std::uint16_t regVal = get_register_16(reg);
	std::uint8_t high, low;
	high = (regVal >> 8) & 0xFF;
	low = (regVal & 0xFF);
	setByteToMemory(get_register_16(SP) - 1, high);
	setByteToMemory(get_register_16(SP)  - 2, low);
	/*setByteToMemory(get_register_16(SP), high);
	setByteToMemory(get_register_16(SP) - 1, low);*/

	registers[SP] -= 2;

	ticks += 16;
}

void CPU::POP(CPU::REGISTERS reg)
{
	std::uint16_t regVal = 0;
	std::uint8_t high, low;

	low = getByteFromMemory(get_register_16(SP));
	high = getByteFromMemory(get_register_16(SP) + 1);

	if (reg == AF)
	{
		low &= 0xF0;
	}

	regVal |= static_cast<std::uint16_t> (high << 8);
	regVal |= low;

	set_register(reg, regVal);

	registers[SP] += 2;

	ticks += 12;
}








/*
	Prefix CB opcode handling
*/

void CPU::handle_CB(std::int8_t instruc)
{

	int regPattern1, regPattern2;
	regPattern1 = (instruc / 0x08) - 0x08;	// B, B, B, B, B, B, B, B, C, C, C, C, C, C, C, C, D, D, etc.
	regPattern2 = (instruc & 0x0F) % 0x08;	// B, C, D, E, H, L, HL, A, B, C, D, etc.

	ticks += 4;
	registers[PC]++;

	switch (instruc)
	{

		/*
			RLC, RRC, RL, RR
		*/

		// RLC [B, C, D, E, H, L, (HL), A]
	case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:

		RLC((CPU::REGISTERS) reg_list[regPattern2]);
		break;


		// RRC [B, C, D, E, H, L, (HL), A]
	case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:

		RRC((CPU::REGISTERS) reg_list[regPattern2]);
		break;
		

		// RL [B, C, D, E, H, L, (HL), A]
	case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:

		RL((CPU::REGISTERS) reg_list[regPattern2]);
		break;

		
		// RR [B, C, D, E, H, L, (HL), A]
	case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:

		RR((CPU::REGISTERS) reg_list[regPattern2]);
		break;



		/*
			SLA, SRA, SRL
		*/

		// SLA [B, C, D, E, H, L, (HL), A]
	case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:

		SLA((CPU::REGISTERS) reg_list[regPattern2]);
		break;


		// SRA [B, C, D, E, H, L, (HL), A]
	case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F:

		SRA((CPU::REGISTERS) reg_list[regPattern2]);
		break;


		// SRL [B, C, D, E, H, L, (HL), A]
	case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:

		SRL((CPU::REGISTERS) reg_list[regPattern2]);
		break;

		
		/*
			SWAP
		*/

		// SWAP [B, C, D, E, H, L, (HL), A]
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:

		SWAP((CPU::REGISTERS) reg_list[regPattern2]);
		break;



		/*
			BIT, SET, RES
		*/

		// BIT [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
	case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
	case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:

		BIT(static_cast<std::uint8_t> (regPattern1), reg_list[regPattern2]);
		break;


		// SET [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
	case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7: case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF:
	case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7: case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF:
	case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE6: case 0xE7: case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF:
	case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF7: case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF:

		SET(static_cast<std::uint8_t> (regPattern1), reg_list[regPattern2]);
		break;

		// RES [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
	case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F:
	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F:
	case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7: case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF:
	case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:

		RES(static_cast<std::uint8_t> (regPattern1), reg_list[regPattern2]);
		break;


	default:
		printf("Error - Do not know how to handle Prefixed CB opcode %i\n", instruc);

	}// end switch()
}


/*
	RLC, RRC, RL, RR
*/

// RLC [B, C, D, E, H, L, (HL), A]
void CPU::RLC(CPU::REGISTERS reg)
{
	std::uint8_t regVal, bit7;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
	{
		regVal = getByteFromMemory(get_register_16(reg));
	}
	else
	{
		regVal = get_register_8(reg);
	}


	bit7 = (regVal >> 7);
	regVal = (regVal << 1) | bit7;

	// Set register
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	if (bit7)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}


// RL [B, C, D, E, H, L, (HL), A]
void CPU::RL(CPU::REGISTERS reg)
{
	std::uint8_t regVal, bit7;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
	{
		regVal = getByteFromMemory(get_register_16(reg));
	}
	else
	{
		regVal = get_register_8(reg);
	}


	bit7 = (regVal >> 7);
	regVal = (regVal << 1) | static_cast<std::uint8_t> (get_flag_carry());

	// Set register
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	if (bit7)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}


// RRC [B, C, D, E, H, L, (HL), A]
void CPU::RRC(CPU::REGISTERS reg)
{
	std::uint8_t regVal, bit0;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
	{
		regVal = getByteFromMemory(get_register_16(reg));
	}
	else
	{
		regVal = get_register_8(reg);
	}


	bit0 = (regVal & 0x01);
	regVal = (regVal >> 1) | (bit0 << 7);

	// Set register
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	if (bit0)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}

// RR [B, C, D, E, H, L, (HL), A]
void CPU::RR(CPU::REGISTERS reg)
{
	std::uint8_t regVal, bit0;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
	{
		regVal = getByteFromMemory(get_register_16(reg));
	}
	else
	{
		regVal = get_register_8(reg);
	}


	bit0 = (regVal & 0x01);
	regVal = (regVal >> 1) | (static_cast<std::uint8_t> (get_flag_carry()) << 7);

	// Set register
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	if (bit0)
		set_flag_carry();
	else
		clear_flag_carry();

	clear_flag_subtract();
	clear_flag_half_carry();

	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}


/*
	BIT, SET, RES
*/

// BIT [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
void CPU::BIT(std::uint8_t getBit, CPU::REGISTERS reg)
{
	std::uint8_t bit, regVal;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Get bit to be used
	bit = (regVal >> getBit) & 0x01;


	// Set/clear flags
	if (bit)
		clear_flag_zero();
	else
		set_flag_zero();

	clear_flag_subtract();
	set_flag_half_carry();


	if (indirect)
		ticks += 12;
	else
		ticks += 8;
}

// SET [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
void CPU::SET(std::uint8_t setBit, CPU::REGISTERS reg)
{
	std::uint8_t bit, regVal;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Set bit
	bit = (0x01 << setBit);
	regVal |= bit;

	// Save modified regVal
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);
	

	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}

// RES [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
void CPU::RES(std::uint8_t setBit, CPU::REGISTERS reg)
{
	std::uint8_t bit, regVal;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Clear bit
	bit = (0x01 << setBit);
	bit = ~bit;
	regVal &= bit;

	// Save modified regVal
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);


	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}






/*
	SLA, SRA, SRL
*/

// SLA [B, C, D, E, H, L, (HL), A]
void CPU::SLA(CPU::REGISTERS reg)
{
	std::uint8_t bit7, regVal;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Get bit7, Shift regVal left 1
	bit7 = regVal >> 7;
	regVal = regVal << 1;

	// Save modified regVal
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	if (bit7)
		set_flag_carry();
	else
		clear_flag_carry();

	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	clear_flag_subtract();
	clear_flag_half_carry();

	// Add to ticks
	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}

// SRA [B, C, D, E, H, L, (HL), A]
void CPU::SRA(CPU::REGISTERS reg)
{
	std::uint8_t bit0, bit7, regVal;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Get bit0 and bit7, Shift regVal right 1, keep original bit7 after shift
	bit0 = regVal & 0x01;
	bit7 = regVal & 0x80;
	regVal = regVal >> 1;
	regVal |= bit7;

	// Save modified regVal
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	clear_flag_carry();

	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	clear_flag_subtract();
	clear_flag_half_carry();

	// Add to ticks
	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}

// SRL [B, C, D, E, H, L, (HL), A]
void CPU::SRL(CPU::REGISTERS reg)
{
	std::uint8_t bit0, regVal;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Get bit0, Shift regVal right 1
	bit0 = regVal & 0x01;
	regVal = regVal >> 1;

	// Save modified regVal
	if (indirect)
		setByteToMemory(get_register_16(reg), regVal);
	else
		set_register(reg, regVal);

	// Set/clear flags
	if (bit0)
		set_flag_carry();
	else
		clear_flag_carry();

	if (regVal == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	clear_flag_subtract();
	clear_flag_half_carry();

	// Add to ticks
	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}




/*
	SWAP
*/

// SWAP [B, C, D, E, H, L, (HL), A]
void CPU::SWAP(CPU::REGISTERS reg)
{
	std::uint8_t regVal, high, low, result;
	bool indirect = false;

	if (reg == CPU::REGISTERS::HL)
		indirect = true;

	// Get register value
	if (indirect)
		regVal = getByteFromMemory(get_register_16(reg));
	else
		regVal = get_register_8(reg);

	// Perform half-byte swap, e.g. 0001 0010 -> 0010 0001
	high = (regVal >> 4) & 0x0F;
	low = regVal & 0x0F;

	result = (low << 4);
	result |= high;

	// Save modified regVal
	if (indirect)
		setByteToMemory(get_register_16(reg), result);
	else
		set_register(reg, result);

	// Set/clear flags
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	clear_flag_subtract();
	clear_flag_half_carry();
	clear_flag_carry();

	// Add to ticks
	if (indirect)
		ticks += 16;
	else
		ticks += 8;
}



