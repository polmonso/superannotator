#ifndef COLORLISTS_H
#define COLORLISTS_H

#include <vector>
#include <QColor>
#include <QIcon>

struct LabelColorList
{
    std::vector<QColor> colorList;

    std::vector<QIcon> iconList;

    inline LabelColorList()
    {
        colorList.push_back( QColor(Qt::magenta) );
        colorList.push_back( QColor(Qt::green) );
        colorList.push_back( QColor(Qt::blue) );

        // create icons
        const int iconSize = 32;
        for (int i=0; i < (int)colorList.size(); i++ )
        {
            QPixmap icon( iconSize, iconSize );
            icon.fill( colorList[i] );

            iconList.push_back( icon );
        }
    }
};

struct OverlayColorList
{
    std::vector<QColor> colorList;

    inline OverlayColorList()
    {
        colorList.push_back( QColor(0xFF, 0xC2, 0x0B) );
        colorList.push_back( QColor(0xFF, 0x0B, 0xFF) );
        colorList.push_back( QColor(0xFF, 0xFF, 0x00) );
    }
};

class SelectionColor : public QColor
{
public:
    SelectionColor() : QColor(0x6A, 0x00, 0xD5) { }
};

class ScoreColor : public QColor
{
public:
    ScoreColor() : QColor(0xFF, 0x00, 0x00) { }
};


#endif // COLORLISTS_H
