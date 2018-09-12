#include <src/emuview.h>
#include <GBCEmulator.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

EmuView::EmuView(QObject * parent)
    : QGraphicsScene(parent),
        prevHash(0)
{

}

EmuView::EmuView(QObject * parent, QGraphicsView * graphicsView)
    :   QGraphicsScene(parent),
        emuView(graphicsView),
        prevHash(0)
{
    emuView->setScene(this);

    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);
}

EmuView::EmuView(QObject * parent, QGraphicsView * graphicsView, std::string filename)
    : QGraphicsScene(parent),
        emuView(graphicsView),
        prevHash(0)
{
    emuView->setScene(this);

    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);
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
        }
        emu.reset();
    }
}

void EmuView::setupEmulator(std::string filename, bool debugMode)
{
    if (emu)
    {
        emu->stop();
        thread->join();
        emu.reset();
    }

    emu = std::make_shared<GBCEmulator>(filename, filename + ".log", debugMode);

#ifdef QT_DEBUG
    emu->setTimePerFrame(0);
#endif
}

void EmuView::runEmulator()
{
    if (thread)
    {
        thread->join();
        thread.reset();
    }

    thread = std::make_shared<std::thread>([&]()
    {
        emu->run();
    });
}

void EmuView::runTo(uint16_t next_pc)
{
    if (thread)
    {
        thread->join();
        thread.reset();
    }
    thread = std::make_shared<std::thread>([&]()
    {
        emu->runTo(next_pc);
    });
}

void EmuView::connectEmulatorSignals()
{
    // Connect frame updater
    connect(&frameCheckTimer, &QTimer::timeout, [&]()
    {
        if (checkNewFrame())
        {
            frame = QImage((unsigned char *)emu->get_frame(), SCREEN_PIXEL_W, SCREEN_PIXEL_H, QImage::Format_ARGB32);
            QPixmap pixels = QPixmap::fromImage(frame);
            pixels = pixels.scaled(pixels.size() * scaleFrameToFit());
            this->clear();
            this->addPixmap(pixels);
            this->setSceneRect(pixels.rect());

            //int hash = hashImage(frame);

            //if (hash != prevHash && emu->get_CPU()->get_register_16(CPU::PC) > 0x0100)
            //{   // New frame/changed frame
            //    int a = 0;
            //    //emu->get_CPU()->startLogging = true;
            //}

            //prevHash = hash;
        }
    });
    frameCheckTimer.start((1.0 / SCREEN_FRAMERATE) * 1000);
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