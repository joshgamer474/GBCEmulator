#ifndef GBCEMULATOR_H
#define GBCEMULATOR_H

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
#include <spdlog/spdlog.h>

class GBCEmulator {

public:

    GBCEmulator(const std::string romName, const std::string logName="log.txt");
    ~GBCEmulator();

    void set_logging_level(spdlog::level::level_enum l);
    void run();
    void stop();

private:

    void read_rom(std::string filename);
    void init_SDL();
    void init_memory();
    void init_gpu();
    void init_logging(std::string logName);

    std::shared_ptr<CPU> cpu;
    std::shared_ptr<CartridgeReader> cartridgeReader;
    std::shared_ptr<MBC> mbc;
    std::shared_ptr<GPU> gpu;

    SDL_Window *window;
    SDL_Surface *screenSurface;
    SDL_Renderer *renderer;

    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> logger;
    std::uint16_t logCounter;

    bool stopRunning;
    std::string logFileBaseName;

};

#endif