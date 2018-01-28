// GBCEmulator.cpp : Defines the entry point for the console application.
//
#include <windows.h>

#include "stdafx.h"
#include "CPU.h"

#include "Memory.h"

#include <fstream>
#include <iterator>
#include <vector>

#include "MBC.h"


void read_rom(std::string filename);
void init_memory();
void run();

void glut_init();
void display();
void timer(int t);

CPU *cpu;
Memory *memory;
MBC *mbc;
GPU *gpu;

int main(int *argc, char **argv)
{

	cpu = new CPU();
	memory = new Memory();
	mbc;
	gpu = new GPU();


	// Read in rom
	read_rom("pokemon-blue.gb");

	// Initialize memory objects, link GB components together
	init_memory();


	/*
		OpenGL stuff
	*/
	char *myargv[1];
	int myargc = 1;
	myargv[0] = _strdup("MyApp");
	glutInit(&myargc, myargv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(SCREEN_PIXEL_W, SCREEN_PIXEL_H);
	glutInitWindowPosition(1200, 400);
	glutCreateWindow("GBCEmulator");

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

	glutDisplayFunc(display);
	glutTimerFunc(0, timer, 0);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glut_init();

	// Run
	glutMainLoop();

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
	memory->gpu = gpu;
	memory->initROMBanks();
	cpu->memory = memory;
}


void run()
{
	// Run bios
	std::uint16_t pc = 0;
	if (cpu->get_register_16(CPU::REGISTERS::PC) <= 256)
	{
		pc = cpu->get_register_16(CPU::REGISTERS::PC);
		printf("PC = %#04x\n", pc);
		cpu->runInstruction(cpu->getInstruction());
		cpu->printRegisters();
	}
}

void glut_init()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void display()
{
	run();
	gpu->display();
}

/// NOTE: This will have to be sorted out since opengl cannot have a timer fast enough for CPU cycles, will probably have to make GPU its own thread
void timer(int t)
{
	glutPostRedisplay();

	// Set FPS
	glutTimerFunc(1000 / 60, timer, 0);
}