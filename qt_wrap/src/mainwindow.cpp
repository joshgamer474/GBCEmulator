#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <GBCEmulator.h>
#include <Joypad.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QGraphicsView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    emuView = std::make_shared<EmuView>(this, ui->graphicsView);
    ui->graphicsView->hide();

    connectSignalsSlots();
}

MainWindow::MainWindow(QWidget *parent, int argc, char *argv[])
    :   QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    emuView = std::make_shared<EmuView>(this, ui->graphicsView);

    connectSignalsSlots();
}

MainWindow::~MainWindow()
{
    if (emuView)
    {
        emuView.reset();
    }

    delete ui;
}

void MainWindow::connectSignalsSlots()
{
    connect(ui->actionOpen_Debugger, &QAction::triggered, this, &MainWindow::openDebuggerWindow);
}

void MainWindow::openDebuggerWindow()
{
    if (!debuggerWindow)
    {
        debuggerWindow = std::make_shared<DebuggerWindow>(this, emuView);
    }

    debuggerWindow->show();
}

void MainWindow::updateFPS(QString fps)
{
    this->setWindowTitle("GBCEmulator | " + fps);
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
    }
}

void MainWindow::keyPressEvent(QKeyEvent * e)
{
    if (!emuView || !emuView->emu)
    {
        return;
    }

    switch (e->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        emuView->emu->set_joypad_button(Joypad::UP); break;
    case Qt::Key_S:
    case Qt::DownArrow:
        emuView->emu->set_joypad_button(Joypad::DOWN); break;
    case Qt::Key_A:
    case Qt::LeftArrow:
        emuView->emu->set_joypad_button(Joypad::LEFT); break;
    case Qt::Key_D:
    case Qt::RightArrow:
        emuView->emu->set_joypad_button(Joypad::RIGHT); break;
    case Qt::Key_Z: emuView->emu->set_joypad_button(Joypad::A); break;
    case Qt::Key_X: emuView->emu->set_joypad_button(Joypad::B); break;
    case Qt::Key_N: emuView->emu->set_joypad_button(Joypad::SELECT); break;
    case Qt::Key_M: emuView->emu->set_joypad_button(Joypad::START); break;
    default:
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent * e)
{
    if (!emuView || !emuView->emu)
    {
        return;
    }

    switch (e->key())
    {
    case Qt::Key_W:
    case Qt::UpArrow:
        emuView->emu->release_joypad_button(Joypad::UP); break;
    case Qt::Key_S:
    case Qt::DownArrow:
        emuView->emu->release_joypad_button(Joypad::DOWN); break;
    case Qt::Key_A:
    case Qt::LeftArrow:
        emuView->emu->release_joypad_button(Joypad::LEFT); break;
    case Qt::Key_D:
    case Qt::RightArrow:
        emuView->emu->release_joypad_button(Joypad::RIGHT); break;
    case Qt::Key_Z: emuView->emu->release_joypad_button(Joypad::A); break;
    case Qt::Key_X: emuView->emu->release_joypad_button(Joypad::B); break;
    case Qt::Key_N: emuView->emu->release_joypad_button(Joypad::SELECT); break;
    case Qt::Key_M: emuView->emu->release_joypad_button(Joypad::START); break;
    default:
        break;
    }
}