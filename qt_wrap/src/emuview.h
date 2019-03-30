#ifndef EMU_VIEW_H
#define EMU_VIEW_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QString>
#include <QTimer>
#include <QPixmap>

#include <chrono>
#include <memory>
#include <thread>

#include <spdlog/spdlog.h>

class GBCEmulator;
class JoypadXInput;

class EmuView : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit EmuView(QObject * parent = 0);
    EmuView(QObject * parent, QGraphicsView  * emuView, std::shared_ptr<spdlog::logger> logger);
    EmuView(QObject * parent, QGraphicsView  * emuView, std::string filename, std::shared_ptr<spdlog::logger> logger);
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
    std::shared_ptr<spdlog::logger> logger;

Q_SIGNALS:
    void updateFPS(QString fps);

private:
    int hashImage(const QImage & p);

    QObject * parent;
    std::shared_ptr<JoypadXInput> xinput;
    int prevHash;
    std::chrono::system_clock::duration prevTime;
};

#endif