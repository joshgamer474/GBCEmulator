#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <GBCEmulator.h>
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