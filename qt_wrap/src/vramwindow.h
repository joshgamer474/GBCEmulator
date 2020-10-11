#ifndef VRAM_WINDOW_H
#define VRAM_WINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <memory>
#include <map>
#include <ui_vramwindow.h>
#include <src/emuview.h>
#include <GPU.h>

namespace Ui {
    class VRAMWindow;
}

#define TOTAL_NUM_TILES 384

class VRAMWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VRAMWindow(QWidget *parent = 0);
    VRAMWindow(QWidget *parent, std::shared_ptr<EmuView> emu);
    virtual ~VRAMWindow();

    void initColorTable();
    void updateColorTable();
    void setGPU(std::shared_ptr<GPU>);
    void initBackgroundMap();
    void updateTileViews();

    std::shared_ptr<GPU> gpu;

private:
    void initTileViews();
    void initBackgroundMapImage();
    void mousePressEvent(QMouseEvent * e);
    void showEvent(QShowEvent * e);
    QVector<QRgb> getColorTableFromPtr(ColorPalette * color_palette);
    void updateQRgb(QRgb & qrgb, const SDL_Color & color);

    Ui::VRAMWindow *ui;
    std::shared_ptr<EmuView> emuView;
    QVector<QVector<QLabel *>> tileViews;
    QVector<QVector<QRgb>> colorTables;
    QVector<QRgb> whiteColorTable;
    std::unique_ptr<QLabel> bgImageLabel;
    std::unique_ptr<QImage> bgImage;
    QTimer bgTimer;
};

#endif