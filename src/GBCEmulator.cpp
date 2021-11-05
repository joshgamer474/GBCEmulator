#include "GBCEmulator.h"
#include <libpng16/png.h>
#include <thread>

GBCEmulator::GBCEmulator(const std::string romName, const std::string logName,
    const std::string biosPath, bool debugMode, const bool force_cgb_mode)
    :   stopRunning(false),
        debugMode(false),
        ranInstruction(false),
        runWithoutSleep(false),
        logFileBaseName(logName),
        frameIsUpdatedFunction(nullptr)
{
    init_logging(logName);

    cartridgeReader = std::make_shared<CartridgeReader>(std::make_shared<spdlog::logger>("CartridgeReader", loggerSink), force_cgb_mode);
    apu     = std::make_shared<APU>(loggerSink, std::make_shared<spdlog::logger>("APU", loggerSink));
    joypad  = std::make_shared<Joypad>(std::make_shared<spdlog::logger>("Joypad", loggerSink));
    serial_transfer = std::make_shared<SerialTransfer>(std::make_shared<spdlog::logger>("SerialTransfer", loggerSink));

    if (!biosPath.empty())
    {   // Read BIOS if supplied
        cartridgeReader->setBiosDestination(biosPath);
        cartridgeReader->readBios();
    }

    read_rom(romName);

    // Initialize GPU and memory objects, link GB components together
    init_gpu(force_cgb_mode);
    init_memory(force_cgb_mode);

    // Initialize CPU
    cpu = std::make_shared<CPU>(std::make_shared<spdlog::logger>("CPU", loggerSink),
        memory,
        cartridgeReader->has_bios);

    // Read in game save (.sav) and RTC clock (.rtc) if available
    filenameNoExtension = romName.substr(0, romName.find_last_of("."));
    mbc->loadSaveIntoRAM(filenameNoExtension + ".sav");
    mbc->loadRTCIntoRAM(filenameNoExtension + ".rtc");

    // Calculate number of CPU cycles that can tick in one frame time
    ticksPerFrame = CLOCK_SPEED / SCREEN_FRAMERATE; // cycles per frame
    ticksAccumulated = 0;
    setTimePerFrame(1.0 / SCREEN_FRAMERATE);

    // Set log levels
    set_logging_level(spdlog::level::err);
    //cpu->logger->set_level(spdlog::level::trace);
/*
    gpu->logger->set_level(spdlog::level::info);
    cpu->logger->set_level(spdlog::level::warn);
    memory->logger->set_level(spdlog::level::info);
    apu->logger->set_level(spdlog::level::trace);
    apu->setChannelLogLevel(spdlog::level::debug);
    joypad->logger->set_level(spdlog::level::warn);
    logger->set_level(spdlog::level::info);*/
    logCounter = 0;
}

GBCEmulator::~GBCEmulator()
{
    logger->info("Destructing");

    // Try to write out .sav file
    mbc->saveRAMToFile(filenameNoExtension + ".sav");

    // Try to write out .rtc file
    mbc->latchCurrTimeToRTC();
    mbc->saveRTCToFile(filenameNoExtension + ".rtc");

    // Write out last frame hash
    uint64_t lastFrameHash = calculateFrameHash(gpu->curr_frame);
    logger->info("Last frame hash: {}", lastFrameHash);

    cpu->memory->reset();
    cpu.reset();
    cartridgeReader.reset();
    mbc.reset();
    gpu.reset();
    apu.reset();
    loggerSink.reset();
}

GBCEmulator& GBCEmulator::operator=(const GBCEmulator& rhs)
{   // Copy from rhs
    *this->apu.get()    = *rhs.apu.get();
    *this->cpu.get()    = *rhs.cpu.get();
    *this->gpu.get()    = *rhs.gpu.get();
    *this->memory.get() = *rhs.memory.get();
    *this->joypad.get() = *rhs.joypad.get();
    *this->mbc.get()    = *rhs.mbc.get();
    *this->cartridgeReader.get() = *rhs.cartridgeReader.get();

    ranInstruction  = rhs.ranInstruction;
    debugMode       = rhs.debugMode;
    frameProcessingTimeMicro = rhs.frameProcessingTimeMicro;
    frameShowTimeMicro = rhs.frameShowTimeMicro;
    stopRunning     = rhs.stopRunning;
    logFileBaseName = rhs.logFileBaseName;
    filenameNoExtension = rhs.filenameNoExtension;
    ticksPerFrame   = rhs.ticksPerFrame;
    ticksAccumulated = rhs.ticksAccumulated;
    frameTimeStart  = rhs.frameTimeStart;
    timePerFrame    = rhs.timePerFrame;
    frameIsUpdatedFunction = rhs.frameIsUpdatedFunction;

    return *this;
}

void GBCEmulator::read_rom(std::string filename)
{
    cartridgeReader->setRomDestination(filename);
    bool successful = cartridgeReader->readRom();

    if (!successful)
    {
        throw std::runtime_error("Could not find file " + filename);
    }
}

void GBCEmulator::init_memory(const bool force_cgb_mode)
{
    // Setup Memory Bank Controller
    mbc = std::make_shared<MBC>(cartridgeReader->getMBCNum(),
        cartridgeReader->num_ROM_banks,
        cartridgeReader->num_RAM_banks,
        std::make_shared<spdlog::logger>("MBC", loggerSink));

    // Setup Memory
    memory = std::make_shared<Memory>(
        std::make_shared<spdlog::logger>("Memory", loggerSink),
        cartridgeReader,
        mbc,
        gpu,
        joypad,
        apu,
        serial_transfer,
        force_cgb_mode);

    gpu->memory = memory;
}

void GBCEmulator::init_gpu(const bool force_cgb_mode)
{
    gpu = std::make_shared<GPU>(std::make_shared<spdlog::logger>("GPU", loggerSink));

    if (cartridgeReader->isColorGB() || force_cgb_mode)
    {
        gpu->init_color_gb();
    }
}

void GBCEmulator::run()
{
    stopRunning = false;

    // Update frameTimeStart to current time
    frameTimeStart = getCurrentTime();

    // Run emulator loop
    while (!stopRunning && !debugMode)
    {
        runNextInstruction();
    }
}

void GBCEmulator::runNextInstruction()
{
    uint8_t ticksRan = cpu->runNextInstruction();

    // Check if Gameboy is in double speed mode
    if (memory->cgb_speed_mode & BIT7)
    {   // In double speed mode, divide tickDiff by 2 to simulate
        // running the CPU at double speed
        ticksRan >>= 1;
    }

    apu->run(ticksRan);
    gpu->run(ticksRan);

	// Update timer
	if (memory->cgb_speed_mode & BIT7)
	{
		memory->updateTimer(ticksRan<<1, CLOCK_SPEED_GBC_MAX);
	}
	else
	{
		memory->updateTimer(ticksRan, CLOCK_SPEED);
	}

#ifdef USE_AUDIO_TIMING
    // Sync video to audio
    if (gpu->frame_is_ready)
    {
        // Display current frame
        if (frameIsUpdatedFunction)
        {
            frameIsUpdatedFunction(gpu->getFrame());
        }
        gpu->frame_is_ready = false;

        apu->logger->trace("Number of samples made during frame: {0:d}", apu->samplesPerFrame);

        // Write out accumulated audio samples to audio device
        apu->writeSamplesOutAsync(apu->audio_device_id);

        // Calculate frame processing time for debug purposes
        auto currTime = getCurrentTime();
        frameProcessingTimeMicro = std::chrono::duration_cast<std::chrono::microseconds>(currTime - frameTimeStart);

        if (runWithoutSleep == false)
        {
            // Let the APU sleep the emulator
            apu->sleepUntilBufferIsEmpty(frameTimeStart);
        }

        // Calculate frame show time for debug purposes
        currTime = getCurrentTime();
        frameShowTimeMicro = std::chrono::duration_cast<std::chrono::microseconds>(currTime - frameTimeStart);
        logger->debug("Frame Show time (milli): {}, Frame Processing time (milli): {}",
            std::to_string(frameShowTimeMicro.count() / 1000.0),
            std::to_string(frameProcessingTimeMicro.count() / 1000.0));

        // Update frameTimeStart to current time
        frameTimeStart = currTime;
    }
#else // use CPU tick timing
    // Note: video will not be in-sync with audio
    ticksAccumulated += ticksRan;
    if (ticksAccumulated >= ticksPerFrame
        && gpu->frame_is_ready)
    {
        ticksAccumulated -= ticksPerFrame;

        //if (gpu->frame_is_ready)
        {
            frameIsUpdatedFunction(gpu->getFrame());
            gpu->frame_is_ready = false;
        }

        apu->logger->info("Number of samples made during frame: {0:d}", apu->samplesPerFrame);

        // Write out accumulated audio samples to audio device
        apu->writeSamplesOutAsync(apu->audio_device_id);

        // Sleep until next burst of ticks_accumulated is ready to be ran
        waitToStartNextFrame();

        // Update frameTimeStart to current time
        frameTimeStart = getCurrentTime();
    }
#endif // USE_AUDIO_TIMING

    if (memory->cgb_perform_speed_switch)
    {   // Perform CPU double speed mode
        memory->cgb_perform_speed_switch = false;

        apu->initCGB();

        // Calculate number of CPU cycles that can tick in one frame's time
        ticksPerFrame = CLOCK_SPEED_GBC_MAX / SCREEN_FRAMERATE; // cycles per frame

        // Set double speed flag
        memory->cgb_speed_mode |= BIT7;
        memory->cgb_speed_mode &= 0xFE;    // Clear bit 0
    }

    if (logCounter % 1000 == 0)
    {
        logCounter = 0;
        loggerSink->flush();
    }
    logCounter++;

    ranInstruction = true;
}

void GBCEmulator::runTo(uint16_t pc)
{
    while (cpu->get_register_16(CPU::REGISTERS::PC) != pc && !stopRunning)
    {
        runNextInstruction();
    }
}

void GBCEmulator::stop()
{
    stopRunning = true;
}

void GBCEmulator::setStopRunning(bool val)
{
    stopRunning = val;
}

void GBCEmulator::init_logging(std::string logName)
{
    // Create loggerSink
    loggerSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logName, 1024 * 1024 * 500, 2);

    // Create logger for this class
    logger = std::make_shared<spdlog::logger>("GBCEmulator", loggerSink);
}

void GBCEmulator::set_logging_level(spdlog::level::level_enum l)
{
    cpu->logger->set_level(l);
    memory->logger->set_level(l);
    apu->logger->set_level(l);
    apu->setChannelLogLevel(l);
    joypad->logger->set_level(l);
    mbc->logger->set_level(l);
    gpu->logger->set_level(l);
    cartridgeReader->logger->set_level(l);
}

bool GBCEmulator::frame_is_ready() const
{
    return gpu->frame_is_ready;
}

std::shared_ptr<APU> GBCEmulator::get_APU()
{
    if (apu)
    {
        return apu;
    }
    return NULL;
}

std::shared_ptr<CPU> GBCEmulator::get_CPU()
{
    if (cpu)
    {
        return cpu;
    }
    return NULL;
}

std::shared_ptr<GPU> GBCEmulator::get_GPU()
{
    if (gpu)
    {
        return gpu;
    }
    return NULL;
}

std::shared_ptr<Joypad> GBCEmulator::get_Joypad()
{
    if (joypad)
    {
        return joypad;
    }
    return NULL;
}

std::vector<uint8_t> GBCEmulator::get_memory_map() const
{
    std::vector<uint8_t> memory_map;
    memory_map.reserve(0xFFFF);

    for (uint16_t i = 0; i < 0xFFFF; i++)
    {
        memory_map.push_back(memory->readByte(i));
    }

    return memory_map;
}

std::vector<uint8_t> GBCEmulator::get_partial_memory_map(uint16_t start_pos, uint16_t end_pos) const
{
    std::vector<uint8_t> partial_memory_map;
    partial_memory_map.reserve(end_pos - start_pos);

    for (uint16_t i = start_pos; i < end_pos; i++)
    {
        partial_memory_map.push_back(memory->readByte(i));
    }
    return partial_memory_map;
}

void GBCEmulator::set_joypad_button(Joypad::BUTTON button)
{
    if (joypad)
    {
        joypad->set_joypad_button(button);
    }
}

void GBCEmulator::release_joypad_button(Joypad::BUTTON button)
{
    if (joypad)
    {
        joypad->release_joypad_button(button);
    }
}

void GBCEmulator::waitToStartNextFrame() const
{
    // Calculate time elapsed since start of frame
    auto currTimeDouble = getCurrentTime();
    auto timeElapsedMilli = currTimeDouble - frameTimeStart;

    // Calculate amount of time to sleep until next frame
    auto timeToWaitMilli = timePerFrame - timeElapsedMilli;
    if (timeToWaitMilli.count() > 0)
    {   // Sleep until next frame needs to start rendering
        std::this_thread::sleep_for(timeToWaitMilli);
    }
}

std::chrono::duration<double> GBCEmulator::getCurrentTime() const
{
    return std::chrono::duration<double>(
        std::chrono::system_clock::now()
        .time_since_epoch());
}

void GBCEmulator::setTimePerFrame(double d)
{
    timePerFrame = std::chrono::duration<double>(d);
}

void GBCEmulator::setFrameUpdateMethod(std::function<void(std::array<SDL_Color, SCREEN_PIXEL_TOTAL> /* frame */)> function)
{
    frameIsUpdatedFunction = function;
}

std::array<SDL_Color, SCREEN_PIXEL_TOTAL> GBCEmulator::getFrame() const
{
    if (gpu)
    {
        return gpu->getFrame();
    }
    return std::array<SDL_Color, SCREEN_PIXEL_TOTAL>();
}

SDL_Color* GBCEmulator::getFrameRaw() const
{
    if (gpu)
    {
        return gpu->curr_frame;
    }
    return NULL;
}

std::string GBCEmulator::getROMName() const
{
    if (cartridgeReader)
    {
        return cartridgeReader->cartridgeFilename;
    }
    return "";
}

uint64_t GBCEmulator::calculateFrameHash(SDL_Color* frame)
{
    uint64_t hash = 0;
    uint32_t rgba = 0;

    for (int i = 0; i < SCREEN_PIXEL_H * SCREEN_PIXEL_W; i++)
    {
        rgba = frame[i].r;
        rgba <<= 8;
        rgba += frame[i].g;
        rgba <<= 8;
        rgba += frame[i].b;
        rgba <<= 8;
        rgba += frame[i].a;

        hash += rgba;
    }
    return hash;
}

uint64_t GBCEmulator::calculateFrameHash(const std::array<SDL_Color, SCREEN_PIXEL_TOTAL>& frame)
{
    uint64_t hash = 0;
    uint32_t rgba = 0;

    for (const auto& pixel : frame)
    {
        rgba = pixel.r;
        rgba <<= 8;
        rgba += pixel.g;
        rgba <<= 8;
        rgba += pixel.b;
        rgba <<= 8;
        rgba += pixel.a;

        hash += rgba;
    }
    return hash;
}

void GBCEmulator::saveFrameToPNG(std::filesystem::path filepath)
{
    // Open file
    FILE* fp = fopen(filepath.string().c_str(), "wb");
    if (!fp) return;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return;

    png_infop info = png_create_info_struct(png);
    if (!info) return;

    if (setjmp(png_jmpbuf(png))) return;

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format
    png_set_IHDR(
        png,
        info,
        SCREEN_PIXEL_W,
        SCREEN_PIXEL_H,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    // Allocate memory for one row (4 bytes per pixel - RGBA)
    png_bytep row = (png_bytep)malloc(4 * SCREEN_PIXEL_W * sizeof(png_byte));

    // Write image data
    int x, y;
    for (y = 0; y < SCREEN_PIXEL_H; y++)
    {
        for (x = 0; x < SCREEN_PIXEL_W; x++)
        {
            SDL_Color& pixel = gpu->curr_frame[((y * SCREEN_PIXEL_W) + x)];
            row[(x * 4) + 0] = pixel.r;
            row[(x * 4) + 1] = pixel.g;
            row[(x * 4) + 2] = pixel.b;
            row[(x * 4) + 3] = pixel.a;
        } 
        // Row is copied to row, write out row
        png_write_row(png, row);
    }
    // End write
    png_write_end(png, NULL);

    // Free row
    free(row);

    // Close file
    fclose(fp);
}

std::string GBCEmulator::getGameTitle() const
{
    if (cartridgeReader)
    {
        return cartridgeReader->getGameTitle();
    }
    return "";
}

void GBCEmulator::changeCGBPalette()
{
    if (gpu)
    {
        gpu->changeCGBPalette();
    }
}

bool GBCEmulator::isColorGB() const
{
    if (cartridgeReader)
    {
        return cartridgeReader->isColorGB();
    }
    return false;
}