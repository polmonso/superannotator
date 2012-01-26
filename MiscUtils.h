#ifndef MISCUTILS_H
#define MISCUTILS_H

#include <QColor>

// takes a slice of total size numElem in contiguous memory
//  and creates a mask with an overlay, according to the given color
// destImg can be equal to baseImg
static inline void overlayRGB( const unsigned int *baseImg, const unsigned char *overlayImg, unsigned int *destImg, unsigned int numElem, const QColor &color )
{
    qreal sR, sG, sB;
    color.getRgbF( &sR, &sG,&sB );
    sR *= 255;
    sG *= 255;
    sB *= 255;

    const qreal div255 = 1.0 / 255.0;

    for (unsigned int i=0; i < numElem; i++)
    {
        qreal r = (baseImg[i] >> 16) & 0xFF;
        qreal g = (baseImg[i] >> 8) & 0xFF;
        qreal b = (baseImg[i] >> 0) & 0xFF;

        qreal sc = overlayImg[i] * div255;
        qreal scInv = 1.0 - sc;

        g = scInv*g + sc*sG;
        b = scInv*b + sc*sB;
        r = r * scInv +  sc*sR;

        unsigned int iR = r;
        unsigned int iG = g;
        unsigned int iB = b;

        //pixPtr[i] = (pixPtr[i] & 0xFF00FFFF) | (((unsigned int) scorePtr[i]) << 16);
        destImg[i] = 0xFF000000 | (iR<<16) | (iG<<8) | iB;
    }
}

#endif // MISCUTILS_H
