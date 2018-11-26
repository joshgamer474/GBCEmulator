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

    whiteColorTable = QVector<QRgb>(4, 0xFFFFFFFF);

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
        colorTables.resize(16);

        // Initialize background and sprite tile colors for CGB
        for (int i = 0; i < gpu->cgb_background_palettes.size(); i++)
        {   // Get color palette
            auto & bg_palette       = gpu->cgb_background_palettes[i];
            auto & sprite_palette   = gpu->cgb_sprite_palettes[i];

            colorTables[i].resize(CGB_NUM_COLORS_PER_PALETTE);
            colorTables[i + CGB_PALETTE_DATA_SIZE].resize(CGB_NUM_COLORS_PER_PALETTE);

            for (int j = 0; j < CGB_NUM_COLORS_PER_PALETTE; j++)
            {   // Get color from palette
                const auto & bg_color       = bg_palette.getColor(j);
                const auto & sprite_color   = sprite_palette.getColor(j);

                colorTables[i][j] = qRgb(bg_color.r, bg_color.g, bg_color.b);
                colorTables[i + CGB_PALETTE_DATA_SIZE][j] = qRgb(sprite_color.r, sprite_color.g, sprite_color.b);
            }
        }
    }
    else
    {   // Initialize background tile color
        colorTables.resize(1);
        colorTables[0].resize(PALETTE_DATA_SIZE);
        for (const auto & item : gpu->bg_palette_color)
        {
            colorTables[0].push_back(qRgb(item.r, item.g, item.b));
        }

        // Initialize sprite tile colors?
    }
}

void VRAMWindow::updateColorTable()
{
    if (!gpu)
    {
        return;
    }

    if (gpu->is_color_gb)
    {   // Update CGB background and sprite tile colors
        for (int i = 0; i < gpu->cgb_background_palettes.size(); i++)
        {   // Get color palette
            auto & bg_palette = gpu->cgb_background_palettes[i];
            auto & sprite_palette = gpu->cgb_sprite_palettes[i];

            for (int j = 0; j < CGB_NUM_COLORS_PER_PALETTE; j++)
            {   // Get color from palette
                const auto & bg_color       = bg_palette.getColor(j);
                const auto & sprite_color   = sprite_palette.getColor(j);

                // Get current QRgb
                auto & bg_qrgb      = colorTables[i][j];
                auto & sprite_qrgb  = colorTables[i + CGB_PALETTE_DATA_SIZE][j];

                // Update QRgbs
                updateQRgb(bg_qrgb, bg_color);
                updateQRgb(sprite_qrgb, sprite_color);
            }
        }
    }
    else
    {   // Update background tile color
        for (int i = 0; i < PALETTE_DATA_SIZE; i++)
        {   // Get current QRgb
            auto & qrgb = colorTables[0][i];

            // Get color from palette
            const auto & color = gpu->bg_palette_color[i];
            
            // Update QRgb
            updateQRgb(qrgb, color);
        }
    }
}

void VRAMWindow::updateQRgb(QRgb & qrgb, const SDL_Color & color)
{
    qrgb = color.a;
    qrgb = (qrgb << 8) | color.r;
    qrgb = (qrgb << 8) | color.g;
    qrgb = (qrgb << 8) | color.b;
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
    updateColorTable();

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
                    image.setColorTable(getColorTableFromPtr(tile.getCGBColorPalette()));
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

QVector<QRgb> VRAMWindow::getColorTableFromPtr(ColorPalette * color_palette)
{
    if (color_palette == NULL)
    {
        return whiteColorTable;
    }

    const auto & color_palette_raw_data = color_palette->getRawData();
    
    for (int i = 0; i < gpu->cgb_background_palettes.size(); i++)
    {
        if (color_palette_raw_data == gpu->cgb_background_palettes[i].getRawData())
        {
            return colorTables[i];
        }
    }

    for (int i = 0; i < gpu->cgb_sprite_palettes.size(); i++)
    {
        if (color_palette_raw_data == gpu->cgb_sprite_palettes[i].getRawData())
        {
            return colorTables[i + CGB_PALETTE_DATA_SIZE];
        }
    }

    return whiteColorTable;
}