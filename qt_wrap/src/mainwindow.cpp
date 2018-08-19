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

    connect(ui->actionOpen_Debugger, &QAction::triggered, [&]()
    {
        debuggerWindow = std::make_shared<DebuggerWindow>(this, emuView);
    });
}

MainWindow::MainWindow(QWidget *parent, int argc, char *argv[])
    :   QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    emuView = std::make_shared<EmuView>(this, ui->graphicsView);

    connect(ui->actionOpen_Debugger, &QAction::triggered, [&]()
    {
        if (debuggerWindow)
        {
            debuggerWindow->show();
        }
        else
        {
            debuggerWindow = std::make_shared<DebuggerWindow>(this, emuView);
        }
    });
}

MainWindow::~MainWindow()
{
    if (emuView)
    {
        emuView.reset();
    }

    delete ui;
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
        emuView->connectEmulatorSignals();
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
    case Qt::Key_W: emuView->emu->set_joypad_button(Joypad::UP); break;
    case Qt::Key_S: emuView->emu->set_joypad_button(Joypad::DOWN); break;
    case Qt::Key_A: emuView->emu->set_joypad_button(Joypad::LEFT); break;
    case Qt::Key_D: emuView->emu->set_joypad_button(Joypad::RIGHT); break;
    case Qt::Key_Z: emuView->emu->set_joypad_button(Joypad::A); break;
    case Qt::Key_X: emuView->emu->set_joypad_button(Joypad::B); break;
    case Qt::Key_N: emuView->emu->set_joypad_button(Joypad::START); break;
    case Qt::Key_M: emuView->emu->set_joypad_button(Joypad::SELECT); break;
    default:
        break;
    }
}