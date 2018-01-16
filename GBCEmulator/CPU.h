#pragma once

#include <cstdint>
#include <vector>
#include "Memory.h"

#ifndef CPU_H
#define CPU_H

class CPU_Opcodes;

class CPU
{
	int NUM_OF_REGISTERS = 6;

	double CLOCK_SPEED = 4.194304 * 1000000; // Hz
	double CLOCK_SPEED_GBC_MAX = 8.4 * 1000000; // Hz

private:

	// Registers
	std::vector<std::int16_t> registers;
	std::int8_t instruction;

	std::uint16_t ticks;

	//Memory memory;

public:

	CPU();
	~CPU();

	Memory *memory;

	bool interrupts_enabled;
	bool is_halted;
	bool is_stopped;

	/*
		Debug stuff
	*/
	void printRegisters();


	/*
		Instruction stuff
	*/
	void getInstruction();
	bool runInstruction(std::int8_t);

	// Opcodes handling
	CPU_Opcodes *operationHandling;


	/*
		Register stuff
	*/

	enum REGISTERS
	{
		BC,	// 16bit reg
		DE,	// 16bit reg
		HL,	// Used for indirect addressing
		AF,	// Accumulator and Flag reg
		SP,	// Stack Pointer, should be 0xFFFE on gameboy start up - Stack grows from top to down
		PC,	// 16bit Program Counter reg
		B,
		C,
		D,
		E,
		H,
		L,
		A,
		F
	};

	std::vector<REGISTERS> reg_list = { CPU::B, CPU::C, CPU::D, CPU::E, CPU::H, CPU::L, CPU::HL, CPU::A };

	enum class FLAGTYPES
	{
		NZ,		// Not zero
		NC,		// Not carry
		Z,		// Zero
		C,		// Carry
		NONE
	};


	// Register getter
	std::int8_t get_register_8(REGISTERS reg);
	std::int16_t get_register_16(REGISTERS reg);


	// Register setters
	void set_register(REGISTERS reg, std::int16_t);
	void set_register(REGISTERS reg, std::int8_t);


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

	bool get_flag_zero();
	bool get_flag_subtract();
	bool get_flag_half_carry();
	bool get_flag_carry();

	void set_flag_zero();
	void set_flag_subtract();
	void set_flag_half_carry();
	void set_flag_carry();

	void clear_flag_zero();
	void clear_flag_subtract();
	void clear_flag_half_carry();
	void clear_flag_carry();





	/*
		Opcode methods
	*/

	void LD (CPU::REGISTERS reg1, CPU::REGISTERS reg2);					// LD X, Y
	void LD(CPU::REGISTERS reg, int8_t val, bool indirect);				// LD X, d8 and LD (X), d8 when indirect == true
	void LD_reg_into_memory(CPU::REGISTERS reg1, CPU::REGISTERS reg2);	// LD (C), A
	void LD(CPU::REGISTERS reg, std::int16_t val);						// LD XY, d16
	void LD(std::int16_t addr, std::int8_t val);						// LD (a16), val
	void LD(std::int16_t addr, std::int16_t val);						// LD (a16), SP
	void LD_INDIRECT_A16(CPU::REGISTERS reg, std::int16_t addr);		// LD A, (a16)
	void LDH(CPU::REGISTERS reg, std::int8_t val);						// LDH A, (a8)
	void LDH_INDIRECT(std::int16_t addr, std::int8_t val);				// LDH (a8), A
	void LD_HL_SPPLUSR8(CPU::REGISTERS reg, std::int8_t r8);			// LD HL, SP+r8

	void ADD(CPU::REGISTERS reg1, std::int8_t d8, bool indirect);		// ADD A, d8
	void ADD_HL(CPU::REGISTERS reg);									// ADD HL, XY
	void ADD_SP_R8(CPU::REGISTERS reg, std::int8_t r8);					// ADD SP, r8
	void ADC(CPU::REGISTERS reg1, std::int8_t d8, bool indirect);		// ADC A, d8
	void SUB(std::int8_t d8, bool indirect);							// SUB X
	void SBC(std::int8_t d8, bool indirect);							// SBC A, X
	void AND(std::int8_t d8, bool indirect);							// AND X	AND (HL) when indirect == true
	void XOR(std::int8_t d8, bool indirect);							// XOR X	XOR (HL) when indirect == true
	void OR (std::int8_t d8, bool indirect);							// OR X		OR (HL) when indirect == true
	void CP (std::int8_t d8, bool indirect);							// Compare	CP X		CP (HL) when indirect == true

	void INC(CPU::REGISTERS reg, bool indirect);						// Increment	INC X	INC (HL) when indirect == true
	void DEC(CPU::REGISTERS reg, bool indirect);						// Decrement	DEC X	DEC (HL) when indirect == true




	void JP(CPU::FLAGTYPES, std::int16_t addr);							// JP a16	JP [NZ, NC, Z, C], a16
	void JP_INDIRECT(std::int16_t addr);								// JP (HL)
		
	void JR(CPU::FLAGTYPES, std::int8_t val);							// JR r8	JR [NZ, NC, Z, C], r8

	void RET(CPU::FLAGTYPES);											// RET		RET [NZ, NC, Z, C]
	void RETI();														// RETI

	void RST(std::int8_t instruc);										// RST [00H, 10H, 20H, 30H, 08H, 18H, 28H, 38H]
	void CALL(CPU::FLAGTYPES, std::int16_t a16);						// CALL a16		CALL [NZ, NC, Z, C], a16

	void DAA();															// DAA (Decimal Adjust Accumulator)
	void SCF();															// SCF (Set carry flag)
	void CCF();															// CCF (Complement carry flag)
	void CPL();															// CPL (Complement A)

	void enable_interrupts();											// EI
	void disable_interrupts();											// DI

	void HALT();
	void NOP();
	void STOP();

	void RLCA();
	void RLA();
	void RRCA();
	void RRA();

	void POP(CPU::REGISTERS reg);
	void PUSH(CPU::REGISTERS reg);

	int8_t getByteFromMemory(CPU::REGISTERS reg);
	int8_t getByteFromMemory(std::int16_t addr);
	void setByteToMemory(int16_t addr, int8_t val);


	int16_t getNextTwoBytes();



	/*
		Prefix CB Opcodes
	*/

	void handle_CB(std::int8_t instruc);

	void RLC(CPU::REGISTERS reg);		// RLC [B, C, D, E, H, L, (HL), A]
	void RRC(CPU::REGISTERS reg);		// RRC [B, C, D, E, H, L, (HL), A]
	void RL(CPU::REGISTERS reg);		// RL [B, C, D, E, H, L, (HL), A]
	void RR(CPU::REGISTERS reg);		// RR [B, C, D, E, H, L, (HL), A]

	void BIT(std::int8_t bit, CPU::REGISTERS reg);	// BIT [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]



};

#endif