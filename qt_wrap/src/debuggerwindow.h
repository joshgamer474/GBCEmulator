#ifndef DEBUGGER_WINDOW_H
#define DEBUGGER_WINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <src/emuview.h>
#include <hexwidget.h>
#include <CPU.h>
#include <vramwindow.h>

namespace Ui {
    class DebuggerWindow;
}

class DebuggerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DebuggerWindow(QWidget *parent = 0);
    DebuggerWindow(QWidget *parent, std::shared_ptr<EmuView> emu);
    virtual ~DebuggerWindow();

    void showEvent(QShowEvent * e);
    void hideEvent(QHideEvent * e);
    void initEmulatorConnections(std::shared_ptr<GBCEmulator> emu);
    void connectToolbarButtons();

signals:
    void runEmulator();
    void runTo(uint16_t);


private:
    void updateRegisterLabels();
    void updateHexWidget(bool getFullMemoryMap = false);

    template<typename T>
    QString makeQStringHex(T number, int num_digits = 4);

    Ui::DebuggerWindow *ui;
    HexWidget *hexWidget;
    VRAMWindow *vramWindow;
    std::shared_ptr<EmuView> emuView;
    std::shared_ptr<GBCEmulator> emu;
    std::shared_ptr<CPU> cpu;
    QTimer updateGUITimer;
    uint16_t pc, next_pc;
    bool isHidden;
};

#endif