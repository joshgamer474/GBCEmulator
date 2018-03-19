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
#include "spdlog/spdlog.h"

void read_rom(std::string filename);
void init_memory();
void init_gpu();
void run();
void init_logging();
void set_logging_level(spdlog::level::level_enum l);

CPU *cpu;
Memory *memory;
CartridgeReader *cartridgeReader;
MBC *mbc;
GPU *gpu;
Joypad *joypad;

SDL_Window *window;
SDL_Surface *screenSurface;
SDL_Renderer *renderer;

std::shared_ptr<spdlog::sinks::rotating_file_sink_st> logger;
std::uint16_t logCounter;

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


	/*
		Gameboy stuff
	*/
	// Init GB
	cpu = new CPU();
	memory = new Memory();
	cartridgeReader = new CartridgeReader();
	mbc = new MBC();
	gpu = new GPU(renderer);
	joypad = new Joypad();

	logger = std::make_shared<spdlog::sinks::rotating_file_sink_st>("log.txt", 1024 * 1024 * 500, 20);
	init_logging();
	set_logging_level(spdlog::level::trace);
	gpu->logger->set_level(spdlog::level::warn);
	cpu->logger->set_level(spdlog::level::warn);
	logCounter = 0;

	// Read in rom
	//read_rom("testroms\\pokemon-blue.gb");
	//read_rom("testroms\\Tetris.gb");
	//read_rom("testroms\\Dr. Mario.gb");
	//read_rom("testroms/Super Mario Land (World).gb");
	read_rom("blarggtests/cpu_instrs/cpu_instrs.gb");
	//read_rom("blarggtests/cpu_instrs/individual/01-special.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/02-interrupts.gb");			// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/03-op sp,hl.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/04-op r,imm.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/05-op rp.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/06-ld r,r.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb");	// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/08-misc instrs.gb");			// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/09-op r,r.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/10-bit ops.gb");				// PASSED
	//read_rom("blarggtests/cpu_instrs/individual/11-op a,(hl).gb");			// PASSED


	// Initialize memory objects, link GB components together
	init_memory();
	init_gpu();


	while (!stop)
	{
		run();
		memory->updateTimer(cpu->ticks, CLOCK_SPEED);
		gpu->run();

		if (logCounter % 100 == 0)
		{
			logCounter = 0;
			logger->flush();
		}
		logCounter++;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

    return 0;
}


void read_rom(std::string filename)
{
	cartridgeReader->setRomDestination(filename);
	cartridgeReader->readRom();
}


void init_memory()
{
	// Setup Memory Bank Controller
	mbc->MBC_init(cartridgeReader->cartridgeType.mbc);

	// Set GB object pointers, move game cartridge into ROM banks
	memory->cartridgeReader = cartridgeReader;
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
	/*if (cartridgeReader->getColorGBFlag())
		gpu->init_color_gb();*/
	gpu->init_color_gb();
}


void run()
{
	// Run bios
	std::uint16_t pc = 0;
	//if (cpu->get_register_16(CPU::REGISTERS::PC) <= 256)
	if (cpu->get_register_16(CPU::REGISTERS::PC) >= 0)
	{
		pc = cpu->get_register_16(CPU::REGISTERS::PC);
		cpu->runInstruction(cpu->getInstruction());
#ifdef ENABLE_DEBUG_PRINT
		cpu->printRegisters();
#endif
	}
}

void init_logging()
{
	// Create loggers
	cpu->logger = std::make_shared<spdlog::logger>("CPU", logger);
	memory->logger = std::make_shared<spdlog::logger>("Memory", logger);
	mbc->logger = std::make_shared<spdlog::logger>("MBC", logger);
	gpu->logger = std::make_shared<spdlog::logger>("GPU", logger);
	cartridgeReader->logger = std::make_shared<spdlog::logger>("CartridgeReader", logger);
	joypad->logger = std::make_shared<spdlog::logger>("Joypad", logger);
}

void set_logging_level(spdlog::level::level_enum l)
{
	cpu->logger->set_level(l);
	memory->logger->set_level(l);
	mbc->logger->set_level(l);
	gpu->logger->set_level(l);
	cartridgeReader->logger->set_level(l);
	joypad->logger->set_level(l);
}