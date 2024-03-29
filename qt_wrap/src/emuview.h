#ifndef EMU_VIEW_H
#define EMU_VIEW_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneDragDropEvent>
#include <QImage>
#include <QObject>
#include <QString>
#include <QPixmap>
#include <QTimer>
#include <memory>
#include <thread>

#include <spdlog/spdlog.h>

class GBCEmulator;
class JoypadInputInterface;

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
    std::shared_ptr<GBCEmulator> emu_savestate;
    QGraphicsView  *emuView;
    std::unique_ptr<QImage> frame;
    QPixmap frame_pixmap;
    QTimer fpsTimer;
    float fps;
    bool debugMode;
    std::shared_ptr<std::thread> thread;
    std::shared_ptr<spdlog::logger> logger;

Q_SIGNALS:
    void updateFPS(QString fps);

public slots:
    void takeSaveState();
    void loadSaveState();

protected:
    void dragEnterEvent(QGraphicsSceneDragDropEvent *);
    void dragMoveEvent(QGraphicsSceneDragDropEvent *);
    void dropEvent(QGraphicsSceneDragDropEvent *);

private:
    int hashImage(const QImage & p);

    QObject * parent;
    std::shared_ptr<JoypadInputInterface> xinput;
    int prevHash;
    bool fpsCountingInitialized;
    std::chrono::system_clock::duration prevTime;
    QTimer joypadTimer;
};

#endif