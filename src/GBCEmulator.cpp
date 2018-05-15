#include "GBCEmulator.h"

GBCEmulator::GBCEmulator(const std::string romName, const std::string logName) :
    cpu(std::make_shared<CPU>()),
    cartridgeReader(std::make_shared<CartridgeReader>()),
    mbc(std::make_shared<MBC>()),
    stopRunning(false),
    logFileBaseName(logName)
{
    init_SDL();
    gpu = std::make_shared<GPU>(renderer);

    init_logging(logName);
    set_logging_level(spdlog::level::trace);
    gpu->logger->set_level(spdlog::level::warn);
    cpu->logger->set_level(spdlog::level::warn);
    logCounter = 0;

    read_rom(romName);

    // Initialize memory objects, link GB components together
    init_memory();
    init_gpu();
}

GBCEmulator::~GBCEmulator()
{
    SDL_DestroyWindow(window);
    SDL_Quit();

    cpu->memory->reset();

    if (cpu)
        cpu.reset();
    if (cartridgeReader)
        cartridgeReader.reset();
    if (mbc)
        mbc.reset();
    if (gpu)
        gpu.reset();
    if (logger)
        logger.reset();
}

void GBCEmulator::init_SDL()
{
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
}

void GBCEmulator::read_rom(std::string filename)
{
    cartridgeReader->setRomDestination(filename);
    bool successful = cartridgeReader->readRom();

    if (!successful)
        throw std::runtime_error("Could not find file " + filename);
}

void GBCEmulator::init_memory()
{
    // Setup Memory Bank Controller
    mbc->MBC_init(cartridgeReader->cartridgeType.mbc);

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
    /*if (cartridgeReader->getColorGBFlag())
        gpu->init_color_gb();*/
    gpu->init_color_gb();
}

void GBCEmulator::run()
{
    while (!stopRunning)
    {
        cpu->runNextInstruction();
        cpu->memory->updateTimer(cpu->ticks, CLOCK_SPEED);
        gpu->run();

        if (logCounter % 100 == 0)
        {
            logCounter = 0;
            logger->flush();
        }
        logCounter++;
    }
}

void GBCEmulator::stop()
{
    stopRunning = true;
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