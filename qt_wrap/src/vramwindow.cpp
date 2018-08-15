#include <vramwindow.h>
#include <GBCEmulator.h>
#include <Tile.h>
#include <vector>
#include <QImage>
#include <QMouseEvent>
#include <QGraphicsPixmapItem>

VRAMWindow::VRAMWindow(QWidget *parent)
    :   QMainWindow(parent),
        ui(new Ui::VRAMWindow)
{
    ui->setupUi(this);

    this->show();
}

VRAMWindow::VRAMWindow(QWidget *parent, std::shared_ptr<EmuView> emu)
    :   QMainWindow(parent),
        ui(new Ui::VRAMWindow),
        emuView(emu)
{
    ui->setupUi(this);

    if (emuView->emu)
    {
        gpu = emuView->emu->get_GPU();
    }

    initTileViews();

    this->show();
}

VRAMWindow::~VRAMWindow()
{

}

void VRAMWindow::initColorTable()
{
    if (!gpu)
    {
        return;
    }

    colorTable.clear();

    for (const auto & item : gpu->bg_palette_color)
    {
        colorTable.push_back(qRgb(item.r, item.g, item.b));
    }
}

void VRAMWindow::setGPU(std::shared_ptr<GPU> g)
{
    if (gpu)
    {
        gpu.reset();
    }
    gpu = g;
}

void VRAMWindow::initTileViews()
{
    int total_num_of_tiles = 384;

    tileViews.clear();
    tileViews.reserve(total_num_of_tiles);

    // First Tile set in VRAM
    for (int i = 0; i < total_num_of_tiles; i++)
    {
        tileViews.push_back(new QLabel(this));
        ui->gridLayout->addWidget(tileViews[i], i / 16, i % 16);
    }
}

void VRAMWindow::updateTileViews()
{
    std::vector<std::vector<Tile>> tiles = gpu->getBGTiles();
    initColorTable();

    int tileCounter = 0;
    for (auto & tileBlock : tiles)
    {
        for (auto & tile : tileBlock)
        {
            QImage image(8, 8, QImage::Format::Format_Indexed8);
            uint8_t row, column;
            uint8_t pixel_val;
            image.setColorTable(colorTable);
            for (uint8_t i = 0; i < 64; i++)
            {
                row     = i / 8;
                column  = i % 8;
                pixel_val = tile.getPixel(row, column);
                image.setPixel(column, row, pixel_val);
            }
            
            // Scale image
            image = image.scaled(8 * 2, 8 * 2, Qt::KeepAspectRatio);

            //image.save(QString::number(tileCounter) + ".bmp", "BMP");

            tileViews[tileCounter]->setPixmap(QPixmap::fromImage(image));
            tileCounter++;
        }

    }

}

void VRAMWindow::mousePressEvent(QMouseEvent * e)
{
    QWidget * w = childAt(e->pos());
    if (w)
    {
        const QLabel * imgLabel = qobject_cast<QLabel *>(w);

        if (imgLabel)
        {
            QImage image = imgLabel->pixmap()->toImage();
            QPixmap pixmap = QPixmap::fromImage(image);
            //QGraphicsPixmapItem img(*imgLabel->pixmap());
            QGraphicsPixmapItem img(pixmap);
            QGraphicsScene * scene = ui->graphicsView_Tile->scene();

            if (!scene)
            {
                scene = new QGraphicsScene(this);
                ui->graphicsView_Tile->setScene(scene);
            }
            scene->addItem(&img);
            ui->graphicsView_Tile->show();
        }
    }
}

void VRAMWindow::showEvent(QShowEvent * e)
{
    QMainWindow::showEvent(e);

    if (gpu)
    {
        updateTileViews();
    }
}