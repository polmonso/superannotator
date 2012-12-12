#include "GraphCutsPlugin.h"
#include "graphCut.h"
#include "gaussianFilter.cxx"

#include <QInputDialog>
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
    // ask for gaussian variance
    bool ok = false;
    float gaussianVariance = QInputDialog::getDouble(0, "Gaussian variance", "Specify the variance for the gaussian filter", 1.6, 0.6, 40.0f, 1, &ok);
    if (!ok) return;

    const float sigma = 100;
    const int seedRadius = 3;

    // generate list of seeds
    Matrix3D<ScoreType> &seedOverlay = mPluginServices->getOverlayVolumeData(0);
    std::vector<Point> sinkPoints;
    std::vector<Point> sourcePoints;

    for(int x = 0; x < seedOverlay.width(); ++x) {
        for(int y = 0; y < seedOverlay.height(); ++y) {
            for(int z = 0; z < seedOverlay.depth(); ++z) {
                Point p;
                p.x = x;
                p.y = y;
                p.z = z;
                if(seedOverlay(x,y,z) == 255) {
                    sourcePoints.push_back(p);
                } else {
                    if(seedOverlay(x,y,z) == 128) {
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

    Matrix3D<PixelType>& volData = mPluginServices->getVolumeVoxelData();
    ulong cubeSize = volData.numElem();

    // get weight image
    //float gaussianVariance = 1.0;
    float* foutputWeightImage = 0;
    gradientMagnitude<unsigned char, float>(volData.data(), volData.width(), volData.height(), volData.depth(), 1, gaussianVariance, foutputWeightImage);

    uchar* outputWeightImage = 0;
    cubeFloat2Uchar(foutputWeightImage,outputWeightImage,volData.width(), volData.height(), volData.depth());
    exportTIFCube(outputWeightImage,"outputWeightImage",volData.depth(),volData.height(),volData.width());
    delete[] foutputWeightImage;

    Matrix3D<OverlayType> &binData = mPluginServices->getOverlayVolumeData(idx_bindata_overlay);

    exportTIFCube(binData.data(),"temp_binCube",volData.depth(),volData.height(),volData.width());

    // copy weight image to overlay
    const int idx_weight_overlay = 2;
    Matrix3D<OverlayType> &weightMatrix = mPluginServices->getOverlayVolumeData(idx_weight_overlay);
    weightMatrix.reallocSizeLike(volData);
    LabelType *dPtr = weightMatrix.data();
    for(ulong i = 0; i < cubeSize; i++) {
        dPtr[i] = outputWeightImage[i];
    }

    // set enabled
    mPluginServices->setOverlayVisible( idx_weight_overlay, true );
    //mPluginServices->updateDisplay();

    //LabelImageType::Pointer labelInput = getLabelImage<TInputPixelType,LabelImageType>(inputData,nx,ny,nz);
    LabelImageType::Pointer labelInput = getLabelImage<uchar,LabelImageType>(binData.data(),binData.width(),binData.height(),binData.depth());

    /*
    LabelImageType* pLabelInput = labelInput.GetPointer();

    // copy label image to overlay
    const int idx_label_overlay = 4;
    Matrix3D<OverlayType> &labelMatrix = mPluginServices->getOverlayVolumeData(idx_label_overlay);
    labelMatrix.reallocSizeLike(volData);
    dPtr = labelMatrix.data();
    for(ulong i = 0; i < cubeSize; i++) {
        dPtr[i] = pLabelInput[i];
    }
    mPluginServices->setOverlayVisible( idx_label_overlay, true );
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
                     labelInput,
                     sourcePoints,sinkPoints,
                     cGCWeight.width,cGCWeight.height,cGCWeight.depth);
    printf("[Main] Running max-flow with %ld sources and %ld sinks\n", sourcePoints.size(), sinkPoints.size());
    g.run_maxflow(&cGCWeight,sourcePoints,sinkPoints,sigma,seedRadius);

    unsigned char* output_data1d = 0;
    Cube originalCube;
    originalCube.width = volData.width();
    originalCube.height = volData.height();
    originalCube.depth = volData.depth();
    originalCube.data = binData.data();
    originalCube.wh = originalCube.width*originalCube.height;


    output_data1d = new uchar[cubeSize];
    //memcpy(output_data1d,pOriginalImage->getRawData(),cubeSize);
    memcpy(output_data1d,binData.data(),cubeSize);
    //memset(output_data1d,0,cubeSize);
    printf("Applying cut\n");
    g.getOutput(&originalCube,output_data1d);
    g.applyCut(&originalCube,output_data1d);

    printf("Exporting cube to output_data1d\n");
    exportTIFCube(output_data1d,"output_data1d",volData.depth(),volData.height(),volData.width());

    // copy output to a new overlay    
    Matrix3D<OverlayType> &ovMatrix = mPluginServices->getOverlayVolumeData(idx_output_overlay);
    ovMatrix.reallocSizeLike(volData);
    //LabelType *dPtr = ovMatrix.data();
    dPtr = ovMatrix.data();
    for(ulong i = 0; i < cubeSize; i++) {
        dPtr[i] = output_data1d[i];
    }

    // set enabled
    mPluginServices->setOverlayVisible( idx_output_overlay, true );
    mPluginServices->updateDisplay();

    printf("Cleaning\n");
    delete[] output_data1d;
    delete[] outputWeightImage;
    printf("Done\n");
}

void GraphCutsPlugin::cleanSeedOverlay()
{
    Matrix3D<ScoreType> &seedOverlay = mPluginServices->getOverlayVolumeData(0);

    if (seedOverlay.isEmpty())
    {
        seedOverlay.reallocSizeLike( mPluginServices->getVolumeVoxelData() );
    }

    seedOverlay.fill(0);
    mPluginServices->setOverlayVisible( 0, true );
    mPluginServices->updateDisplay();
}

void GraphCutsPlugin::transferOverlay()
{
    Matrix3D<ScoreType> &outputOverlay = mPluginServices->getOverlayVolumeData(idx_output_overlay);

    if (!outputOverlay.isEmpty())
    {
        Matrix3D<ScoreType> &inputOverlay = mPluginServices->getOverlayVolumeData(idx_bindata_overlay);
        inputOverlay.copyFrom(outputOverlay);
        mPluginServices->setOverlayVisible(idx_bindata_overlay, true );
        mPluginServices->updateDisplay();
    }
}
