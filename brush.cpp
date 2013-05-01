#include "brush.h"


CubeBrush::CubeBrush(){
    width = height = depth = 10;
}

CubeBrush::CubeBrush(int width,int height, int depth)
{
    this->width = width;
    this->height = height;
    this->depth = depth;
}

void CubeBrush::paint(Matrix3D<LabelType> &data,
                        int x, int y, int z,
                        LabelType label)
{
    for(int i = x-width;i<x+width;i++) {
        for(int j = y-height; j<y+height; j++) {
            for(int k = z-depth; k<z+depth; k++)
            {
                //TODO maybe skip rows by shifting i=0,j=0...
                if (i < 0 || j < 0 || k < 0)
                    continue;
                if (i >= data.width() || j >= data.height()
                  ||k >= data.depth())
                    continue;

                 data.set(i, j, k, label);
            }
        }
    }
}

