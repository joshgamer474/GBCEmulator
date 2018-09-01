#ifndef HEX_WIDGET_H
#define HEX_WIDGET_H

#include <QAbstractScrollArea>

#define BYTES_PER_LINE 16
#define PIXEL_SPACE 4

class HexWidget : public QAbstractScrollArea
{
public:
    explicit HexWidget(QWidget * parent = 0);
    virtual ~HexWidget();

    void clear();
    void setData(const std::vector<uint8_t> & bytes);
    QSize getFullWidgetSize();
    void paintEvent(QPaintEvent *);
    void setCursor(size_t pc_pos);
    void updateScrollBarLine(size_t pc_pos);

private:
    QByteArray data;
    size_t cursor_pos;
    int byte_width, byte_height;
    bool data_updated;
};

#endif