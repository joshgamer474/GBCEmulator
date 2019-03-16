#ifndef EMU_VIEW_H
#define EMU_VIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImage>
#include <QObject>
#include <QString>
#include <QPixmap>
#include <QTimer>
#include <JoypadInputInterface.h>
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

    void init();
    QGraphicsScene* getThis();

    void setupEmulator(std::string filename, bool debugMode = false);
    void setupFPSCounting();
    void runEmulator();
    void runTo(uint16_t pc);
    void updateScene();
    void initFrame();
    bool checkNewFrame();
    uint8_t scaleFrameToFit();

    std::shared_ptr<GBCEmulator> emu;
    QGraphicsView  *emuView;
    std::unique_ptr<QImage> frame;
    QPixmap frame_pixmap;
    QTimer fpsTimer;
    float fps;
    std::shared_ptr<std::thread> thread;

Q_SIGNALS:
    void updateFPS(QString fps);

private:
    int hashImage(const QImage & p);

    QObject * parent;
    std::shared_ptr<JoypadInputInterface> xinput;
    int prevHash;
};

#endif