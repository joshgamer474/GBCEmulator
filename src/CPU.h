#ifndef CPU_H
#define CPU_H

#include <cstdint>
#include <vector>
#include <string>
#include <spdlog/spdlog.h>

#define CLOCK_SPEED         4 * 1024 * 1024				// Hz/CPU cycles
#define CLOCK_SPEED_GBC_MAX 8 * 1024 * 1024

//#define ENABLE_DEBUG_PRINT

class CPU_Opcodes;
class Memory;

class CPU
{
	int NUM_OF_REGISTERS = 6;

private:

    void checkJoypadForInterrupt();

	// Registers
	std::vector<std::uint16_t> registers;
	std::uint8_t instruction;

public:

	CPU(std::shared_ptr<spdlog::logger> logger,
        std::shared_ptr<Memory> memory);
	~CPU();

    uint8_t runNextInstruction();

	std::shared_ptr<Memory> memory;

	bool interrupt_master_enable;
	bool interrupts_enabled;
	bool is_halted;
    bool halt_do_not_increment_pc;
	bool is_stopped;
	bool startLogging = false;
	std::shared_ptr<spdlog::logger> logger;


	/*
		Debug stuff
	*/
	void printRegisters();


	/*
		Instruction stuff
	*/
	uint8_t getInstruction(uint8_t & ticks_ran);
	uint8_t runInstruction(std::uint8_t);

	// Opcodes handling
	CPU_Opcodes *operationHandling;


	/*
		Interrupt stuff
	*/
    uint8_t handleInterrupt();
	unsigned char interrupt_table[5] = {0x40, 0x48, 0x50, 0x58, 0x60};

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

	
	std::vector<std::string> REGISTERS_STR = {"BC", "DE", "HL", "AF", "SP", "PC", "B", "C", "D", "E", "H", "L", "A", "F"};
	std::vector<REGISTERS> reg_list = { CPU::B, CPU::C, CPU::D, CPU::E, CPU::H, CPU::L, CPU::HL, CPU::A };

	enum class FLAGTYPES
	{
		NZ,		// Not zero
		NC,		// Not carry
		Z,		// Zero
		C,		// Carry
		NONE
	};
	std::vector<std::string> FLAGTYPES_STR = { "NZ", "NC", "Z", "C", "NONE" };

	std::string getRegisterString(REGISTERS);

	// Register getter
	std::uint8_t get_register_8(REGISTERS reg);
	std::uint16_t get_register_16(REGISTERS reg);


	// Register setters
	void set_register(REGISTERS reg, std::uint16_t);
	void set_register(REGISTERS reg, std::uint8_t);
	void set_register(REGISTERS reg, std::int8_t);


	/*
	Flag methods

	8-bit Register

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

	uint8_t LD (CPU::REGISTERS reg1, CPU::REGISTERS reg2);					// LD X, Y
	uint8_t LD(CPU::REGISTERS reg, uint8_t val, bool indirect);				// LD X, d8 and LD (X), d8 when indirect == true
	uint8_t LD_reg_into_memory(CPU::REGISTERS reg1, CPU::REGISTERS reg2);	// LD (C), A
	uint8_t LD(CPU::REGISTERS reg, std::uint16_t val);						// LD XY, d16
	uint8_t LD(std::uint16_t addr, std::uint8_t val);						// LD (a16), val
	uint8_t LD(std::uint16_t addr, std::uint16_t val);						// LD (a16), SP
	uint8_t LD_INDIRECT_A16(CPU::REGISTERS reg, std::uint16_t addr);		// LD A, (a16)
	uint8_t LDH(CPU::REGISTERS reg, std::uint8_t val);						// LDH A, (a8)
	uint8_t LDH_INDIRECT(std::uint16_t addr, std::uint8_t val);				// LDH (a8), A
	uint8_t LD_HL_SPPLUSR8(CPU::REGISTERS reg, std::int8_t r8);			    // LD HL, SP+r8

	uint8_t ADD(CPU::REGISTERS reg1, std::uint8_t d8, bool indirect);		// ADD A, d8
	uint8_t ADD_HL(CPU::REGISTERS reg);									    // ADD HL, XY
	uint8_t ADD_SP_R8(CPU::REGISTERS reg, std::int8_t r8);					// ADD SP, r8
	uint8_t ADC(CPU::REGISTERS reg1, std::uint8_t d8, bool indirect);		// ADC A, d8
	uint8_t SUB(std::uint8_t d8, bool indirect);							// SUB X
	uint8_t SBC(std::uint8_t d8, bool indirect);							// SBC A, X
	uint8_t AND(std::uint8_t d8, bool indirect);							// AND X	AND (HL) when indirect == true
	uint8_t XOR(std::uint8_t d8, bool indirect);							// XOR X	XOR (HL) when indirect == true
	uint8_t OR (std::uint8_t d8, bool indirect);							// OR X		OR (HL) when indirect == true
	uint8_t CP (std::uint8_t d8, bool indirect);							// Compare	CP X		CP (HL) when indirect == true

	uint8_t INC(CPU::REGISTERS reg, bool indirect);						// Increment	INC X	INC (HL) when indirect == true
	uint8_t DEC(CPU::REGISTERS reg, bool indirect);						// Decrement	DEC X	DEC (HL) when indirect == true

	uint8_t JP(CPU::FLAGTYPES, std::uint16_t addr);						// JP a16	JP [NZ, NC, Z, C], a16
	uint8_t JP_INDIRECT(std::uint16_t addr);								// JP (HL)
		
	uint8_t JR(CPU::FLAGTYPES, std::int8_t val);							// JR r8	JR [NZ, NC, Z, C], r8

	uint8_t RET(CPU::FLAGTYPES);											// RET		RET [NZ, NC, Z, C]
	uint8_t RETI();														// RETI

	uint8_t RST(std::uint8_t instruc);										// RST [00H, 10H, 20H, 30H, 08H, 18H, 28H, 38H]
	uint8_t CALL(CPU::FLAGTYPES, std::uint16_t a16);						// CALL a16		CALL [NZ, NC, Z, C], a16

	uint8_t DAA();															// DAA (Decimal Adjust Accumulator)
	uint8_t SCF();															// SCF (Set carry flag)
	uint8_t CCF();															// CCF (Complement carry flag)
	uint8_t CPL();															// CPL (Complement A)

	uint8_t enable_interrupts();											// EI
	uint8_t disable_interrupts();											// DI

	uint8_t HALT();
	uint8_t NOP();
	uint8_t STOP();

	uint8_t RLCA();
	uint8_t RLA();
	uint8_t RRCA();
	uint8_t RRA();

	uint8_t POP(CPU::REGISTERS reg);
	uint8_t PUSH(CPU::REGISTERS reg);

    uint8_t peekNextByte();
    int8_t peekNextByteSigned();
	uint16_t getNextTwoBytes();
    uint16_t peekNextTwoBytes();

	/*
		Prefix CB Opcodes
	*/

	uint8_t handle_CB(std::uint8_t instruc);

	uint8_t RLC(CPU::REGISTERS reg);		// RLC [B, C, D, E, H, L, (HL), A]
	uint8_t RRC(CPU::REGISTERS reg);		// RRC [B, C, D, E, H, L, (HL), A]
	uint8_t RL(CPU::REGISTERS reg);		// RL [B, C, D, E, H, L, (HL), A]
	uint8_t RR(CPU::REGISTERS reg);		// RR [B, C, D, E, H, L, (HL), A]

	uint8_t SLA(CPU::REGISTERS reg);		// SLA [B, C, D, E, H, L, (HL), A]
	uint8_t SRA(CPU::REGISTERS reg);		// SRA [B, C, D, E, H, L, (HL), A]
	uint8_t SWAP(CPU::REGISTERS reg);		// SWAP [B, C, D, E, H, L, (HL), A]
	uint8_t SRL(CPU::REGISTERS reg);		// SRL [B, C, D, E, H, L, (HL), A]

	uint8_t BIT(std::uint8_t bit, CPU::REGISTERS reg);	// BIT [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
	uint8_t SET(std::uint8_t bit, CPU::REGISTERS reg);	// RES [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]
	uint8_t RES(std::uint8_t bit, CPU::REGISTERS reg);	// RES [0, 1, 2, 3, 4, 5, 6, 7], [B, C, D, E, H, L, (HL), A]



	/*
		Reading and Writing from/to memory
	*/

	uint8_t getByteFromMemory(CPU::REGISTERS reg);
	uint8_t getByteFromMemory(std::uint16_t addr);
	void setByteToMemory(uint16_t addr, uint8_t val);

    std::string getOpcodeString(uint8_t opcode);
    std::string getOpcodeStringCB(uint8_t opcode);

    uint8_t getInstructionSize(uint8_t opcode);
    
    template <typename T>
    std::string numToHex(T number);
};

#endif