#include <src/emuwindow.h>
#include <GBCEmulator.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

EmuWindow::EmuWindow(QObject * parent, QGraphicsView * graphicsView)
    :   QObject(parent),
        emuView(graphicsView),
        emuScene(new QGraphicsScene(this))
{
    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);
}

EmuWindow::EmuWindow(QObject * parent, QGraphicsView * graphicsView, std::string filename)
    :   QObject(parent),
        emuView(graphicsView),
        emuScene(new QGraphicsScene(this))
{
    emuView->setAlignment(Qt::AlignCenter);
    emuView->setAcceptDrops(true);
    setupEmulator(filename);
}

EmuWindow::~EmuWindow()
{
    if (emu)
    {
        emu->stop();
        thread.join();
        emu.reset();
    }
}

void EmuWindow::setupEmulator(std::string filename)
{
    if (emu)
    {
        emu->stop();
        thread.join();
        emu.reset();
    }

    emu = std::make_unique<GBCEmulator>(filename, filename + ".log");

    thread = std::thread([&]()
    {
        emu->run();
    });

    connectEmulatorSignals();
}

void EmuWindow::connectEmulatorSignals()
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
        }
    });
    frameCheckTimer.start(1);
}

bool EmuWindow::checkNewFrame()
{
    return emu->frame_is_ready();
}

uint8_t EmuWindow::scaleFrameToFit()
{
    QRect emuViewRect = emuView->rect();

    uint8_t widthScale  = emuViewRect.width()/ SCREEN_PIXEL_W;
    uint8_t heightScale = emuViewRect.height()/ SCREEN_PIXEL_H;

    // Return smallest scale ratio
    return (widthScale > heightScale) ? heightScale : widthScale;
}

void EmuWindow::dragMoveEvent(QDragMoveEvent * e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void EmuWindow::dragEnterEvent(QDragEnterEvent * e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void EmuWindow::dropEvent(QDropEvent * e)
{
    for (const QUrl & url : e->mimeData()->urls())
    {
        QString filename = url.toLocalFile();
        setupEmulator(filename.toStdString());
    }
}