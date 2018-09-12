#include <hexwidget.h>
#include <QPainter>
#include <QScrollBar>
#include <QPaintEvent>
#include <QPalette>

HexWidget::HexWidget(QWidget * parent)
    : QAbstractScrollArea(parent),
    data_updated(false)
{
    setFont(QFont("Courier", 8));
    byte_width = fontMetrics().width("QQ", 2);
    byte_height = fontMetrics().height();

    setFocusPolicy(Qt::StrongFocus);
}

HexWidget::~HexWidget()
{

}

void HexWidget::clear()
{
    data.clear();
    viewport()->update();
}

QSize HexWidget::getFullWidgetSize()
{
    size_t width = BYTES_PER_LINE * PIXEL_SPACE * byte_width;
    size_t height = (std::ceil(data.size() / BYTES_PER_LINE) + 3)* byte_height;
    return QSize(width, height);
}

void HexWidget::setData(const std::vector<uint8_t> & bytes)
{
    data = QByteArray(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    cursor_pos = 0;
    data_updated = true;
}

void HexWidget::updateData(const std::vector<uint8_t> & partial_bytes, uint16_t start_pos, uint16_t end_pos)
{
    QByteArray partialData = QByteArray(reinterpret_cast<const char*>(partial_bytes.data()), partial_bytes.size());
    data.replace(start_pos, end_pos - start_pos, partialData);
    data_updated = true;
}

void HexWidget::setCursor(size_t pos)
{
    cursor_pos = pos;
    updateScrollBarLine(pos);
    viewport()->update();
}

void HexWidget::updateScrollBarLine(size_t pos)
{
    size_t scroll_bar_line = pos / 16;

    // Give some scroll line buffer so the cursor isn't always in the top line
    scroll_bar_line -= 2;

    verticalScrollBar()->setValue(scroll_bar_line);
}

void HexWidget::paintEvent(QPaintEvent * e)
{
    QString number;
    size_t counter = 0;
    size_t draw_x, draw_y;
    size_t hex_offset_column_x;

    if (data.size() == 0)
    {
        return;
    }

    draw_x = draw_y = 0;
    hex_offset_column_x = (byte_width * 2) + PIXEL_SPACE;

    QPainter p(viewport());
    QRect eRect = e->rect();

    // Get which line the viewport is showing as its top line, calculate bottom line index
    int topLineIndex    = getTopLineIndex();
    int bottomLineIndex = getBottomLineIndex();

    // Draw Hex Offset header at top of viewport (00, 01, 02, 03, ..., 0F)
    p.fillRect(hex_offset_column_x, 0, (byte_width + PIXEL_SPACE) * BYTES_PER_LINE, byte_height, Qt::darkGray);
    draw_x = hex_offset_column_x;
    draw_y += byte_height;
    for (uint8_t i = 0; i <= 0x0F; i++)
    {
        number = QString::number(i, 16).rightJustified(2, '0').toUpper();
        p.drawText(draw_x, draw_y * 0.75, number);
        draw_x += byte_width + PIXEL_SPACE;
    }

    // Get data[startPos] and data[endPos]
    int startCharIndex = topLineIndex * BYTES_PER_LINE;
    int stopCharIndex = bottomLineIndex * BYTES_PER_LINE;

    if (stopCharIndex > data.size())
    {
        stopCharIndex = data.size();
    }

    // Draw data viewed in viewport
    for (int i = startCharIndex; i <= stopCharIndex; i++)
    {
        const uint8_t & byte = data[i];

        // Check if we should draw on the next line
        if (counter % BYTES_PER_LINE == 0)
        {
            draw_x = hex_offset_column_x;
            draw_y += byte_height;

            // Draw Hex Offset to the left of viewport (0000, 0010, 0020, ...)
            p.fillRect(0, draw_y - (0.75 * byte_height), (2 * byte_width) + (0.5 * PIXEL_SPACE), byte_height, Qt::darkGray);
            number = QString::number(static_cast<uint16_t>(i), 16).rightJustified(4, '0').toUpper();
            p.drawText(0, draw_y, number);
        }

        // Highlight character if cursor position
        if (i == cursor_pos)
        {
            p.fillRect(draw_x, draw_y - (0.75 * byte_height), byte_width, byte_height, Qt::yellow);
        }

        number = QString::number(byte, 16).rightJustified(2, '0').toUpper();
        p.drawText(draw_x, draw_y, number);

        draw_x += byte_width + PIXEL_SPACE;
        counter++;
    }
}

int HexWidget::getTopLineIndex()
{
    return verticalScrollBar()->value();
}

int HexWidget::getBottomLineIndex()
{
    // Get shown viewport size and fullscreen size if everything were displayed
    QSize shownSize = viewport()->size();
    QSize fullSize = getFullWidgetSize();

    // Setup scroll step size and scroll bar's range
    verticalScrollBar()->setPageStep(shownSize.height() / byte_height);
    verticalScrollBar()->setRange(0, (fullSize.height() - shownSize.height()) / byte_height);

    // Get which line the viewport is showing as its top line, calculate bottom line index
    int topLineIndex = getTopLineIndex();
    int bottomLineIndex = topLineIndex + (shownSize.height() / byte_height);

    return bottomLineIndex;
}