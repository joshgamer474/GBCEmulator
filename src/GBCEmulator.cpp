#include "GBCEmulator.h"

GBCEmulator::GBCEmulator(const std::string romName, const std::string logName, bool debugMode)
    : cpu(std::make_shared<CPU>()),
    cartridgeReader(std::make_shared<CartridgeReader>()),
    mbc(std::make_shared<MBC>()),
    stopRunning(false),
    debugMode(false),
    ranInstruction(false),
    logFileBaseName(logName)
{
#ifdef SDL_DRAW
    init_SDL();
#endif

    gpu = std::make_shared<GPU>(renderer);

    init_logging(logName);
    set_logging_level(spdlog::level::trace);
    gpu->logger->set_level(spdlog::level::info);
    cpu->logger->set_level(spdlog::level::info);
    logCounter = 0;

    read_rom(romName);

    // Initialize memory objects, link GB components together
    init_memory();
    init_gpu();

    // Calculate number of CPU cycles that can run in one frame's time
    ticksPerFrame = CLOCK_SPEED / SCREEN_FRAMERATE; // cycles per frame
    ticksRan = 0;
    timePerFrame = std::chrono::duration<double>(1.0 / SCREEN_FRAMERATE);
}

GBCEmulator::~GBCEmulator()
{
#ifdef SDL_DRAW
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif

    cpu->memory->reset();
    cpu.reset();
    cartridgeReader.reset();
    mbc.reset();
    gpu.reset();
    logger.reset();
}

#ifdef SDL_DRAW
void GBCEmulator::init_SDL()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);

#ifdef DEBUG
    window = SDL_CreateWindow("GBCEmulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_PIXEL_W * 4, SCREEN_PIXEL_H * 4,
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
#else
    window = SDL_CreateWindow("GBCEmulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_PIXEL_W, SCREEN_PIXEL_H,
        SDL_WINDOW_SHOWN);
#endif

    glContext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    screenSurface = SDL_GetWindowSurface(window);
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
    SDL_UpdateWindowSurface(window);
}
#endif

void GBCEmulator::read_rom(std::string filename)
{
    cartridgeReader->setRomDestination(filename);
    bool successful = cartridgeReader->readRom();

    if (!successful)
    {
        throw std::runtime_error("Could not find file " + filename);
    }
}

void GBCEmulator::init_memory()
{
    // Setup Memory Bank Controller
    mbc->MBC_init(cartridgeReader->getMBCNum());

    // Set GB object pointers, move game cartridge into ROM banks
    cpu->memory->cartridgeReader = cartridgeReader;
    cpu->memory->mbc = mbc;
    cpu->memory->initROMBanks();
    gpu->cpu = cpu;
}

void GBCEmulator::init_gpu()
{
    cpu->memory->gpu = gpu;
    gpu->memory = cpu->memory;
    if (cartridgeReader->isColorGB())
    {
        gpu->init_color_gb();
    }
}

void GBCEmulator::run()
{
    stopRunning = false;

    while (!stopRunning && !debugMode)
    {
        runNextInstruction();
    }
}

void GBCEmulator::runNextInstruction()
{
    waitToRunInstruction();
    cpu->runNextInstruction();
    gpu->run();

    if (logCounter % 100 == 0)
    {
        logCounter = 0;
        logger->flush();
    }
    logCounter++;

    if (gpu->frame_is_ready)
    {
        frameIsUpdatedFunction();
        gpu->frame_is_ready = false;

        // Update frameStartTime to current time
        frameStartTime = getCurrentTime();
    }

    if (cpu->memory->cgb_perform_speed_switch)
    {   // Perform CPU double speed mode
        cpu->memory->cgb_perform_speed_switch = false;

        // Calculate number of CPU cycles that can run in one frame's time
        ticksPerFrame = CLOCK_SPEED_GBC_MAX / SCREEN_FRAMERATE; // cycles per frame

        // Set double speed flag
        cpu->memory->cgb_speed_mode |= BIT7;
    }

    ranInstruction = true;
}

void GBCEmulator::runTo(uint16_t pc)
{
    while (cpu->get_register_16(CPU::PC) != pc && !stopRunning)
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
    // Create logger
    logger = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logName, 1024 * 1024 * 500, 20);

    // Create loggers for each class
    cpu->logger = std::make_shared<spdlog::logger>("CPU", logger);
    cpu->memory->logger = std::make_shared<spdlog::logger>("Memory", logger);
    cpu->memory->joypad->logger = std::make_shared<spdlog::logger>("Joypad", logger);
    mbc->logger = std::make_shared<spdlog::logger>("MBC", logger);
    gpu->logger = std::make_shared<spdlog::logger>("GPU", logger);
    cartridgeReader->logger = std::make_shared<spdlog::logger>("CartridgeReader", logger);
}

void GBCEmulator::set_logging_level(spdlog::level::level_enum l)
{
    cpu->logger->set_level(l);
    cpu->memory->logger->set_level(l);
    cpu->memory->joypad->logger->set_level(l);
    mbc->logger->set_level(l);
    gpu->logger->set_level(l);
    cartridgeReader->logger->set_level(l);
}

bool GBCEmulator::frame_is_ready()
{
    return gpu->frame_is_ready;
}

SDL_Color * GBCEmulator::get_frame()
{
    return gpu->frame;
}

std::shared_ptr<CPU> GBCEmulator::get_CPU()
{
    if (cpu)
    {
        return cpu;
    }
    else
    {
        return NULL;
    }
}

std::shared_ptr<GPU> GBCEmulator::get_GPU()
{
    if (gpu)
    {
        return gpu;
    }
    else
    {
        return NULL;
    }
}

std::vector<uint8_t> GBCEmulator::get_memory_map()
{
    std::vector<uint8_t> memory_map;
    memory_map.reserve(0xFFFF);

    for (uint16_t i = 0; i < 0xFFFF; i++)
    {
        memory_map.push_back(cpu->memory->readByte(i));
    }

    return memory_map;
}

std::vector<uint8_t> GBCEmulator::get_partial_memory_map(uint16_t start_pos, uint16_t end_pos)
{
    std::vector<uint8_t> partial_memory_map;
    partial_memory_map.reserve(end_pos - start_pos);

    for (uint16_t i = start_pos; i < end_pos; i++)
    {
        partial_memory_map.push_back(cpu->memory->readByte(i));
    }
    return partial_memory_map;
}

void GBCEmulator::set_joypad_button(Joypad::BUTTON button)
{
    if (cpu->memory->joypad)
    {
        cpu->memory->joypad->set_joypad_button(button);
    }
}

void GBCEmulator::release_joypad_button(Joypad::BUTTON button)
{
    if (cpu->memory->joypad)
    {
        cpu->memory->joypad->release_joypad_button(button);
    }
}

void GBCEmulator::waitToRunInstruction()
{
    uint64_t ticksDiff = cpu->ticks - ticksRan;

    // Check if enough ticks have been used to produce 1 frame
    if (ticksDiff >= ticksPerFrame)
    {
        ticksRan = cpu->ticks;

        // Calculate time elapsed since start of frame
        auto currTimeDouble = getCurrentTime();
        auto timeElapsedMilli = currTimeDouble - frameStartTime;

        // Calculate amount of time to sleep until next frame
        auto timeToWaitMilli = timePerFrame - timeElapsedMilli;
        if (timeToWaitMilli.count() > 0)
        {   // Sleep until next frame needs to start rendering
            std::this_thread::sleep_for(timeToWaitMilli);
        }
    }
}

std::chrono::duration<double> GBCEmulator::getCurrentTime()
{
    auto currTime = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration<double>(currTime);
}

void GBCEmulator::setTimePerFrame(double d)
{
    timePerFrame = std::chrono::duration<double>(d);
}

void GBCEmulator::setFrameUpdateMethod(std::function<void(void)> function)
{
    frameIsUpdatedFunction = function;
}