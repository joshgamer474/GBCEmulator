#include "debuggerwindow.h"
#include "ui_debuggerwindow.h"
#include <string>
#include <GBCEmulator.h>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

DebuggerWindow::DebuggerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DebuggerWindow)
{
    ui->setupUi(this);
    this->show();
}

DebuggerWindow::DebuggerWindow(QWidget *parent, std::shared_ptr<EmuWindow> emu)
    : QMainWindow(parent),
    ui(new Ui::DebuggerWindow),
    emuWindow(emu)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(emuWindow->emuScene);
    this->show();
}

DebuggerWindow::~DebuggerWindow()
{
    if (emuWindow)
    {
        emuWindow.reset();
    }

    delete ui;
}