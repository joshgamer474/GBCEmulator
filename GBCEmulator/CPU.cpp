#include "stdafx.h"
#include "CPU.h"


CPU::CPU()
{
	operationHandling = new CPU_Opcodes();
	memory = new Memory();

	registers.resize(NUM_OF_REGISTERS);
	ticks = 0;
}

CPU::~CPU()
{
	delete operationHandling;
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


std::int8_t CPU::get_register_8(REGISTERS reg)
{
	if (reg < B)
	{
		printf("Error - Using get_register_8() to get register: %i", reg);
		return 0;
	}
	else
	{
		if (reg % 2 == 0)	// Return upper 8
		{
			return static_cast<std::int8_t> (registers[(reg - B) / 2] & 0xFF00) >> 8;
		}
		else				// Return lower 8
		{
			return static_cast<std::int8_t> (registers[(reg - B) / 2] & 0x00FF);
		}
	}
}


std::int16_t CPU::get_register_16(REGISTERS reg) 
{ 
	if (reg == CPU::REGISTERS::SP || reg == CPU::REGISTERS::PC)	// Return full 16 bits
	{
		return registers[reg];
	}
	if (reg < B)			// Return little endian converted back to big endian
	{
		std::int16_t littleEndian = registers[reg];
		std::int8_t lowerByte = static_cast<std::int8_t> ((littleEndian >> 8) & 0xFF);
		std::int8_t upperByte = static_cast<std::int8_t> (littleEndian & 0x00FF);
		std::int16_t bigEndian = 0x0000;
		bigEndian |= (upperByte << 8);
		bigEndian |= lowerByte;
		return bigEndian;
	}
	else
	{
		if (reg % 2 == 0)	// Return upper 8
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
void CPU::set_register(REGISTERS reg, std::int16_t val)
{
	if (reg == CPU::REGISTERS::SP || reg == CPU::REGISTERS::PC)	// Set full 16 bits
	{
		registers[reg] = val;
	}
	else if (reg < B)	// Set full 16 bits in little endian
	{
		std::int8_t upperByte = static_cast<std::int8_t> ((val >> 8) & 0xFF);
		std::int8_t lowerByte = static_cast<std::int8_t> (val & 0x00FF);
		std::int16_t littleEndian = 0x0000;
		littleEndian |= (lowerByte << 8);
		littleEndian |= upperByte;
		registers[reg] = littleEndian;
	}
	else			// Set only 8 bits
	{
		//printf("Error in set_register() - reg >= B, reg: %i", reg);
		set_register(reg, (std::int8_t) (val & 0x00FF));
	}
}

void CPU::set_register(REGISTERS reg, std::int8_t val)
{
	if (reg >= B)
	{
		std::int16_t *reg_ptr = &registers[(reg - B) / 2];

		if (reg % 2 == 0)	// Is upper 8 bits
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
		printf("Error in set_register() - reg < B, reg: %i", reg);
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

void CPU::set_flag_zero()			{ registers[AF] |= 0x0080; }
void CPU::set_flag_subtract()		{ registers[AF] |= 0x0040; }
void CPU::set_flag_half_carry()		{ registers[AF] |= 0x0020; }
void CPU::set_flag_carry()			{ registers[AF] |= 0x0010; }

void CPU::clear_flag_zero()			{ registers[AF] &= 0xFF7F; }
void CPU::clear_flag_subtract()		{ registers[AF] &= 0xFFBF; }
void CPU::clear_flag_half_carry()	{ registers[AF] &= 0xFFDF; }
void CPU::clear_flag_carry()		{ registers[AF] &= 0xFFEF; }




/*
	Debug Methods
*/
void CPU::printRegisters()
{
	printf("------------------------------\n");
	printf("\t\tRegisters\n");
	printf("------------------------------\n");
	for (int i = 0; i < NUM_OF_REGISTERS; i++)
	{
		printf("%i: %#010x\n", (CPU::REGISTERS) i, registers[i]);
	}
	printf("\n");
}

/*
	Instruction Methods
*/

// Get instruction from Ram[PC]
void CPU::getInstruction()
{
	//instruction = ;
}

bool CPU::runInstruction(std::int8_t instruc)
{

	int regPattern1, regPattern2;
	regPattern1 = (instruc / 0x08) - 0x08;	// B, B, B, B, B, B, B, B, C, C, C, C, C, C, C, C, D, D, etc.
	regPattern2 = (instruc & 0x0F) % 0x08;	// B, C, D, E, H, L, A, B, C, D, etc.


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

		registers[PC]++;
		LD((CPU::REGISTERS) operationHandling->reg_list[regPattern1], (CPU::REGISTERS) operationHandling->reg_list[regPattern2]);
		break;


		// LD A, (Y)
	case 0x0A: case 0x1A:

		registers[PC]++;
		LD(A, getByteFromMemory((CPU::REGISTERS) operationHandling->reg_list[(instruc & 0xF0) >> 4]), false);	// (BC), (DE)
		break;

		// LD A, (HL+-)
	case 0x2A: case 0x3A:

		registers[PC]++;
		LD(A, getByteFromMemory(HL), false);

		if (instruc == 0x2A)
			set_register(HL, static_cast<std::int16_t> (get_register_16(HL) + 1));	// HL+
		else
			set_register(HL, static_cast<std::int16_t> (get_register_16(HL) - 1));	// HL-
		break;
		break;

		// LD X, (HL)
	case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x7E:

		registers[PC]++;
		LD((CPU::REGISTERS) operationHandling->reg_list[regPattern1], getByteFromMemory(HL), false);
		break;


		// LD (HL), Y
	case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:			case 0x77:

		registers[PC]++;
		LD_reg_into_memory(HL, (CPU::REGISTERS) operationHandling->reg_list[regPattern2]);
		break;

		// LD [(BC), (DE)], A
	case 0x02: case 0x12:

		registers[PC]++;
		LD_reg_into_memory((CPU::REGISTERS) ((instruc >> 4) & 0x0F), A);
		break;

		//LD (HL+-), Y
	case 0x22: case 0x32:

		registers[PC]++;
		LD_reg_into_memory(HL, A);

		if (instruc == 0x22)
			set_register(HL, static_cast<std::int16_t> (get_register_16(HL) + 1));	// HL+
		else
			set_register(HL, static_cast<std::int16_t> (get_register_16(HL) - 1));	// HL-
		break;


		// LD X, d8
	case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E:		case 0x3E:

		registers[PC]++;
		LD((CPU::REGISTERS) operationHandling->reg_list[regPattern1], getByteFromMemory(PC), false);
		registers[PC]++;
		break;

		// LD (HL), d8
	case 0x36:

		registers[PC]++;
		LD(HL, getByteFromMemory(PC), true);
		registers[PC]++;
		break;

		// LD (a16), A
	case 0xEA:

		registers[PC]++;
		std::int16_t a16 = getNextTwoBytes();
		LD(a16, get_register_8(A));
		break;

		// LD A, (a16)
	case 0xFA:

		registers[PC]++;
		std::int16_t a16 = getNextTwoBytes();
		LD_INDIRECT_A16(A, a16);
		break;

		// LD XY, d16
	case 0x01: case 0x11: case 0x21: case 0x31:

		registers[PC]++;
		std::int16_t d16 = getNextTwoBytes();

		if (instruc == 0x31)
			LD((CPU::REGISTERS) SP, d16);
		else
			LD((CPU::REGISTERS) ((instruc & 0xF0) >> 4), d16);	// BC, DE, HL

		break;

		// LD SP, HL
	case 0xF9:
		
		registers[PC]++;
		LD(SP, HL);
		break;

		// LD (a16), SP
	case 0x08:

		registers[PC]++;
		std::int16_t a16 = getNextTwoBytes();
		LD(a16, get_register_16(SP));
		break;


		// LDH A, (a8)  = LD A, (0xFF00 + a8)
	case 0xF0:

		registers[PC]++;
		std::int8_t a8 = getByteFromMemory(PC);
		std::int8_t parenA8 = getByteFromMemory(static_cast<std::int16_t> (0xFF00 + a8));
		LDH(A, parenA8);
		break;

		// LDH (a8), A = LD (0xFF00 + a8), A
	case 0xE0:

		registers[PC]++;
		std::int8_t a8 = getByteFromMemory(PC);
		LDH_INDIRECT(static_cast<std::int16_t> (0xFF00 + a8), get_register_8(A));
		break;

		// LD HL, SP+r8
	case 0xF8:

		registers[PC]++;
		std::int8_t r8 = getByteFromMemory(PC);
		LD_HL_SPPLUSR8(HL, r8);
		break;


		/*
			ADD
		*/


		// ADD A, Y
	case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:		case 0x87:

		registers[PC]++;
		ADD(CPU::REGISTERS::A, get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// ADD A, (HL)
	case 0x86:

		registers[PC]++;
		ADD(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// ADD A, d8
	case 0xC6:

		registers[PC]++;
		ADD(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;

		// ADD HL, XY
	case 0x09: case 0x19: case 0x29: case 0x39:

		registers[PC]++;
		if (instruc == 0x39)
			ADD_HL((CPU::REGISTERS) SP);
		else
			ADD_HL((CPU::REGISTERS) ((instruc & 0xF0) >> 4));	// BC, DE, HL
		break;

		// ADD SP, r8
	case 0xE8:

		registers[PC]++;
		std::int8_t r8 = getByteFromMemory(get_register_16(PC));
		registers[PC]++;
		ADD_SP_R8(SP, r8);
		break;


		/*
			ADC
		*/

		// ADC A, Y
	case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D:		 case 0x8F:

		registers[PC]++;
		ADC(CPU::REGISTERS::A, get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// ADC A, (HL)
	case 0x8E:

		registers[PC]++;
		ADC(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// ADC A, d8
	case 0xCE:

		registers[PC]++;
		ADC(CPU::REGISTERS::A, memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;



		/*
			SUB
		*/

		// SUB X
	case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95:		 case 0x97:

		registers[PC]++;
		SUB(get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// SUB (HL)
	case 0x96:

		registers[PC]++;
		SUB(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// SUB d8
	case 0xD6:

		registers[PC]++;
		SUB(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;



		/*
			SBC
		*/

		// SBC X
	case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D:		 case 0x9F:

		registers[PC]++;
		SBC(get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// SBC A, (HL)
	case 0x9E:

		registers[PC]++;
		SBC(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// SBC d8
	case 0xDE:

		registers[PC]++;
		SBC(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;




		/*
			AND
		*/

		// AND X
	case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5:		 case 0xA7:

		registers[PC]++;
		AND(get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// AND (HL)
	case 0xA6:

		registers[PC]++;
		AND(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// AND d8
	case 0xE6:

		registers[PC]++;
		AND(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;




		/*
			XOR
		*/

		// XOR X
	case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD:		 case 0xAF:

		registers[PC]++;
		XOR(get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// XOR (HL)
	case 0xAE:

		registers[PC]++;
		XOR(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// XOR d8
	case 0xEE:

		registers[PC]++;
		XOR(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;



		/*
			OR
		*/

		// OR X
	case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5:		 case 0xB7:

		registers[PC]++;
		OR(get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// OR (HL)
	case 0xB6:

		registers[PC]++;
		OR(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// OR d8
	case 0xF6:

		registers[PC]++;
		OR(memory->readByte(get_register_16((CPU::REGISTERS) PC)), true);
		registers[PC]++;
		break;


		/*
			CP
		*/

		// CP X
	case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD:		 case 0xBF:

		registers[PC]++;
		CP(get_register_8((CPU::REGISTERS) operationHandling->reg_list[regPattern2]), false);
		break;

		// CP X
	case 0xBE:

		registers[PC]++;
		CP(memory->readByte(get_register_16((CPU::REGISTERS) HL)), true);
		break;

		// CP d8
	case 0xFE:

		registers[PC]++;
		CP(memory->readByte(get_register_16((CPU::REGISTERS) PC)), false);
		registers[PC]++;
		break;



		/*
			IND
		*/

		// INC XY
	case 0x03: case 0x13: case 0x23: case 0x33:

		registers[PC]++;
		if (instruc == 0x33)
			INC((CPU::REGISTERS) SP, false);
		else
			INC((CPU::REGISTERS) ((instruc & 0xF0) >> 4), false);	// INC BC, DE, HL
		break;

		// INC X
	case 0x04: case 0x14: case 0x24: case 0x34:
	case 0x0C: case 0x1C: case 0x2C: case 0x3C:

		registers[PC]++;
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

		registers[PC]++;
		if (instruc == 0x3B)
			DEC((CPU::REGISTERS) SP, false);
		else
			DEC((CPU::REGISTERS) ((instruc & 0xF0) >> 4), false);	// DEC BC, DE, HL
		break;

		// DEC X
	case 0x05: case 0x15: case 0x25: case 0x35:
	case 0x0D: case 0x1D: case 0x2D: case 0x3D:

		registers[PC]++;
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
			JP
		*/

		// JP a16
	case 0xC2: case 0xC3: case 0xCA: case 0xD2: case 0xDA:

		registers[PC]++;
		std::int16_t addr = getNextTwoBytes();
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

		registers[PC]++;
		std::int16_t hlVal = 0x0000;
		hlVal |= getByteFromMemory(HL);
		JP_INDIRECT(hlVal);
		break;


		/*
			JR
		*/
		// JR r8		JR [NZ, NC, Z, C], r8
	case 0x18: case 0x20: case 0x28: case 0x30: case 0x38:

		registers[PC]++;
		std::int8_t r8 = getByteFromMemory(CPU::REGISTERS::PC);
		registers[PC]++;
		if (instruc == 0xC3)
			JR(CPU::FLAGTYPES::NONE, r8);						// JP r8
		else
		{
			std::int8_t flagType = ((instruc & 0xF0) >> 4) - 0x02;	// 0, 1
			if ((instruc & 0x0F) == 0x08)
				JP((CPU::FLAGTYPES) (flagType + 2), addr);		// JP [Z, C], r8
			else
				JP((CPU::FLAGTYPES) flagType, addr);			// JP [NZ, NC], r8

		}
		break;

	}// end switch()

	

	return false;
}










/*
	Memory methods
*/

// Perform (reg)
int8_t CPU::getByteFromMemory(CPU::REGISTERS reg)
{
	return memory->readByte(get_register_16(reg));
}

// Perform (addr)
int8_t CPU::getByteFromMemory(std::int16_t addr)
{
	return memory->readByte(addr);
}

void CPU::setByteToMemory(int16_t addr, int8_t val)
{
	memory->setByte(addr, val);
}



int16_t CPU::getNextTwoBytes()
{
	std::int16_t d16 = 0x0000;
	d16 |= getByteFromMemory(PC);
	registers[PC]++;
	d16 |= ((static_cast<std::int16_t>(getByteFromMemory(PC)) << 8) & 0xFF00);
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
	int8_t val = get_register_8(reg2);

	// Get address from reg1
	int16_t addr = get_register_16(reg1);

	setByteToMemory(addr, val);

	// Add to ticks
	ticks += 8;
}

// LD X, d8 and LD (X), d8 when indirect == true
void CPU::LD(CPU::REGISTERS reg, int8_t val, bool indirect=false)
{
	if (!indirect)
		set_register(reg, val);
	else
	{
		// Get address to write to
		int16_t addr = get_register_16(reg);
		setByteToMemory(addr, val);
	}

	// Add to ticks
	if (!indirect)
		ticks += 8;
	else
		ticks += 12;
}


// LD XY, d16
void CPU::LD(CPU::REGISTERS reg, std::int16_t val)
{
	set_register(reg, val);

	// Add to ticks
	ticks += 12;
}

// LD (a16), val
void CPU::LD(std::int16_t addr, std::int8_t val)
{
	setByteToMemory(addr, val);

	// Add to ticks
	ticks += 16;
}

// LD (a16), SP
void CPU::LD(std::int16_t addr, std::int16_t val)
{
	std::int8_t upperByte = static_cast<std::int8_t> ((val >> 8) & 0xFF);
	std::int8_t lowerByte = static_cast<std::int8_t> (val & 0xFF);

	setByteToMemory(addr, lowerByte);
	setByteToMemory(addr + 1, upperByte);

	// Add to ticks
	ticks += 20;
}


// LD A, (a16)
void CPU::LD_INDIRECT_A16(CPU::REGISTERS reg, std::int16_t addr)
{
	// Read in addr->val
	std::int8_t val = getByteFromMemory(addr);

	set_register(reg, val);

	// Add to ticks
	ticks += 16;
}

// LDH A, (a8)
void CPU::LDH(CPU::REGISTERS reg, std::int8_t val)
{
	set_register(reg, val);

	// Add to ticks
	ticks += 12;
}

// LDH (a8), A
void CPU::LDH_INDIRECT(std::int16_t addr, std::int8_t val)
{
	setByteToMemory(addr, val);

	// Add to ticks
	ticks += 16;
}

// LD HL, SP+r8
void CPU::LD_HL_SPPLUSR8(CPU::REGISTERS reg, std::int8_t r8)
{
	std::int16_t spVal = get_register_16(SP);
	std::int16_t result = spVal + r8;
	set_register(reg, result);


	// Clear flag zero
	clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if (spVal & 0x0100 == 0 && result & 0x0100 > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (spVal & 0x8000 == 0 && result & 0x8000 > 0)
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
void CPU::ADD(CPU::REGISTERS reg, std::int8_t d8, bool indirect=false)
{
	// Get reg1->value and reg2->value
	std::int16_t r, result;
	r = get_register_8(reg);

	result = r + d8;

	set_register(reg, static_cast<std::int8_t> (result & 0x00FF));

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if (r & 0x0010 == 0 && result & 0x0010 > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (r & 0x0080 == 0 && result & 0x0080 > 0)
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
void CPU::ADC(CPU::REGISTERS reg, std::int8_t val, bool indirect=false)
{
	ADD(reg, val, indirect);

	if (get_flag_carry())
		set_register(reg, static_cast<std::int8_t> (get_register_8(reg) + 0x0001));
}


// ADD HL, XY
void CPU::ADD_HL(CPU::REGISTERS reg)
{
	std::int16_t hlVal, regVal, result;
	hlVal = get_register_16(HL);
	regVal = get_register_16(reg);

	result = hlVal + regVal;

	set_register(reg, static_cast<std::int8_t> (result & 0x00FF));

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if (hlVal & 0x0100 == 0 && result & 0x0100 > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (hlVal & 0x8000 == 0 && result & 0x8000 > 0)
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
	std::int16_t result = spVal + r8;

	// Clear flag zero
	clear_flag_zero();

	// Clear flag negative
	clear_flag_subtract();

	// Check flag half carry
	if (spVal & 0x0100 == 0 && result & 0x0100 > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (spVal & 0x8000 == 0 && result & 0x8000 > 0)
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
void CPU::SUB(std::int8_t d8, bool indirect=false)
{
	// Get reg->value and regA->value
	std::int16_t regAValue, result;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue - d8;

	set_register(CPU::REGISTERS::A, static_cast<std::int8_t> (result & 0x00FF));

	// Check flag zero
	if (result == 0)
		set_flag_zero();
	else
		clear_flag_zero();

	// Set flag negative
	set_flag_subtract();

	// Check flag half carry
	if (regAValue & 0x0010 == 0 && result & 0x0010 > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (regAValue & 0x0080 == 0 && result & 0x0080 > 0)
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
void CPU::SBC(std::int8_t d8, bool indirect=false)
{
	SUB(d8, indirect);

	if (get_flag_carry())
		set_register(A, static_cast<std::int8_t> (get_register_8(A) - 0x0001));
}


/*
	AND, XOR, OR, CP, INC, DEC methods
*/

// AND X
// AND (HL) when indirect == true
void CPU::AND(std::int8_t d8, bool indirect=false)
{
	std::int16_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue & regValue;

	set_register(CPU::REGISTERS::A, static_cast<std::int8_t> (result & 0x00FF));

	// Check flag zero
	if (result == 0)
		set_flag_carry();
	else
		clear_flag_carry();

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
void CPU::XOR(std::int8_t d8, bool indirect=false)
{
	std::int16_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue ^ regValue;

	set_register(CPU::REGISTERS::A, static_cast<std::int8_t> (result & 0x00FF));

	// Check flag zero
	if (result == 0)
		set_flag_carry();
	else
		clear_flag_carry();

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
void CPU::OR(std::int8_t d8, bool indirect=false)
{
	std::int16_t regValue, regAValue, result;
	regValue = d8;
	regAValue = get_register_8(CPU::REGISTERS::A);

	result = regAValue | regValue;

	set_register(CPU::REGISTERS::A, static_cast<std::int8_t> (result & 0x00FF));

	// Check flag zero
	if (result == 0)
		set_flag_carry();
	else
		clear_flag_carry();

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
void CPU::CP(std::int8_t d8, bool indirect=false)
{
	// Get reg->value and regA->value
	std::int16_t regValue, regAValue, result;
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
	if (regAValue & 0x0010 == 0 && result & 0x0010 > 0)
		set_flag_half_carry();
	else
		clear_flag_half_carry();

	// Check flag carry
	if (regAValue & 0x0080 == 0 && result & 0x0080 > 0)
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
	std::int16_t regValue, result;

	if (!indirect)
		regValue = get_register_16(reg);					// Get reg->value
	else
		regValue = memory->readByte(get_register_16(reg));	// Get memory[reg]

	result = regValue + 1;


	if (reg != CPU::REGISTERS::BC && reg != CPU::REGISTERS::DE && (reg != CPU::REGISTERS::HL && !indirect) && reg != CPU::REGISTERS::SP)
	{

		// Check flag zero
		if (result == 0)
			set_flag_zero();
		else
			clear_flag_zero();

		// Clear flag negative
		clear_flag_subtract();


		// Check flag half carry
		if (regValue & 0x0010 == 0 && result & 0x0010 > 0)
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
	std::int16_t regValue, result;

	if (!indirect)
		regValue = get_register_16(reg);					// Get reg->value
	else
		regValue = memory->readByte(get_register_16(reg));	// Get memory[reg]

	result = regValue - 1;


	if (reg != CPU::REGISTERS::BC && reg != CPU::REGISTERS::DE && (reg != CPU::REGISTERS::HL && !indirect) && reg != CPU::REGISTERS::SP)
	{

		// Check flag zero
		if (result == 0)
			set_flag_zero();
		else
			clear_flag_zero();

		// Set flag negative
		set_flag_subtract();


		// Check flag half carry
		if (regValue & 0x0010 == 0 && result & 0x0010 > 0)
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
void CPU::JP(CPU::FLAGTYPES flagType, std::int16_t addr)
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
void CPU::JP_INDIRECT(std::int16_t addr)
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
		set_register(PC, static_cast<std::int16_t>(get_register_16(PC) + static_cast<std::int16_t>(val)));	// Jump!
		ticks += 12;
	}
	else
	{
		ticks += 8;
	}
}