#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <src/emuview.h>
#include <src/debuggerwindow.h>

namespace Ui {
class MainWindow;
}

class JoypadXInput;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    MainWindow(QWidget *parent, int argc, char *argv[]);
    ~MainWindow();

public Q_SLOTS:
    void openDebuggerWindow();
    void updateFPS(QString fps);

private:
    void connectSignalsSlots();
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

    Ui::MainWindow *ui;
    std::shared_ptr<DebuggerWindow> debuggerWindow;
    std::shared_ptr<EmuView> emuView;
    std::shared_ptr<JoypadXInput> xinput;
};

#endif // MAINWINDOW_H
