#include "GraphCutsPlugin.h"
#include "graphCut.h"
#include "gaussianFilter.cxx"

#include <vector>
#include "utils.h"


#include "itkImage.h"
#include "itkImageFileWriter.h"
#include "itkImageFileReader.h"
#include "itkImportImageFilter.h"
#include "itkConnectedComponentFunctorImageFilter.h"
#include "itkLabelObject.h"
#include "itkLabelMap.h"
#include "itkLabelImageToShapeLabelMapFilter.h"
#include "itkShapeLabelObject.h"
#include "itkLabelMapToBinaryImageFilter.h"

extern "C" Q_DECL_EXPORT PluginBase* createPlugin();

PluginBase *createPlugin()
{
    return new GraphCutsPlugin();
}

void GraphCutsPlugin::runGraphCuts()
{
    // generate list of seeds
    Matrix3D<ScoreType> &scoreMatrix = mPluginServices->getOverlayVolumeData(0);
    std::vector<Point> sinkPoints;
    std::vector<Point> sourcePoints;

    for(int x = 0; x < scoreMatrix.width(); ++x) {
        for(int y = 0; y < scoreMatrix.height(); ++y) {
            for(int z = 0; z < scoreMatrix.depth(); ++z) {
                Point p;
                p.x = x;
                p.y = y;
                p.z = z;
                if(scoreMatrix(x,y,z) == 255) {
                    sourcePoints.push_back(p);
                } else {
                    if(scoreMatrix(x,y,z) == 128) {
                       sinkPoints.push_back(p);
                    }
                }
            }
        }
    }

    if(sourcePoints.size() == 0) {
        printf("[Main] Error: 0 source points\n");
        return;
    }
    if(sinkPoints.size() == 0) {
        printf("[Main] Error: 0 sink points\n");
        return;
    }

    float sigma = 100;
    int seedRadius = 3;

    Matrix3D<PixelType>& volData = mPluginServices->getVolumeVoxelData();
    ulong cubeSize = volData.numElem();

    // get weight image
    float gaussianVariance = 1.0;
    float* foutputWeightImage = 0;
    gradientMagnitude<unsigned char, float>(volData.data(), volData.width(), volData.height(), volData.depth(), 1, gaussianVariance, foutputWeightImage);

    uchar* outputWeightImage = 0;
    cubeFloat2Uchar(foutputWeightImage,outputWeightImage,volData.width(), volData.height(), volData.depth());
    exportTIFCube(outputWeightImage,"outputWeightImage",volData.depth(),volData.height(),volData.width());
    delete[] foutputWeightImage;

    const int idx_binary_cube = 1;
    Matrix3D<OverlayType> &binData = mPluginServices->getOverlayVolumeData(idx_binary_cube);

    exportTIFCube(binData.data(),"temp_binCube",volData.depth(),volData.height(),volData.width());

    const int idx_new_overlay = 2;
    Matrix3D<OverlayType> &ovMatrix = mPluginServices->getOverlayVolumeData(idx_new_overlay);

    // MUST BE RESIZED!
    ovMatrix.reallocSizeLike(volData);

    /*
    LabelType *dPtr = ovMatrix.data();
    ulong cubeSize = ovMatrix.numElem();
    for(ulong i = 0; i < cubeSize; i++) {
        dPtr[i] = outputWeightImage[i];
    }

    // set enabled
    mPluginServices->setOverlayVisible( idx_new_overlay, true );

    mPluginServices->updateDisplay();
    */

    Cube cGCWeight;
    cGCWeight.width = volData.width();
    cGCWeight.height = volData.height();
    cGCWeight.depth = volData.depth();
    cGCWeight.data = outputWeightImage;
    cGCWeight.wh = volData.width()*volData.height();
    GraphCut g;
    printf("[Main] Extracting sub-cube\n");
    g.extractSubCube(binData.data(),
                   cGCWeight.data,
                   sourcePoints,sinkPoints,
                   cGCWeight.width,cGCWeight.height,cGCWeight.depth);
    printf("[Main] Running max-flow with %ld sources and %ld sinks\n", sourcePoints.size(), sinkPoints.size());
    g.run_maxflow(&cGCWeight,sourcePoints,sinkPoints,sigma,seedRadius);

    unsigned char* output_data1d = 0;
    Cube originalCube;
    originalCube.width = volData.width();
    originalCube.height = volData.height();
    originalCube.depth = volData.depth();
    //originalCube.data = volData.data();
    originalCube.data = binData.data();
    originalCube.wh = originalCube.width*originalCube.height;

#if 0
    g.getOutput(&originalCube,output_data1d);

    ulong subCubeWidth;
    ulong subCubeHeight;
    ulong subCubeDepth;
    g.getCubeSize(subCubeWidth,
                subCubeHeight,
                subCubeDepth);



  pOutput4DImage.setData(output_data1d, subCubeWidth,subCubeHeight,subCubeDepth,
                         1, //p4DImage.getCDim(),
                         imageType);

#else
    //ulong cubeSize = volData.numElem();
    output_data1d = new uchar[cubeSize];
    //memcpy(output_data1d,pOriginalImage->getRawData(),cubeSize);
    memset(output_data1d,0,cubeSize);
    printf("Applying cut\n");
    g.applyCut(&originalCube,output_data1d);

    printf("Exporting cube to output_data1d\n");
    exportTIFCube(output_data1d,"output_data1d",volData.depth(),volData.height(),volData.width());

#endif

    LabelType *dPtr = ovMatrix.data();
    for(ulong i = 0; i < cubeSize; i++) {
        dPtr[i] = output_data1d[i];
    }

    // set enabled
    mPluginServices->setOverlayVisible( idx_new_overlay, true );
    mPluginServices->updateDisplay();

    printf("Cleaning\n");
    delete[] output_data1d;
    delete[] outputWeightImage;
    printf("Done\n");
}
