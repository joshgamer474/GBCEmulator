#include <src/emuview.h>
#include <GBCEmulator.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QThread>
#include <QApplication>

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

    initFrame();
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

void EmuView::initFrame()
{
    // Create QImage frame
    frame = std::make_unique<QImage>((unsigned char *)emu->get_frame(),
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

    // Create new QPixmap from QImage
    frame_pixmap = QPixmap::fromImage(*frame);
    frame_pixmap = frame_pixmap.scaled(frame_pixmap.size() * scaleFrameToFit());

    // Refresh QGraphicsScene
    this->clear();
    this->addPixmap(frame_pixmap);
    this->setSceneRect(frame_pixmap.rect());
}