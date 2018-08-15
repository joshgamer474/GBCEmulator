#ifndef VRAM_WINDOW_H
#define VRAM_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <ui_vramwindow.h>
#include <src/emuview.h>
#include <memory>
#include <GPU.h>

namespace Ui {
    class VRAMWindow;
}

class VRAMWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VRAMWindow(QWidget *parent = 0);
    VRAMWindow(QWidget *parent, std::shared_ptr<EmuView> emu);
    virtual ~VRAMWindow();

    void initColorTable();
    void setGPU(std::shared_ptr<GPU>);
    void updateTileViews();

    std::shared_ptr<GPU> gpu;

private:
    void initTileViews();
    void mousePressEvent(QMouseEvent * e);
    void showEvent(QShowEvent * e);

    Ui::VRAMWindow *ui;
    std::shared_ptr<EmuView> emuView;
    QList<QLabel *> tileViews;
    QVector<QRgb> colorTable;
};

#endif