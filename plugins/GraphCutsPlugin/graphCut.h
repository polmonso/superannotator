#ifndef GRAPHCUT_H_
#define GRAPHCUT_H_

// Include graph.h first to avoid compilation errors
// due to the re-definition of symbols
#include "kgraph.h"
#include "kgraph.cpp"
#include "maxflow.cpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <float.h>
//#include <cv.h>
//#include <highgui.h>

// ITK Header Files
#include "itkConnectedComponentFunctorImageFilter.h"
#include "itkImportImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkLabelObject.h"
#include "itkLabelMap.h"
#include "itkLabelImageToShapeLabelMapFilter.h"
#include "itkShapeLabelObject.h"
#include "itkLabelMapToBinaryImageFilter.h"


//-----------------------------------------------------------------------------

#define flag_verbose false
#define PRINT_MESSAGE(format, ...) if(flag_verbose) printf (format, ## __VA_ARGS__)

using namespace std;

typedef maxflow::Graph<float,float,float> GraphType;
typedef unsigned char uchar;

struct Point
{
    int x;
    int y;
    int z;
};

struct Cube
{
  ulong width;
  ulong height;
  ulong depth;
  ulong wh; //width*height
  unsigned char* data;

  //unsigned char at(int i, int j, int k)
  int at(int i, int j, int k)
  {
    return (int)data[k*wh+j*width+i];
  }

  ~Cube()
  {
    delete[] data;
  }
};

//-----------------------------------------------------------------------------

class GraphCut
{
 private:
  GraphType::node_id* m_node_ids;
  int ni,nj,nk;
  ulong nij;
  ulong subX, subY, subZ;
  Cube* subCube;
  //uchar* subCube;
  //ulong subWidth, subHeight, subDepth;

 public:

  GraphCut();
  ~GraphCut();

  void applyCut(Cube* inputCube, unsigned char* output_data);

  unsigned long at(int i, int j, int k);

  template <typename TInputPixelType>
    void extractSubCube(TInputPixelType* inputData,
                        TInputPixelType* weights,
                        vector<Point>& sourcePoints,
                        vector<Point>& sinkPoints,
                        long nx, long ny, long nz);

  template <typename TInputPixelType, typename LabelImageType>
    typename LabelImageType::Pointer getLabelImage(TInputPixelType* inputData,
                                                   long nx, long ny, long nz);

  void getCubeSize(ulong& cubeWidth,
                   ulong& cubeHeight,
                   ulong& cubeDepth);

  void getOutput(Cube* inputCube,uchar*& output_data);
 
  void run_maxflow(Cube* cube,
                   vector<Point>& sourcePoints, vector<Point>& sinkPoints,
                   float sigma = 100.0f, int minDist = 3);
 
  GraphType *m_graph;

  // true if the run_maxflow method is running
  bool running_maxflow;
};



template <typename TInputPixelType, typename LabelImageType>
typename LabelImageType::Pointer GraphCut::getLabelImage(TInputPixelType* inputData,
                                                         long nx, long ny, long nz)
{
  const unsigned int Dimension = 3;
  typedef itk::Image< TInputPixelType, Dimension > InputImageType;
  //typedef unsigned int TLabelPixelType;
  //typedef itk::Image< TLabelPixelType, Dimension > LabelImageType;

  typedef itk::ConnectedComponentImageFilter< InputImageType, LabelImageType > CCFilterType;
  typename CCFilterType::Pointer ccFilter;

  ccFilter = CCFilterType::New();

  // set parameters
  ccFilter->SetFullyConnected(true);
  ccFilter->SetBackgroundValue(0);

  // import data to an itk image
  typedef itk::ImportImageFilter< TInputPixelType, Dimension > ImportFilterType;
		
  typename ImportFilterType::Pointer importFilter = ImportFilterType::New();
		
  typename ImportFilterType::SizeType size;
  size[0] = nx;
  size[1] = ny;
  size[2] = nz;
		
  typename ImportFilterType::IndexType start;
  start.Fill( 0 );
		
  typename ImportFilterType::RegionType region;
  region.SetIndex( start );
  region.SetSize(  size  );
		
  importFilter->SetRegion( region );	
  //region.SetSize( size );
		
  typename InputImageType::PointType origin;
  origin.Fill( 0.0 );
		
  importFilter->SetOrigin( origin );
				
  typename ImportFilterType::SpacingType spacing;
  spacing.Fill( 1.0 );
		
  importFilter->SetSpacing( spacing );
  importFilter->SetImportPointer(inputData, 0, false);

  // run filter
  ccFilter->SetInput(importFilter->GetOutput());
  ccFilter->Update();

  LabelImageType* labelImage = ccFilter->GetOutput();
  return labelImage;
}

template <typename TInputPixelType>
void GraphCut::extractSubCube(TInputPixelType* inputData,
                              TInputPixelType* weights,
                              vector<Point>& sourcePoints,
                              vector<Point>& sinkPoints,
                              long nx, long ny, long nz)
{
  const unsigned int Dimension = 3;
  typedef unsigned int TLabelPixelType;
  typedef itk::Image< TLabelPixelType, Dimension > LabelImageType;

  LabelImageType::Pointer labelInput = getLabelImage<TInputPixelType,LabelImageType>(inputData,nx,ny,nz);

  typedef itk::ShapeLabelObject< long, Dimension >       LabelObjectType;
  //typedef itk::ShapeLabelObject< TInputPixelType, Dimension >       LabelObjectType;
  typedef itk::LabelMap< LabelObjectType >             LabelMapType;
  typedef itk::LabelImageToShapeLabelMapFilter< LabelImageType, LabelMapType > LabelFilterType;

  typename LabelFilterType::Pointer labelFilterInput = LabelFilterType::New();
  labelFilterInput->SetBackgroundValue(0);
  labelFilterInput->SetInput(labelInput);
  labelFilterInput->Update();

  LabelMapType* labelMapInput = labelFilterInput->GetOutput();

  typename LabelImageType::IndexType p;
  p[0] = sourcePoints[0].x;
  p[1] = sourcePoints[0].y;
  p[2] = sourcePoints[0].z;
  
  for(int i=0; i < labelMapInput->GetNumberOfLabelObjects(); i++)
    {
      LabelObjectType * labelObject = labelMapInput->GetNthLabelObject(i);

      typename LabelObjectType::RegionType region = labelObject->GetRegion();
      typename LabelImageType::IndexType indexType = region.GetIndex();
      typename LabelImageType::SizeType sizeType = region.GetSize();

      subX = indexType[0];
      subY = indexType[1];
      subZ = indexType[2];
      printf("Object %d. position=(%ld,%ld,%ld)\n", i, subX, subY, subZ);
    }

  int ccID = labelInput->GetPixel(p) - 1;
  printf("labelObject=%d\n", ccID);
  if(ccID < 0)
    return; // error : only accept unsigned 8 bits images
  LabelObjectType * labelObject = labelMapInput->GetNthLabelObject(ccID);

  typename LabelObjectType::RegionType region = labelObject->GetRegion();
  typename LabelImageType::IndexType indexType = region.GetIndex();
  typename LabelImageType::SizeType sizeType = region.GetSize();

  ulong cubeSize = sizeType[0]*sizeType[1]*sizeType[2];
  subX = indexType[0];
  subY = indexType[1];
  subZ = indexType[2];
  printf("Setting sub-cube. position=(%ld,%ld,%ld)\n", subX, subY, subZ);
  subCube = new Cube;
  subCube->data = new uchar[cubeSize];
  subCube->width = sizeType[0];
  subCube->height = sizeType[1];
  subCube->depth = sizeType[2];

  ulong sliceSize = nx*ny;
  ulong cubeIdx = 0;
  ulong inputIdx;
  for(int i=0; i < subCube->width; i++)
    for(int j=0; j < subCube->height; j++)
      for(int k=0; k < subCube->depth; k++,cubeIdx++)
        {
          inputIdx = (k+subZ)*sliceSize + (j+subY)*nx + (i+subX);
          subCube->data[cubeIdx] = weights[inputIdx];
        }
}


#endif //GRAPHCUT_H_
