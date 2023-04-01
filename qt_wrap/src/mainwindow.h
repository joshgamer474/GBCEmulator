#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <JoypadInputInterface.h>
#include <src/emuview.h>
#include <src/debuggerwindow.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    MainWindow(QWidget *parent, int argc, char *argv[]);
    ~MainWindow();

public Q_SLOTS:
    void openRomWindow();
    void openDebuggerWindow();
    void updateFPS(QString fps);

protected:
    void dragEnterEvent(QDragEnterEvent *);
    void dropEvent(QDropEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

private:
    void init();
    void openRom(const QString& filename);
    void connectSignalsSlots();
    void setLogLevels(spdlog::level::level_enum level);

    Ui::MainWindow *ui;
    std::shared_ptr<JoypadInputInterface> xinput;
    std::shared_ptr<DebuggerWindow> debuggerWindow;
    std::shared_ptr<EmuView> emuView;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> logger;
};

#endif // MAINWINDOW_H
