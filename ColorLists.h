#ifndef COLORLISTS_H
#define COLORLISTS_H

#include <vector>
#include <QColor>
#include <QIcon>

struct LabelColorList
{
    std::vector<int> hueList;
    std::vector<QIcon> iconList;

    inline LabelColorList()
    {
        hueList.push_back( QColor(Qt::blue).hsvHue() );
        hueList.push_back( QColor(Qt::green).hsvHue() );
        hueList.push_back( QColor(Qt::red).hsvHue() );

        // create icons
        const int iconSize = 32;
        for (int i=0; i < (int)hueList.size(); i++ )
        {
            QPixmap icon( iconSize, iconSize );
            icon.fill( QColor::fromHsv( hueList[i], 255, 255 ) );

            iconList.push_back( icon );
        }
    }


};

#endif // COLORLISTS_H
