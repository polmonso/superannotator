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
        colorList.push_back( QColor(Qt::red) );

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

#endif // COLORLISTS_H
