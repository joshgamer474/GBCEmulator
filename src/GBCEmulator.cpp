#include "GBCEmulator.h"

GBCEmulator::GBCEmulator(const std::string romName) :
    cpu(new CPU()),
    memory(new Memory()),
    cartridgeReader(new CartridgeReader()),
    mbc(new MBC()),
    joypad(new Joypad()),
    stop(false),
    logFileBaseName("log.txt")
{
    init_SDL();
    gpu = new GPU(renderer);

    init_logging();
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

    if (cpu)
        delete cpu;
    if (memory)
        delete memory;
    if (cartridgeReader)
        delete cartridgeReader;
    if (mbc)
        delete mbc;
    if (joypad)
        delete joypad;
    if (gpu)
        delete gpu;
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
    memory->cartridgeReader = cartridgeReader;
    memory->mbc = mbc;
    memory->initROMBanks();
    memory->joypad = joypad;
    joypad->memory = memory;
    gpu->cpu = cpu;
    cpu->memory = memory;
}

void GBCEmulator::init_gpu()
{
    memory->gpu = gpu;
    gpu->joypad = joypad;
    gpu->memory = memory;
    /*if (cartridgeReader->getColorGBFlag())
        gpu->init_color_gb();*/
    gpu->init_color_gb();
}

void GBCEmulator::run()
{
    while (!stop)
    {
        cpu->runNextInstruction();
        memory->updateTimer(cpu->ticks, CLOCK_SPEED);
        gpu->run();

        if (logCounter % 100 == 0)
        {
            logCounter = 0;
            logger->flush();
        }
        logCounter++;
    }
}

void GBCEmulator::init_logging()
{
    // Create logger
    logger = std::make_shared<spdlog::sinks::rotating_file_sink_st>("log.txt", 1024 * 1024 * 500, 20);

    // Create loggers for each class
    cpu->logger = std::make_shared<spdlog::logger>("CPU", logger);
    memory->logger = std::make_shared<spdlog::logger>("Memory", logger);
    mbc->logger = std::make_shared<spdlog::logger>("MBC", logger);
    gpu->logger = std::make_shared<spdlog::logger>("GPU", logger);
    cartridgeReader->logger = std::make_shared<spdlog::logger>("CartridgeReader", logger);
    joypad->logger = std::make_shared<spdlog::logger>("Joypad", logger);
}

void GBCEmulator::set_logging_level(spdlog::level::level_enum l)
{
    cpu->logger->set_level(l);
    memory->logger->set_level(l);
    mbc->logger->set_level(l);
    gpu->logger->set_level(l);
    cartridgeReader->logger->set_level(l);
    joypad->logger->set_level(l);
}