#include <src/emuview.h>
#include <src/mainwindow.h>

#include <GBCEmulator.h>
#include <JoypadXInput.h>
#include <JoypadInputInterface.h>

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QMainWindow>
#include <QMimeData>
#include <QThread>

#include <chrono>

EmuView::EmuView(QObject * parent)
    :   QGraphicsScene(parent),
        parent(parent),
        prevHash(0)
{

}

EmuView::EmuView(QObject * parent,
    QGraphicsView * graphicsView,
    std::shared_ptr<spdlog::logger> _logger)
    :   QGraphicsScene(parent),
        parent(parent),
        emuView(graphicsView),
        logger(_logger),
        prevHash(0)
{
    init();
}

EmuView::EmuView(QObject * parent,
    QGraphicsView * graphicsView,
    std::string filename,
    std::shared_ptr<spdlog::logger> _logger)
    :   QGraphicsScene(parent),
        parent(parent),
        emuView(graphicsView),
        logger(_logger),
        prevHash(0)
{
    init();
    setupEmulator(filename);
}

EmuView::~EmuView()
{
    if (emu)
    {
        emu->stop();
        if (thread)
        {
            thread->join();
            thread.reset();
        }
        emu.reset();
    }
}

void EmuView::init()
{
    logger->trace("Running init()");

    emuView->setScene(this);
    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);

//#ifdef _WIN32
    xinput = std::make_shared<JoypadXInput>();
//#endif // _WIN32
}

void EmuView::setupEmulator(std::string filename, bool debugMode)
{
    if (emu)
    {
        logger->info("Stopping current GBCEmulator");
        emu->stop();
        thread->join();
        emu.reset();
    }

    logger->info("Creating GBCEmulator, giving file: {0}", filename.c_str());
    emu = std::make_shared<GBCEmulator>(filename, filename + ".log", "", debugMode);

    if (xinput)
    {
        xinput->setJoypad(emu->get_Joypad());
    }

#ifdef QT_DEBUG
    emu->setTimePerFrame(0);
#endif

    initFrame();
    setupFPSCounting();
}

void EmuView::setupFPSCounting()
{
    fps = 0;
    connect(this, &EmuView::updateFPS, static_cast<MainWindow*>(parent), &MainWindow::updateFPS);
    connect(&fpsTimer, &QTimer::timeout, this, [this]()
    {
        emit updateFPS(QString::number(fps));
        fps = 0;
    });

    connect(&joypadTimer, &QTimer::timeout, this, [this]()
    {
        // Get controller Xinput
        xinput->refreshButtonStates(0);
    });
}

void EmuView::runEmulator()
{
    if (thread && thread->joinable())
    {
        logger->trace("Stopping GBCEmulator thread");
        thread->join();
        thread.reset();
    }

    logger->trace("Creating std::thread for GBCEmulator to run in");
    thread = std::make_shared<std::thread>([&]()
    {
        emu->run();
    });

    // Start FPS timer
    fpsTimer.start(1000);
    joypadTimer.start(1);
}

void EmuView::runTo(uint16_t next_pc)
{
    if (thread)
    {
        logger->trace("Stopping GBCEmulator thread");
        thread->join();
        thread.reset();
    }

    thread = std::make_shared<std::thread>([&]()
    {
        logger->trace("Creating std::thread for GBCEmulator to runTo in");
        emu->runTo(next_pc);
    });
}

void EmuView::initFrame()
{
    logger->trace("Initializing QImage frame, setting up frame getting function to GBCEmulator");

    // Create QImage frame
    frame = std::make_unique<QImage>(reinterpret_cast<unsigned char*>(emu->getFrameRaw()),
        SCREEN_PIXEL_W,
        SCREEN_PIXEL_H,
        QImage::Format_RGBA8888);

    // Setup frame update method
    emu->setFrameUpdateMethod(std::bind(&EmuView::updateScene, this));

    // Update scene
    updateScene();
}

bool EmuView::checkNewFrame()
{
    return emu->frame_is_ready();
}

uint8_t EmuView::scaleFrameToFit()
{
    QRect emuViewRect = emuView->rect();

    uint8_t widthScale  = emuViewRect.width()/ SCREEN_PIXEL_W;
    uint8_t heightScale = emuViewRect.height()/ SCREEN_PIXEL_H;

    // Return smallest scale ratio
    return (widthScale > heightScale) ? heightScale : widthScale;
}

QGraphicsScene* EmuView::getThis()
{
    return this;
}

int EmuView::hashImage(const QImage & image)
{
    int hash = 0;
    for (int y = 0; y < image.height(); y++)
    {
        for (int x = 0; x < image.width(); x++)
        {
            const QRgb pixel = image.pixel(x, y);
            hash += pixel;
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
    }
    return hash;
}

void EmuView::updateScene()
{
    if (QThread::currentThread() != QApplication::instance()->thread())
    {
        QMetaObject::invokeMethod(this, &EmuView::updateScene);
        return;
    }

    fps++;

    auto currTime = std::chrono::system_clock::now().time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(currTime - prevTime);
    int microInt = microseconds.count();
    logger->trace("Time between frame {}: {} milliseconds",
        fps,
        microInt / 1000.0);
    prevTime = currTime;

    // Create new QPixmap from QImage
    frame_pixmap = QPixmap::fromImage(*frame);
    frame_pixmap = frame_pixmap.scaled(frame_pixmap.size() * scaleFrameToFit());

    // Refresh QGraphicsScene
    this->clear();
    this->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    this->addPixmap(frame_pixmap);
    this->setSceneRect(frame_pixmap.rect());
}

void EmuView::takeSaveState()
{
    if (!emu)
    {
        return;
    }

    // Stop the emulator first so we don't save it while running
    emu->setStopRunning(true);
    if (emu_savestate)
    {
        emu_savestate.reset();
    }

    // Create emulator savestate
    emu_savestate = std::make_shared<GBCEmulator>(emu->getROMName(), emu->getROMName() + ".log");

    // Copy current emulator into emulator savestate
    *emu_savestate.get() = *emu.get();

    // Start emulator again
    runEmulator();
}

void EmuView::loadSaveState()
{
    if (!emu_savestate || !emu)
    {   // Need a save state and an emulator to load a savestate
        return;
    }

    emu->stop();
    if (thread && thread->joinable())
    {
        // Close emulator thread
        thread->join();
    }

    // Load emulator savestate into emulator
    *emu.get() = *emu_savestate.get();

    // Start emulator again
    runEmulator();
}
