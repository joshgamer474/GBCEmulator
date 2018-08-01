#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <string>
#include <GBCEmulator.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    emuWindow = std::make_shared<EmuWindow>(this, ui->graphicsView);
    ui->graphicsView->setScene(emuWindow->emuScene);

    connect(ui->actionOpen_Debugger, &QAction::triggered, [&]()
    {
        debuggerWindow = std::make_shared<DebuggerWindow>(this, emuWindow);
    });
}

MainWindow::MainWindow(QWidget *parent, int argc, char *argv[])
    :   QMainWindow(parent),
        ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setAcceptDrops(true);

    emuWindow = std::make_shared<EmuWindow>(this, ui->graphicsView);
    ui->graphicsView->setScene(emuWindow->emuScene);

    connect(ui->actionOpen_Debugger, &QAction::triggered, [&]()
    {
        debuggerWindow = std::make_shared<DebuggerWindow>(this, emuWindow);
    });
}

MainWindow::~MainWindow()
{
    if (emuWindow)
    {
        emuWindow.reset();
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
        QString filename = url.toLocalFile();
        emuWindow->setupEmulator(filename.toStdString());
    }
}

void MainWindow::resizeEvent(QResizeEvent * e)
{
    ui->graphicsView->resize(e->size());
}