#ifndef EMU_VIEW_H
#define EMU_VIEW_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QPixmap>
#include <memory>
#include <thread>

class GBCEmulator;

class EmuView : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit EmuView(QObject * parent = 0);
    EmuView(QObject * parent, QGraphicsView  * emuView);
    EmuView(QObject * parent, QGraphicsView  * emuView, std::string filename);
    virtual ~EmuView();

    QGraphicsScene* getThis();

    void setupEmulator(std::string filename, bool debugMode = false);
    void runEmulator();
    void runTo(uint16_t pc);
    void connectEmulatorSignals();
    void updateScene();
    void initFrame();
    bool checkNewFrame();
    uint8_t scaleFrameToFit();

    std::shared_ptr<GBCEmulator> emu;
    QGraphicsView  *emuView;
    std::unique_ptr<QImage> frame;
    QPixmap frame_pixmap;
    QTimer frameCheckTimer;
    std::shared_ptr<std::thread> thread;

private:
    int hashImage(const QImage & p);

    int prevHash;
};

#endif