#ifndef BRUSH_H
#define BRUSH_H

#include "Matrix3D.h"
#include "CommonTypes.h"

class Brush
{
public:
    virtual void paint(Matrix3D<LabelType> &data, int x, int y, int z, LabelType label) = 0;
    virtual ~Brush() {}
};

class CubeBrush : public Brush
{
public:
    int width;
    int height;
    int depth;

    CubeBrush();

    CubeBrush(int width, int height, int depth);

    void paint(Matrix3D<LabelType> &data, int x, int y, int z, LabelType label);

};

#endif // BRUSH_H
