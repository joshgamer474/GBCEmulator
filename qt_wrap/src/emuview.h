#ifndef EMU_VIEW_H
#define EMU_VIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
//#include <QGraphicsScene>
//#include <QGraphicsView>
#include <QImage>
#include <QObject>
#include <QString>
#include <QPixmap>
#include <QTimer>
#include <JoypadInputInterface.h>
#include <memory>
#include <thread>

class GBCEmulator;

class EmuView : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit EmuView(QWidget * parent = 0);
    EmuView(QWidget * parent, std::string filename);
    virtual ~EmuView();

    void init();
    QOpenGLWidget* getThis();

    void setupEmulator(std::string filename, bool debugMode = false);
    void setupFPSCounting();
    void runEmulator();
    void runTo(uint16_t pc);
    void updateScene();
    void initFrame();
    bool checkNewFrame();
    uint8_t scaleFrameToFit();

    std::shared_ptr<GBCEmulator> emu;
    std::unique_ptr<QImage> frame;
    QPixmap frame_pixmap;
    QTimer fpsTimer;
    float fps;
    std::shared_ptr<std::thread> thread;

Q_SIGNALS:
    void updateFPS(QString fps);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height);

private:
    int hashImage(const QImage & p);
    void cleanup();

    QObject * parent;
    QOpenGLShaderProgram * program;
    std::shared_ptr<JoypadInputInterface> xinput;
    int prevHash;
    GLuint vao;
    QOpenGLBuffer vbo;
    GLuint texture;
};

#endif