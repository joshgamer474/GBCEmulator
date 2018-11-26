#include <vramwindow.h>
#include <GBCEmulator.h>
#include <ColorPalette.h>
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
    initBackgroundMap();
    initBackgroundMapImage();

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

    colorTables.clear();

    if (gpu->is_color_gb)
    {
        colorTables.resize(8);
        // Initialize background tile colors for CGB
        for (int i = 0; i < gpu->cgb_background_palettes.size(); i++)
        {   // Get color palette
            auto & palette = gpu->cgb_background_palettes[i];

            for (int j = 0; j < CGB_NUM_COLORS_PER_PALETTE; j++)
            {   // Get color from palette
                const auto & color = palette.getColor(j);
                colorTables[i].push_back(qRgb(color.r, color.g, color.b));
            }
        }
        // Implement colors for sprites?
    }
    else
    {
        colorTables.resize(1);
        for (const auto & item : gpu->bg_palette_color)
        {
            colorTables[0].push_back(qRgb(item.r, item.g, item.b));
        }
    }
}

void VRAMWindow::setGPU(std::shared_ptr<GPU> g)
{
    if (gpu)
    {
        gpu.reset();
    }
    gpu = g;

    // Initialize background label and image
    if (!bgImageLabel)
    {
        initBackgroundMap();
    }

    if (!bgImage)
    {
        initBackgroundMapImage();
    }
}

void VRAMWindow::initTileViews()
{
    tileViews.clear();
    tileViews.resize(2);
    tileViews[0].reserve(TOTAL_NUM_TILES);
    tileViews[1].reserve(TOTAL_NUM_TILES);

    // Both Tile sets in VRAM
    for (int i = 0; i < 2; i++)
    //for (auto & tileSet : tileViews)
    {   // Initialize tile set with QLabels
        auto & tileSet = tileViews[i];
        for (int j = 0; j < TOTAL_NUM_TILES; j++)
        {
            tileSet.push_back(new QLabel(this));

            if (i == 0)
            {
                ui->gridLayout->addWidget(tileSet[j], j / 16, j % 16);
            }
            else
            {
                ui->gridLayout2->addWidget(tileSet[j], j / 16, j % 16);
            }
        }
    }
}

void VRAMWindow::initBackgroundMap()
{
    if (bgImageLabel)
    {
        return;
    }

    bgImageLabel = std::make_unique<QLabel>(this);
    ui->verticalLayout->addWidget(bgImageLabel.get());

    // Connect QTimer for background map updating to lambda function
    connect(&bgTimer, &QTimer::timeout, [this]()
    {
        bgImageLabel->setPixmap(QPixmap::fromImage(*bgImage));
    });
}

void VRAMWindow::initBackgroundMapImage()
{
    // Only create QImage once
    if (bgImage || !gpu || !bgImageLabel)
    {
        return;
    }

    initColorTable();

    // Create QImage from raw data
    bgImage = std::make_unique<QImage>((unsigned char *)gpu->bg_frame.data(), 256, 256, QImage::Format_RGBA8888);

    // Set QImage to QLabel
    bgImageLabel->setPixmap(QPixmap::fromImage(*bgImage));

    // Start QTimer to update/refresh QLabel
    bgTimer.start((1.0 / SCREEN_FRAMERATE) * 1000);
}

void VRAMWindow::updateTileViews()
{
    std::vector<std::vector<std::vector<Tile>>> & tiles = gpu->getBGTiles();
    initColorTable();

    for (int i = 0; i < tiles.size(); i++)
    {
        auto & tileSet = tiles[i];
        auto & tileLabelSet = tileViews[i];
        int tileCounter = 0;
        for (auto & tileBlock : tileSet)
        {
            for (auto & tile : tileBlock)
            {
                // Create QImage from raw pixel data
                QImage image((unsigned char *)(tile.pixels.data()), 8, 8, QImage::Format::Format_Indexed8);

                // Set QImage's color table
                if (gpu->is_color_gb)
                {
                    image.setColorTable(colorTables[tile.getCGBBGPaletteNum()]);
                }
                else
                {
                    image.setColorTable(colorTables[0]);
                }

                // Scale image
                image = image.scaled(8 * 2, 8 * 2, Qt::KeepAspectRatio);

                // Update QLabel's QImage
                tileLabelSet[tileCounter]->setPixmap(QPixmap::fromImage(image));
                tileCounter++;
            }
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