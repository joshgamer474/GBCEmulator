#include <src/emuview.h>
#include <GBCEmulator.h>
#include <src/mainwindow.h>
#include <JoypadXInput.h>

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QMainWindow>
#include <QMimeData>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QThread>

EmuView::EmuView(QWidget * parent)
    :   QOpenGLWidget(parent),
        parent(parent),
        prevHash(0)
{
    init();
}

EmuView::EmuView(QWidget * parent, std::string filename)
    :   QOpenGLWidget(parent),
        parent(parent),
        prevHash(0)
{
    init();
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
            thread.reset();
        }
        emu.reset();
    }

    cleanup();
}

void EmuView::init()
{
    //emuView->setScene(this);
    // emuView->setAlignment(Qt::AlignCenter);
    // emuView->setAcceptDrops(true);

#ifdef _WIN32
    xinput = std::make_shared<JoypadXInput>();
#endif // _WIN32

    // QTimer::singleShot(1, [this]()
    // {
    //     initializeGL();
    // });
}

void EmuView::initializeGL()
{
    makeCurrent();

    //logger->info("context() returned: {0:b}", context);
    initializeOpenGLFunctions();
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);

    if (frame)
    {
        glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, frame->width(), frame->height(), 0, GL_RGB8, GL_UNSIGNED_BYTE, frame->bits());
        //glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    
    
    
    // Create glshader
    QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);

    // Create OpenGLShaderProgram
    program = new QOpenGLShaderProgram();
    program->addShader(vertexShader);

    program->link();
    program->bind();
    
    glActiveTexture(GL_TEXTURE0);
    program->setUniformValue("texture", 0);

    glClearColor(1.0f, 0.0f, 0, 0);

    doneCurrent();
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

    if (xinput)
    {
        xinput->setJoypad(emu->get_Joypad());
    }

#ifdef QT_DEBUG
    emu->setTimePerFrame(0);
#endif

    initFrame();
    setupFPSCounting();

    initializeGL();
}

void EmuView::setupFPSCounting()
{
    fps = 0;
    connect(this, &EmuView::updateFPS, static_cast<MainWindow*>(parent), &MainWindow::updateFPS);
    connect(&fpsTimer, &QTimer::timeout, this, [this]()
    {
        emit updateFPS(QString::number(fps));
        fps = 0;
    });
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

    // Start FPS timer
    fpsTimer.start(1000);
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
    frame = std::make_unique<QImage>(reinterpret_cast<unsigned char*>(emu->get_frame()),
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
    // QRect emuViewRect = emuView->rect();

    // uint8_t widthScale  = emuViewRect.width()/ SCREEN_PIXEL_W;
    // uint8_t heightScale = emuViewRect.height()/ SCREEN_PIXEL_H;

    // // Return smallest scale ratio
    // return (widthScale > heightScale) ? heightScale : widthScale;
    return 400;
}

QOpenGLWidget* EmuView::getThis()
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

    fps++;

    // Create new QPixmap from QImage
    // frame_pixmap = QPixmap::fromImage(*frame);
    // frame_pixmap = frame_pixmap.scaled(frame_pixmap.size() * scaleFrameToFit());

    // // Refresh QGraphicsScene
    // this->clear();
    // this->setBackgroundBrush(QBrush(Qt::black, Qt::SolidPattern));
    // this->addPixmap(frame_pixmap);
    // this->setSceneRect(frame_pixmap.rect());

    if (frame)
    {
        // makeCurrent();
        // glBindTexture(GL_TEXTURE_2D, texture);
        //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, frame->width(), frame->height(), 0, GL_RGB8, GL_UNSIGNED_BYTE, frame->bits());
        // glBindTexture(GL_TEXTURE_2D, 0);
        // doneCurrent();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, frame->width(), frame->height(), 0, GL_RGB, GL_UNSIGNED_BYTE, frame->bits());
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glColor3f(1.0, 1.0, 1.0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(1, -1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, 1);
        glTexCoord2f(1, 1);
        glVertex2f(1, 1);
        glEnd();
        glPopMatrix();

    }

    if (xinput)
    {   // Get controller Xinput
        xinput->refreshButtonStates(0);
    }
}

void EmuView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    makeCurrent();

    program->bind();

    float vertices[] =
    {
        -1.0,   -1.0,
        1.0,    -1.0,
        1.0,    1.0,
        -1.0,   1.0
    };

    float coordTexture[] =
    {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0
    };

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, coordTexture);
    glEnableVertexAttribArray(2);

    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawArrays(GL_QUADS, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(0);

    program->release();
}

void EmuView::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void EmuView::cleanup()
{
    makeCurrent();
    doneCurrent();
}