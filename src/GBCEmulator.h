#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#ifndef GBCEMULATOR_H
#define GBCEMULATOR_H

#include <windows.h>
#include "stdafx.h"
#include <chrono>
#include <fstream>
#include <iterator>
#include <vector>
#include <functional>

#include "CPU.h"
#include "Memory.h"
#include "Joypad.h"
#include "MBC.h"
#include "GPU.h"
#include "CartridgeReader.h"
#include "Debug.h"
#include <spdlog/spdlog.h>
extern "C" {
#include <SDL.h>
}


class GBCEmulator
{
public:
    GBCEmulator(const std::string romName, const std::string logName="log.txt", bool debugMode=false);
    virtual ~GBCEmulator();

    void set_logging_level(spdlog::level::level_enum l);
    void run();
    void runNextInstruction();
    void runTo(uint16_t pc);
    void stop();
    void setStopRunning(bool val);
    bool frame_is_ready();
    SDL_Color * get_frame();
    std::shared_ptr<CPU> get_CPU();
    std::shared_ptr<GPU> get_GPU();
    std::vector<uint8_t> get_memory_map();
    std::vector<uint8_t> get_partial_memory_map(uint16_t start_pos, uint16_t end_pos);
    void set_joypad_button(Joypad::BUTTON button);
    void release_joypad_button(Joypad::BUTTON button);
    void setTimePerFrame(double d);

    void setFrameUpdateMethod(std::function<void(void)> function);

    bool ranInstruction;
    bool debugMode;
    bool isInitialized;
    SDL_Window *window;

private:
    void read_rom(std::string filename);
    void init_SDL();
    void init_memory();
    void init_gpu();
    void init_logging(std::string logName);
    void waitToStartNextFrame();
    std::chrono::duration<double> getCurrentTime();
 
    std::shared_ptr<CPU> cpu;
    std::shared_ptr<CartridgeReader> cartridgeReader;
    std::shared_ptr<MBC> mbc;
    std::shared_ptr<GPU> gpu;

    SDL_GLContext glContext;
    SDL_Surface *screenSurface;
    SDL_Renderer *renderer;

    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> logger;
    std::uint16_t logCounter;

    bool stopRunning;
    std::string logFileBaseName;

    uint64_t ticksPerFrame;
    uint64_t ticksRan;
    std::chrono::duration<double> frameStartTime;
    std::chrono::duration<double> timePerFrame;

    std::function<void(void)> frameIsUpdatedFunction;
};

#endif