#ifndef EMU_WINDOW_H
#define EMU_WINDOW_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <memory>
#include <thread>

class GBCEmulator;

class EmuWindow : public QObject
{
    Q_OBJECT

public:
    EmuWindow(QObject * parent, QGraphicsView  * emuView);
    EmuWindow(QObject * parent, QGraphicsView  * emuView, std::string filename);
    virtual ~EmuWindow();

    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);
    void dragMoveEvent(QDragMoveEvent *);
    void setupEmulator(std::string filename);
    void connectEmulatorSignals();
    bool checkNewFrame();
    uint8_t scaleFrameToFit();

    std::unique_ptr<GBCEmulator> emu;
    QGraphicsScene *emuScene;
    QGraphicsView  *emuView;
    QImage frame;
    QTimer frameCheckTimer;
    std::thread thread;
};

#endif