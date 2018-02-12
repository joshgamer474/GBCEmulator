// GBCEmulator.cpp : Defines the entry point for the console application.
//
#include <windows.h>

#include "stdafx.h"
#include <fstream>
#include <iterator>
#include <vector>

#include "CPU.h"
#include "Memory.h"
#include "Joypad.h"
#include "MBC.h"
#include "GPU.h"
#include "CartridgeReader.h"
#include "Debug.h"

void read_rom(std::string filename);
void init_memory();
void init_gpu();
void run();

CPU *cpu;
Memory *memory;
MBC *mbc;
GPU *gpu;
Joypad *joypad;


SDL_Window *window;
SDL_Surface *screenSurface;
SDL_Renderer *renderer;
TTF_Font *font;
SDL_Color message_color;
SDL_Surface *surfaceMessage;
SDL_Texture *surfaceTexture;
SDL_Rect surfaceMessageRect;

#undef main
int main(int *argc, char **argv)
{

	/*
	SDL stuff
	*/
	SDL_Init(SDL_INIT_VIDEO);
#ifdef DEBUG
	window = SDL_CreateWindow("GBCEmulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4, SDL_WINDOW_SHOWN);
#else
	window = SDL_CreateWindow("GBCEmulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_PIXEL_W, SCREEN_PIXEL_H, SDL_WINDOW_SHOWN);
#endif
	renderer = SDL_CreateRenderer(window, -1, 0);
	screenSurface = SDL_GetWindowSurface(window);
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);
	SDL_Event event;
	bool stop = false;

	// Init font stuff
	int ret = TTF_Init();
	font = TTF_OpenFont("C:\\Windows\\Fonts\\REFSAN.TTF", 12);
	message_color = { 0, 255, 0 };
	surfaceMessageRect = { 0, SCREEN_PIXEL_H * 2, SCREEN_PIXEL_W, SCREEN_PIXEL_H };


	/*
		Gameboy stuff
	*/
	// Init GB
	cpu = new CPU();
	memory = new Memory();
	mbc;
	//gpu = new GPU();
	gpu = new GPU(renderer);
	joypad = new Joypad();


	// Read in rom
	//read_rom("pokemon-blue.gb");
	//read_rom("Tetris.gb");
	read_rom("Dr. Mario.gb");
	//read_rom("blarggtests/cpu_instrs/cpu_instrs.gb");
	//read_rom("blarggtests/cpu_instrs/individual/06-ld r,r.gb");

	// Initialize memory objects, link GB components together
	init_memory();
	init_gpu();



	while (!stop)
	{
		run();
		gpu->run();
	}

	TTF_CloseFont(font);
	SDL_DestroyWindow(window);
	SDL_Quit();

    return 0;
}


void read_rom(std::string filename)
{
	memory->cartridgeReader->setRomDestination(filename);
	memory->cartridgeReader->readRom();
}


void init_memory()
{
	// Setup Memory Bank Controller
	mbc = new MBC(memory->cartridgeReader->cartridgeType.mbc);

	// Set GB object pointers, move game cartridge into ROM banks
	memory->mbc = mbc;
	memory->initROMBanks();
	memory->joypad = joypad;
	joypad->memory = memory;
	gpu->cpu = cpu;
	cpu->memory = memory;
}

void init_gpu()
{
	memory->gpu = gpu;
	gpu->joypad = joypad;
	gpu->memory = memory;
}


void run()
{
	// Run bios
	std::uint16_t pc = 0;
	//if (cpu->get_register_16(CPU::REGISTERS::PC) <= 256)
	if (cpu->get_register_16(CPU::REGISTERS::PC) >= 0)
	{
		pc = cpu->get_register_16(CPU::REGISTERS::PC);
#ifdef ENABLE_DEBUG_PRINT
		printf("PC = %#04x\n", pc);
#endif
		cpu->runInstruction(cpu->getInstruction());
#ifdef ENABLE_DEBUG_PRINT
		cpu->printRegisters();
#endif
	}
}