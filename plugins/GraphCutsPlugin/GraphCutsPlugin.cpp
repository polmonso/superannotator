#include "GraphCutsPlugin.h"
#include "graphCut.h"
#include "gaussianFilter.cxx"

#include <vector>

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

void exportTIFCube(uchar* rawData,
                   const char* filename,
                   int cubeDepth,
                   int cubeHeight,
                   int cubeWidth)
{
  // import data to an itk image
  const int dimension = 3;
  typedef uchar TInputPixelType;
  typedef itk::Image< TInputPixelType, dimension > InputImageType;
  typedef itk::Image< TInputPixelType, dimension > OutputImageType;
  typedef itk::ImportImageFilter< TInputPixelType, dimension > ImportFilterType;
  ImportFilterType::Pointer importFilter = ImportFilterType::New();

  ImportFilterType::SizeType size;
  size[0] = cubeWidth;
  size[1] = cubeHeight;
  size[2] = cubeDepth;

  ImportFilterType::IndexType start;
  start.Fill(0);

  ImportFilterType::RegionType region;
  region.SetIndex(start);
  region.SetSize(  size  );

  importFilter->SetRegion( region );

  InputImageType::PointType origin;
  origin.Fill(0.0);

  importFilter->SetOrigin( origin );

  ImportFilterType::SpacingType spacing;
  spacing.Fill(1.0);

  importFilter->SetSpacing( spacing );
  importFilter->SetImportPointer(rawData, 0, false);

  stringstream sout;
  sout << filename;
  int n = strlen(filename);
  if(n < 4 || strcmp(filename+n-4,".tif")!=0)
     sout << ".tif";
  //printf("[Utils] Writing output cube %s\n", sout.str().c_str());
  typedef itk::ImageFileWriter< OutputImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName(sout.str().c_str());
  writer->SetInput(importFilter->GetOutput());
  writer->Update();
}

void cubeFloat2Uchar(float* inputData, uchar*& outputData,
                     int nx, int ny, int nz)
{
  float minValue = FLT_MAX; //9999999999;
  float maxValue = -1;
  int cubeIdx = 0;
  for(int z=0; z < nz; z++)
    for(int y=0; y < ny; y++)
      for(int x=0; x < nx; x++)
        {
          if(maxValue < inputData[cubeIdx])
              maxValue = inputData[cubeIdx];
          if(minValue > inputData[cubeIdx])
              minValue = inputData[cubeIdx];

          cubeIdx++;
        }

  printf("[util] cubeFloat2Uchar : min %f, max %f\n", minValue, maxValue);

  // allocate memory
  //MESSAGE("allocating memory\n");
  outputData = new uchar[nx*ny*nz];

  // copy to output cube
  float scale = 255.0f/(maxValue-minValue);
  cubeIdx = 0;
  for(int z=0; z < nz; z++)
    for(int y=0; y < ny; y++)
      for(int x=0; x < nx; x++)
        {
          outputData[cubeIdx] = (inputData[cubeIdx]-minValue)*scale;
          cubeIdx++;
        }
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

    if(sourcePoints.size() == 0 || sinkPoints.size() == 0) {
        return;
    }

    float sigma = 1;
    int seedRadius = 1;

    Matrix3D<PixelType>& volData = mPluginServices->getVolumeVoxelData();

    // get weight image
    float gaussianVariance = 1.0;
    float* foutputWeightImage = 0;
    gradientMagnitude<unsigned char, float>(volData.data(), volData.width(), volData.height(), volData.depth(), 1, gaussianVariance, foutputWeightImage);

    uchar* outputWeightImage = 0;
    cubeFloat2Uchar(foutputWeightImage,outputWeightImage,volData.width(), volData.height(), volData.depth());

    exportTIFCube(outputWeightImage,"outputWeightImage",volData.depth(),volData.height(),volData.width());

    const int volume_idx = 0;
    Matrix3D<OverlayType> &ovMatrix = mPluginServices->getOverlayVolumeData(volume_idx);

    // MUST BE RESIZED!
    ovMatrix.reallocSizeLike(volData);

    LabelType *dPtr = ovMatrix.data();
    ulong cubeSize = ovMatrix.numElem();
    for(ulong i = 0; i < cubeSize; i++) {
        dPtr[i] = outputWeightImage[i];
    }
    printf("Done\n");
    delete[] outputWeightImage;
    delete[] foutputWeightImage;

    // set enabled
    mPluginServices->setOverlayVisible( volume_idx, true );

    mPluginServices->updateDisplay();

    Cube cGCWeight;
    cGCWeight.width = volData.width();
    cGCWeight.height = volData.height();
    cGCWeight.depth = volData.depth();
    cGCWeight.data = outputWeightImage;
    cGCWeight.wh = volData.width()*volData.height();
    GraphCut g;
    g.extractSubCube(volData.data(),
                   cGCWeight.data,
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
    //ulong cubeSize = volData.numElem();
    output_data1d = new uchar[cubeSize];
    //memcpy(output_data1d,pOriginalImage->getRawData(),cubeSize);
    g.applyCut(&originalCube,output_data1d);

    exportTIFCube(output_data1d,"output_data1d",volData.depth(),volData.height(),volData.width());

#endif


}
