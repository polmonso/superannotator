#ifndef SUPERVOXELER_H
#define SUPERVOXELER_H

#define fatalMsg(x) qFatal(x)
#include <slic/LKM.h>
#include <slic/mex/include/create_slicmap.hxx>
#undef fatalMsg

#include "Matrix3D.h"
/**
 ** Class to ease the task of computing and using supervoxels
 */
template<typename T>    // T is the input data type
class SuperVoxeler
{
public:
    typedef unsigned int    IDType; // supervoxel ID type

private:
    bool mIsEmpty;

    Matrix3D<IDType>    mPixelToVoxel;  // pixel to voxel ID 'table'
    SlicMapType         mVoxelToPixel;  // voxel ID to pixel 'table'

    unsigned int mNumLabels; //number of supervoxels


public:
    SuperVoxeler() { mIsEmpty = true; mNumLabels = 0; }

    unsigned int numLabels() const { return mNumLabels; }

    void apply( const Matrix3D<T> &img, int step, unsigned int cubeness )
    {
        LKM* lkm = new LKM;

        sidType**   kLabels;
        int numLabels;

        /** This is a waste of memory, so it should be changed inside the supervoxel routines **/
        Matrix3D<double> dblMatrix( img.width(), img.height(), img.depth() );
        for (unsigned int i=0; i < img.numElem(); i++)
            dblMatrix.data()[i] = img.data()[i];    // stupid copy



        lkm->DoSupervoxelSegmentationForGrayVolume(dblMatrix.data(), img.width(), img.height(), img.depth(), kLabels, numLabels, step, cubeness);

        qDebug("Num labels: %d", (int)numLabels);
        //qDebug("Num elem: %u", img.numElem());


        // we can free the double matrix
        dblMatrix.freeData();

        // now another waste.. copy labels back to a normal array
        mPixelToVoxel.realloc( img.width(), img.height(), img.depth() );

        //qDebug("Size: %d %d %d", mPixelToVoxel.width(), mPixelToVoxel.height(), mPixelToVoxel.depth());


        unsigned int sz = img.width() * img.height();
        qDebug("Sz: %d", (int)sz);
        for (unsigned int z=0; z < img.depth(); z++)
        {
            unsigned int zOff = z * sz;
            memcpy( mPixelToVoxel.data() + zOff, kLabels[z], sz*sizeof(unsigned int) );
        }

        // free kLabels
        for (int z=0; z < img.depth(); z++)
            delete[] kLabels[z];
        delete[] kLabels;

        delete lkm; // free lkm itself

        mNumLabels = numLabels;


        /** Compute the inverse map **/
        qDebug("Computing slic map");
        createSlicMap( mPixelToVoxel, numLabels, mVoxelToPixel );
    }


    const Matrix3D<IDType> & pixelToVoxel() const { return mPixelToVoxel; }
    const SlicMapType & voxelToPixel() const { return mVoxelToPixel; }
};

#endif // SUPERVOXELER_H
