#ifndef MATRIX3D_H
#define MATRIX3D_H

#include <itkImage.h>
#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>


template<typename T>
class Matrix3D
{
private:    //disable copy operator, at least for now
    Matrix3D( const Matrix3D& rhs );
    Matrix3D& operator=( const Matrix3D& rhs );

    typedef itk::Image<T, 3> ItkImageType;

public:
    typedef T DataType;
    Matrix3D() { mData = 0; mWidth = mHeight = mDepth = 0; updateCache(); mKeepOnDestr = false; }

    // empty, just garbage data
    Matrix3D( unsigned int w, unsigned int h, unsigned int d ) {
        mData = 0; mKeepOnDestr = false;
        realloc(w,h,d);
    }

    // generates a cropped region in the destination matrix
    inline void cropRegion( unsigned int sX, unsigned int sY, unsigned int sZ,
                            unsigned int w, unsigned int h, unsigned int d,
                            Matrix3D<T> *dest) const
    {
        dest->realloc( w, h, d );

        unsigned int endZ = sZ + d;
        unsigned int endY = sY + h;
        unsigned int stXY = sX;
        unsigned int edXY = stXY + w;

        unsigned int szCount = 0;
        for(; sZ < endZ; sZ++ )
        {
            const T * srcZ = sliceData(sZ);
            unsigned int yCount = 0;
            for (unsigned int y=sY; y < endY; y++)
            {
                std::copy( srcZ + stXY + y*mWidth, srcZ + y*mWidth + edXY, dest->sliceData(szCount) + w*yCount );
                yCount++;
            }

            szCount++;
        }
    }

    // empty, just garbage data
    inline void realloc( unsigned int w, unsigned int h, unsigned int d ) {
        freeData();

        mWidth = w;
        mHeight = h;
        mDepth = d;

        allocateEmpty();
        mKeepOnDestr = false;
        updateCache();
    }

    // sets every element to have value 'val'
    inline void fill( T val ) {
        for (unsigned int i=0; i < mNumElem; i++)
            mData[i] = val;
    }

    // calls realloc, copying the size from matrix m
    template<typename K>
    inline void reallocSizeLike( const Matrix3D<K> &m ) {
        realloc( m.width(), m.height(), m.depth() );
    }

    // existing pointer
    Matrix3D( T *data, unsigned int w, unsigned int h, unsigned int d ) {
        mWidth = w;
        mHeight = h;
        mDepth = d;
        mData = data;

        mKeepOnDestr = true;
        updateCache();
    }

    inline void updateCache() { mNumElem = mWidth*mHeight*mDepth; mSz = mWidth*mHeight; }

    inline T *data() { return mData; }
    inline const T *data() const { return mData; }

    inline unsigned int coordToIdx( unsigned int x, unsigned int y, unsigned int z ) const
    {
        return x + y*mWidth + z*mSz;
    }

    inline T& operator () (unsigned int x, unsigned int y, unsigned int z) {
        return mData[coordToIdx(x,y,z)];
    }

    inline const T& operator () (unsigned int x, unsigned int y, unsigned int z) const {
        return mData[coordToIdx(x,y,z)];
    }

    inline bool isEmpty() const { return mData == 0; }
    inline unsigned int numElem() const { return mNumElem; }

    inline void freeData() {
        //qDebug("Free");
        if ((mData != 0) && (mKeepOnDestr == false)) {
            //qDebug("Free2");
            delete[] mData;

            mData = 0;
        }
    }


    inline unsigned int    width() const { return mWidth; }
    inline unsigned int    height() const { return mHeight; }
    inline unsigned int    depth() const { return mDepth; }

    ~Matrix3D()
    {
        freeData();
    }

    static bool getFileDimensions( const std::string &fName, unsigned int &w, unsigned int &h, unsigned int &d )
    {
        try
        {
            typename itk::ImageFileReader<ItkImageType>::Pointer reader = itk::ImageFileReader<ItkImageType>::New();
            reader->SetFileName( fName );
            //reader->Update();

            typename ItkImageType::Pointer img = reader->GetOutput();

            typename ItkImageType::SizeType imSize = img->GetLargestPossibleRegion().GetSize();

            w = imSize[0];
            h = imSize[1];
            d = imSize[2];
        }
        catch(std::exception &e)
        {
            return false;
        }

        return true;
    }

    bool load( const std::string &fName )  // load from file
    {
        try
        {
            typename itk::ImageFileReader<ItkImageType>::Pointer reader = itk::ImageFileReader<ItkImageType>::New();
            reader->SetFileName( fName );
            reader->Update();

            typename ItkImageType::Pointer img = reader->GetOutput();

            typename ItkImageType::IndexType index;
            index[0] = index[1] = index[2] = 0;

            mData = &img->GetPixel( index );

            img->Register();    //so it won't delete the data ;)

            typename ItkImageType::SizeType imSize = img->GetLargestPossibleRegion().GetSize();
            mWidth = imSize[0];
            mHeight = imSize[1];
            mDepth = imSize[2];

            updateCache();
            mKeepOnDestr = true;
        }
        catch(std::exception &e)
        {
            return false;
        }

        return true;
    }

    bool save( const std::string &fName )
    {
        typedef itk::Image<T, 3> ItkImageType;

        try
        {
            typename ItkImageType::Pointer itkImg = ItkImageType::New();

            typename ItkImageType::PixelContainer::Pointer pixContainer = ItkImageType::PixelContainer::New();
            pixContainer->SetImportPointer( mData, mNumElem );

            itkImg->SetPixelContainer( pixContainer );

            typename ItkImageType::SizeType imSize;
            imSize[0] = mWidth;
            imSize[1] = mHeight;
            imSize[2] = mDepth;

            itkImg->SetRegions(imSize);


            typename itk::ImageFileWriter<ItkImageType>::Pointer writer = itk::ImageFileWriter<ItkImageType>::New();
            writer->SetFileName(fName);
            writer->SetInput(itkImg);

            writer->Update();
        }
        catch( std::exception &e )
        {
            return false;
        }

        return true;
    }

    // this is for a given Z-slice
    inline const T *sliceData(unsigned int z) const
    {
        if (isEmpty())
            qFatal("Tried to get slice from empty ImageVolume");

        if (z >= mDepth)
            z = mDepth-1;

        return mData + z*mWidth*mHeight;
    }

    inline T *sliceData(unsigned int z)
    {
        if (isEmpty())
            qFatal("Tried to get slice from empty ImageVolume");

        if (z >= mDepth)
            z = mDepth-1;

        return mData + z*mWidth*mHeight;
    }

    // sliceCoord 0..2
    inline void QImageSlice( unsigned int z, QImage &qimg ) const
    {
        qimg = QImage( width(), height(), QImage::Format_RGB32 );
        unsigned int sz = width()*height();

        unsigned int *dataPtr = (unsigned int *) qimg.constBits(); //trick!

        const unsigned char *p = sliceData(z);

        for (unsigned int i=0; i < sz; i++) {
            unsigned int D = p[i];
            dataPtr[i] = D | (D<<8) | (D<<16) | (0xFF<<24);
        }
    }

    // if all elemenst are equal
    bool operator ==(const Matrix3D<T>& b) const
    {
        for (unsigned int i=0; i < mNumElem; i++)
        {
            if ( b.data()[i] != data()[i] ){
                qDebug(" != at %d", (int)i );
                return false;
            }
        }

        return true;
    }

private:
    T *mData;
    unsigned int mWidth, mHeight, mDepth;
    unsigned int mSz; // this is a cached version of mWidth*mHeight
    unsigned int mNumElem;
    bool    mKeepOnDestr;  // if data should not be deleted upon object destruction

    void allocateEmpty() {
        mSz = mWidth*mHeight;
        mNumElem = mSz * mDepth;

        mData = new T[mNumElem];
    }
};


#endif // MATRIX3D_H
