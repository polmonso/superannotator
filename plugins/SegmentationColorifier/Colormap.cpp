#include "Colormap.h"

#define COLORMAP_SIZE 64

static Colormap::Scalar colormapLines[3 * COLORMAP_SIZE] =
{
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,    0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255,
    0,   128,     0,
  255,     0,     0,
    0,   191,   191,
  191,     0,   191,
  191,   191,     0,
   64,    64,    64,
    0,     0,   255
};


Colormap::Colormap( ColormapType type )
{
    mType = type;
}

void Colormap::get( std::vector<itkRGBPixel> &list )
{
    list.clear();
    list.resize( COLORMAP_SIZE );

    for (unsigned i=0; i < COLORMAP_SIZE; i++)
    {
        unsigned char R = colormapLines[ 3*i + 0 ];
        unsigned char G = colormapLines[ 3*i + 1 ];
        unsigned char B = colormapLines[ 3*i + 2 ];
        list[i].Set( R, G, B );
    }
}
