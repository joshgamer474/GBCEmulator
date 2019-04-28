#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#ifndef GBCEMULATOR_H
#define GBCEMULATOR_H

#ifdef _WIN32
#include <windows.h>
#include "stdafx.h"
#endif // _WIN32

#include <chrono>
#include <fstream>
#include <functional>
#include <iterator>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <experimental/filesystem>

#include "APU.h"
#include "CPU.h"
#include "Memory.h"
#include "Joypad.h"
#include "MBC.h"
#include "GPU.h"
#include "CartridgeReader.h"
#include "Debug.h"

extern "C" {
#include <SDL.h>
}

#define USE_AUDIO_TIMING

class APU;
class CPU;
class Memory;
class Joypad;
class MBC;
class GPU;
class CartridgeReader;

class GBCEmulator
{
public:
    GBCEmulator(const std::string romName, const std::string logName = "log.txt", bool debugMode = false);
    virtual ~GBCEmulator();
    GBCEmulator& operator=(const GBCEmulator& rhs);

    void set_logging_level(spdlog::level::level_enum l);
    void run();
    void runNextInstruction();
    void runTo(uint16_t pc);
    void stop();
    void setStopRunning(bool val);
    bool frame_is_ready();
    SDL_Color * get_frame();
    std::shared_ptr<APU> get_APU();
    std::shared_ptr<CPU> get_CPU();
    std::shared_ptr<GPU> get_GPU();
    std::shared_ptr<Joypad> get_Joypad();
    std::vector<uint8_t> get_memory_map();
    std::vector<uint8_t> get_partial_memory_map(uint16_t start_pos, uint16_t end_pos);
    void set_joypad_button(Joypad::BUTTON button);
    void release_joypad_button(Joypad::BUTTON button);
    void setTimePerFrame(double d);
    std::string getROMName() const;

    void setFrameUpdateMethod(std::function<void(SDL_Color * /* frame */)> function);
    static uint64_t calculateFrameHash(SDL_Color* frame);
    void saveFrameToPNG(std::experimental::filesystem::path filepath);

    bool ranInstruction;
    bool debugMode;
    bool isInitialized;
    bool runWithoutSleep;
    std::chrono::microseconds frameProcessingTimeMicro;   // This is updated right before calling frameIsUpdatedFunction()
                                                // so it is easily accessible to all wrappers
    std::chrono::microseconds frameShowTimeMicro;

private:
    void read_rom(std::string filename);
    void init_memory();
    void init_gpu();
    void init_logging(std::string logName);
    void waitToStartNextFrame();
    std::chrono::duration<double> getCurrentTime();
 
    std::shared_ptr<APU> apu;
    std::shared_ptr<CPU> cpu;
    std::shared_ptr<CartridgeReader> cartridgeReader;
    std::shared_ptr<MBC> mbc;
    std::shared_ptr<GPU> gpu;
    std::shared_ptr<Memory> memory;
    std::shared_ptr<Joypad> joypad;

    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> loggerSink;
    std::shared_ptr<spdlog::logger> logger;
    std::uint16_t logCounter;

    bool stopRunning;
    std::string logFileBaseName;
    std::string filenameNoExtension;

    uint64_t ticksPerFrame;
    uint32_t ticksAccumulated;
    std::chrono::duration<double> frameTimeStart;
    std::chrono::duration<double> timePerFrame;

    std::function<void(SDL_Color * /* frame */)> frameIsUpdatedFunction;
};

#endif