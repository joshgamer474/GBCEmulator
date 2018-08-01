#ifndef DEBUGGER_WINDOW_H
#define DEBUGGER_WINDOW_H

#include <QMainWindow>
#include <src/emuwindow.h>

namespace Ui {
    class DebuggerWindow;
}

class DebuggerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DebuggerWindow(QWidget *parent = 0);
    DebuggerWindow(QWidget *parent, std::shared_ptr<EmuWindow> emu);
    virtual ~DebuggerWindow();

private:
    Ui::DebuggerWindow *ui;
    std::shared_ptr<EmuWindow> emuWindow;
};

#endif