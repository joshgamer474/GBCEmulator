#include <src/emuview.h>
#include <GBCEmulator.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

EmuView::EmuView(QObject * parent, QGraphicsView * graphicsView)
    :   QObject(parent),
        emuView(graphicsView),
        emuScene(new QGraphicsScene(this))
{
    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);
}

EmuView::EmuView(QObject * parent, QGraphicsView * graphicsView, std::string filename)
    :   QObject(parent),
        emuView(graphicsView),
        emuScene(new QGraphicsScene(this))
{
    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);
    setupEmulator(filename);
}

EmuView::~EmuView()
{
    if (emu)
    {
        emu->stop();
        thread.join();
        emu.reset();
    }
}

void EmuView::setupEmulator(std::string filename)
{
    if (emu)
    {
        emu->stop();
        thread.join();
        emu.reset();
    }

    emu = std::make_shared<GBCEmulator>(filename, filename + ".log");
}

void EmuView::runEmulator()
{
    thread = std::thread([&]()
    {
        emu->run();
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
            emuScene->clear();
            emuScene->addPixmap(pixels);
            emuScene->setSceneRect(pixels.rect());
            emuScene->fitInView
        }
    });
    frameCheckTimer.start(1);
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