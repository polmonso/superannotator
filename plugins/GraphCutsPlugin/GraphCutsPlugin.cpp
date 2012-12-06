#include "GraphCutsPlugin.h"
#include "graphCut.h"
#include "gaussianFilter.cxx"

#include <vector>

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
                    sinkPoints.push_back(p);
                }
            }
        }
    }


    float sigma = 1;
    int seedRadius = 1;

    Matrix3D<PixelType>& volData = mPluginServices->getVolumeVoxelData();

    // get weight image
    float gaussianVariance = 1.0;
    uchar* outputWeightImage = 0;
    gradientMagnitude(volData.data(), volData.width(), volData.height(), volData.depth(), 1, gaussianVariance, outputWeightImage);

    Cube cGCWeight;
    cGCWeight.width = volData.width();
    cGCWeight.height = volData.height();
    cGCWeight.depth = volData.depth();
    cGCWeight.data = outputWeightImage;
    cGCWeight.wh = volData.width()*volData.height();
    GraphCut g;
    g.extractSubCube(volData.data(),
                   GCWeight_data,
                   sourcePoints,sinkPoints,
                   cGCWeight.width,cGCWeight.height,cGCWeight.depth);
    g.run_maxflow(&cGCWeight,sourcePoints,sinkPoints,sigma,seedRadius);

    unsigned char* output_data1d = 0;
    Cube originalCube;
    originalCube.width = volData.width();
    originalCube.height = volData.height();
    originalCube.depth = volData.depth();
    originalCube.data = volData.data();
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
    ulong cubeSize = volData.numElem();
    output_data1d = new uchar[cubeSize];
    memcpy(output_data1d,pOriginalImage->getRawData(),cubeSize);
    g.applyCut(&originalCube,output_data1d);
#endif



}
