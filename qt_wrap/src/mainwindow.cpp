#include <mainwindow.h>
#include <ui_mainwindow.h>

#include <GBCEmulator.h>
#include <Joypad.h>
#include <JoypadXInput.h>

#include <QDropEvent>
#include <QDragEnterEvent>
#include <QGraphicsView>
#include <QFileDialog>
#include <QMimeData>

#include <string>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    init();
    ui->graphicsView->hide();
}

MainWindow::MainWindow(QWidget *parent, int argc, char *argv[])
    :   QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    init();
}

MainWindow::~MainWindow()
{
    if (emuView)
    {
        emuView.reset();
    }

    delete ui;
}

void MainWindow::init()
{
    ui->setupUi(this);
    logger = std::make_shared<spdlog::sinks::rotating_file_sink_st>("GBCEmulator_qt.log", 1024 * 1024 * 500, 1);

    emuView = std::make_shared<EmuView>(this, ui->graphicsView, std::make_shared<spdlog::logger>("EmuView", logger));
    xinput = std::make_shared<JoypadXInput>();
    setLogLevels(spdlog::level::info);

    connectSignalsSlots();
}

void MainWindow::connectSignalsSlots()
{
    connect(ui->actionOpen_ROM, &QAction::triggered, this, &MainWindow::openRomWindow);
    connect(ui->actionOpen_Debugger, &QAction::triggered, this, &MainWindow::openDebuggerWindow);
}

void MainWindow::openRomWindow()
{
  QFileDialog dialog;
  dialog.setNameFilter("GB/GBC (*.gb, *.gbc, *.zip)");
  dialog.exec();
  const QString filename = dialog.selectedFiles().first();
  openRom(filename);
}

void MainWindow::openDebuggerWindow()
{
    if (!debuggerWindow)
    {
        debuggerWindow = std::make_shared<DebuggerWindow>(this, emuView);
    }

    emuView->debugMode = true;
    debuggerWindow->show();
}

void MainWindow::updateFPS(QString fps)
{
    this->setWindowTitle("GBCEmulator | " + fps);
}

void MainWindow::openRom(const QString& filename)
{
    if (debuggerWindow)
    {
        emuView->setupEmulator(filename.toStdString(), true);   // Run in debug mode
        debuggerWindow->initEmulatorConnections(emuView->emu);
    }
    else
    {
        emuView->setupEmulator(filename.toStdString());         // Run in normal mode
        emuView->runEmulator();
    }
    ui->graphicsView->show();
}

void MainWindow::dragEnterEvent(QDragEnterEvent * e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent * e)
{
    for (const QUrl & url : e->mimeData()->urls())
    {
        ui->graphicsView->show();

        QString filename = url.toLocalFile();
        openRom(filename);
    }
}

void MainWindow::keyPressEvent(QKeyEvent * e)
{
    if (!emuView || !emuView->emu)
    {
        return;
    }

    Joypad::BUTTON button = Joypad::BUTTON::NONE;

    switch (e->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        button = Joypad::UP;
        break;
    case Qt::Key_S:
    case Qt::DownArrow:
        button = Joypad::DOWN;
        break;
    case Qt::Key_A:
    case Qt::LeftArrow:
        button = Joypad::LEFT;
        break;
    case Qt::Key_D:
    case Qt::RightArrow:
        button = Joypad::RIGHT;
        break;
    case Qt::Key_Z:
        button = Joypad::A;
        break;
    case Qt::Key_X:
        button = Joypad::B;
        break;
    case Qt::Key_N:
        button = Joypad::SELECT;
        break;
    case Qt::Key_M:
        button = Joypad::START;
        break;
    default:
        return;
    }

    emuView->emu->set_joypad_button(button);
}

void MainWindow::keyReleaseEvent(QKeyEvent * e)
{
    if (!emuView || !emuView->emu)
    {
        return;
    }

    Joypad::BUTTON button = Joypad::BUTTON::NONE;

    switch (e->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        button = Joypad::UP;
        break;
    case Qt::Key_S:
    case Qt::DownArrow:
        button = Joypad::DOWN;
        break;
    case Qt::Key_A:
    case Qt::LeftArrow:
        button = Joypad::LEFT;
        break;
    case Qt::Key_D:
    case Qt::RightArrow:
        button = Joypad::RIGHT;
        break;
    case Qt::Key_Z:
        button = Joypad::A;
        break;
    case Qt::Key_X:
        button = Joypad::B;
        break;
    case Qt::Key_N:
        button = Joypad::SELECT;
        break;
    case Qt::Key_M:
        button = Joypad::START;
        break;
    default:
        return;
    }

    emuView->emu->release_joypad_button(button);
}

void MainWindow::setLogLevels(spdlog::level::level_enum level)
{
    if (emuView)
    {
        emuView->logger->set_level(level);
    }
}