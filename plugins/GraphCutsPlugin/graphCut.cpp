#include "graphCut.h"

GraphCut::GraphCut()
{
  m_node_ids = 0;
  m_graph = 0;
  running_maxflow = false;
  subCube = 0;
  subX = 0;
  subY = 0;
  subZ = 0;
}

GraphCut::~GraphCut() {
  // Free memory
  if(m_node_ids != 0)
    {
      delete[] m_node_ids;
    }
  if(m_graph!=0)
    delete m_graph;
  if(subCube != 0)
    delete subCube;
}

void GraphCut::applyCut(Cube* inputCube, unsigned char* output_data)
{
    if(output_data == 0) {
        return;
    }

    ulong wh = ni*nj;
    ulong n = wh*nk;

    ulong nSources = 0;
    ulong nSinks = 0;
    for(int cubeIdx=0; cubeIdx<n; cubeIdx++) {
        if(m_graph->what_segment(m_node_ids[cubeIdx]) == GraphType::SOURCE)
            nSources++;
        else
            nSinks++;
    }

  printf("nSources=%ld, nSinks=%ld\n", nSources,nSinks);

  int nodeType;
  int nNodes;
  if(nSources < nSinks)
    {
      nNodes = nSources;
      nodeType = GraphType::SOURCE;
    }
  else
    {
      nNodes = nSinks;
      nodeType = GraphType::SINK;
    }

  vector<Point> lNodes(nNodes);

#if 1
  ulong inputCube_sliceSize = inputCube->width*inputCube->height;
  ulong outputCubeIdx;
  ulong cubeIdx = 0;
  for(int k=0;k<nk;k++)
    for(int j = 0;j<nj;j++)
      for(int i = 0;i<ni;i++,cubeIdx++)
        {
          outputCubeIdx = ((k+subZ)*inputCube_sliceSize) + ((j+subY)*inputCube->width) + (i+subX);
          if(inputCube->data[outputCubeIdx] != 0) {
            if(m_graph->what_segment(m_node_ids[cubeIdx]) == nodeType) {
                output_data[outputCubeIdx] = 128;
            } else {
                output_data[outputCubeIdx] = 255;
            }
          }
          /*
          if(inputCube->data[outputCubeIdx] != 0) {
          //if(inputCube->at(i+subX,j+subY,k+subZ) != 0) {
              if(m_graph->what_segment(m_node_ids[cubeIdx]) == nodeType)
                {

                  //Point p;
                  //p.x = i;
                  //p.y = j;
                  //p.z = k;
                  //lNodes.push_back(p);

                  outputCubeIdx = ((k+subZ)*inputCube_sliceSize) + ((j+subY)*inputCube->width) + (i+subX);
                  //printf("Cutting (%d,%d,%d)\n",_x+subX,_y+subY,_z+subZ);
                  output_data[outputCubeIdx] = 255;
                } else {
                  outputCubeIdx = ((k+subZ)*inputCube_sliceSize) + ((j+subY)*inputCube->width) + (i+subX);
                  //printf("Cutting (%d,%d,%d)\n",_x+subX,_y+subY,_z+subZ);
                  output_data[outputCubeIdx] = 128;
                }
          }
          */
        }
#else
  ulong cubeIdx = 0;
  for(int k=0;k<nk;k++)
    for(int j = 0;j<nj;j++)
      for(int i = 0;i<ni;i++,cubeIdx++)
        {
          //if(m_graph->what_segment(m_node_ids[cubeIdx]) == nodeType) {
            Point p;
            p.x = i;
            p.y = j;
            p.z = k;
            lNodes.push_back(p);
          //}
        }
#endif

  printf("List of nodes generated. Contains %ld nodes\n",lNodes.size());

#if 0
  const int nh_size = 1;
  int x,y,z;
  int sx,sy,sz;
  int ex,ey,ez;
  ulong inputCube_sliceSize = inputCube->width*inputCube->height;
  ulong outputCubeIdx;
  for(vector<Point>::iterator itNode = lNodes.begin();
      itNode != lNodes.end(); itNode++)
    {
      //z = cubeIdx/wh;
      //y = (cubeIdx-(z*cubeIdx))/wh;      
      x = itNode->x;
      y = itNode->y;
      z = itNode->z;
      //printf("(%d,%d,%d)\n",x,y,z);
      sx = max(0,x-nh_size);
      ex = min(x+nh_size,ni-1);
      sy = max(0,y-nh_size);
      ey = min(y+nh_size,nj-1);
      sz = max(0,z-nh_size);
      ez = min(z+nh_size,nk-1);
      for(int _x = sx; _x <= ex; _x++) 
        for(int _y = sy; _y <= ey; _y++)
          for(int _z = sz; _z <= ez; _z++)
            {
              if(inputCube->at(_x+subX,_y+subY,_z+subZ) != 0) {
                  cubeIdx = (_z*wh) + (_y*ni) + _x;
                  if(m_graph->what_segment(m_node_ids[cubeIdx]) != nodeType)
                    {
                      outputCubeIdx = ((_z+subZ)*inputCube_sliceSize) + ((_y+subY)*inputCube->width) + (_x+subX);
                      //printf("Cutting (%d,%d,%d)\n",_x+subX,_y+subY,_z+subZ);
                      output_data[outputCubeIdx] = 255;
                    } else {
                      outputCubeIdx = ((_z+subZ)*inputCube_sliceSize) + ((_y+subY)*inputCube->width) + (_x+subX);
                      //printf("Cutting (%d,%d,%d)\n",_x+subX,_y+subY,_z+subZ);
                      output_data[outputCubeIdx] = 128;
                  }
              }
            }
    }
#endif
}

// caller is responsible for freeing memory
void GraphCut::getOutput(Cube* inputCube, uchar*& output_data)
{
  printf("GraphCut::getOutput Allocating %ldx%ldx%ld=%ld B\n",ni,nj,nk,ni*nj*nk);
  output_data = new uchar[ni*nj*nk];
  ulong wh = ni*nj;
  ulong cubeIdx = 0;
  for(int k=0;k<nk;k++)
    for(int j = 0;j<nj;j++)
      for(int i = 0;i<ni;i++,cubeIdx++)
        {
          if(m_graph->what_segment(m_node_ids[cubeIdx]) == GraphType::SOURCE)
            {
              output_data[k*wh+j*ni+i] = inputCube->at(i+subX,j+subY,k+subZ);
            }
          else
            output_data[k*wh+j*ni+i] = 128; //255
        }
}

void GraphCut::getCubeSize(ulong& cubeWidth,
                           ulong& cubeHeight,
                           ulong& cubeDepth)
{
  cubeWidth = subCube->width;
  cubeHeight = subCube->height;
  cubeDepth = subCube->depth;
}


unsigned long GraphCut::at(int i, int j, int k)
{
  return ((unsigned long)k*nij)+j*ni+i;
}

/*
 * Run Min-Cut/Max-Flow algorithm for energy minimization
 * Energy function is defined as E = B + R
 * B = Boundary term, R = Regional term
 */
void GraphCut::run_maxflow(Cube* cube,
                           vector<Point>& sourcePoints, vector<Point>& sinkPoints,
                           float sigma, int minDist)
{
  float weightToSource;
  float weightToSink;
  float weight;
  // TODO : compute the weight k
  // (weight of edge between mark object to the source or mark background to the sink)
  //float K = FLT_MAX;
  float K = 1e8;
  float MAX_WEIGHT = 1e2;

  running_maxflow = true;

  if(subCube)
    {
      printf("[GraphCuts] Performing maxflow on sub-cube\n");
      printf("[GraphCuts] Original size=(%ld,%ld,%ld)\n",cube->width,cube->height,cube->depth);
      cube = subCube;
      printf("[GraphCuts] New size=(%ld,%ld,%ld)\n",cube->width,cube->height,cube->depth);
    }

  ni = cube->width;
  nj = cube->height;
  nk = cube->depth;
  nij = ni*nj;

  // Free memory
  if(m_node_ids!=0) {
      delete[] m_node_ids;
  }
  if(m_graph!=0) {
    delete m_graph;
  }

  printf("[GraphCuts] graph size = (%d,%d,%d)\n", ni, nj, nk);

  // TODO : Compute correct parameters
  ulong nNodes = ni*nj*nk;
  ulong nEdges = nNodes*3;
  m_graph = new GraphType(nNodes, nEdges);

  m_node_ids = new GraphType::node_id[nNodes];
  for(ulong i = 0;i < nNodes; i++) {
      m_node_ids[i] = m_graph->add_node();
  }

  printf("[GraphCuts] %d nodes added\n", nNodes);

//#define USE_HISTOGRAM

#ifdef USE_HISTOGRAM
  // Compute histogram for boundary term
  const int nbItemsPerBin = 25;
  const int histoSize = 255/nbItemsPerBin;
  float* histoSource = new float[histoSize];
  int binId;
  memset(histoSource,0,histoSize*sizeof(float));
  for(vector<Point>::iterator itPoint=sourcePoints.begin();
      itPoint != sourcePoints.end();itPoint++)
    {
      binId = (int)cube->at(itPoint->x, itPoint->y, itPoint->z)/nbItemsPerBin - 1;
      if(binId < 0)
          binId = 0;
      if(binId >= histoSize)
          binId = histoSize - 1;
      histoSource[binId]++;
    }

  float* histoSink = new float[histoSize];
  memset(histoSink,0,histoSize*sizeof(float));
  for(vector<Point>::iterator itPoint=sinkPoints.begin();
      itPoint != sinkPoints.end();itPoint++)
    {
      binId = (int)cube->at(itPoint->x, itPoint->y, itPoint->z)/nbItemsPerBin - 1;
      if(binId < 0)
          binId = 0;
      if(binId >= histoSize)
          binId = histoSize - 1;
      histoSink[binId]++;
    }


  // Normalize histograms
  for(int i = 0;i<histoSize;i++)
    {
      histoSource[i] /= histoSize;
      histoSink[i] /= histoSize;
    }

  printf("[GraphCuts] Histograms\n");
  printf("Source:\n");
  for(int i=0;i<histoSize;i++)
    printf("%f ",histoSource[i]);
  printf("\nSink:\n");
  for(int i=0;i<histoSize;i++)
    printf("%f ",histoSink[i]);
  printf("\n\n");

#endif

  ulong nodeIdx = 0;
  nEdges = 0;
  ulong dist;
  int seedX, seedY, seedZ;
  //const int minDist = 9;
  minDist *= minDist; //squared distance
  for(int k = 0;k<nk;k++)
    {
      //printf("* %d/%d\n",k,nk);
      for(int j = 0;j<nj;j++)
        {
          for(int i = 0;i<ni;i++,nodeIdx++)
            {
              // Compute regional term
              weightToSink = 0.;
              weightToSource = 0;

              // Compute weights to source and sink nodes
              for(vector<Point>::iterator itPoint=sourcePoints.begin();
                  itPoint != sourcePoints.end();itPoint++)
                {
                  seedX = itPoint->x - subX;
                  seedY = itPoint->y - subY;
                  seedZ = itPoint->z - subZ;
                  dist = (seedX-i)*(seedX-i) + (seedY-j)*(seedY-j) + (seedZ-k)*(seedZ-k);
                  if(dist <= minDist)
                    {
                      printf("[GraphCuts] Source found %d %d %d\n", i, j, k);
                      weightToSource = K;
                      break;
                    }
                }

              for(vector<Point>::iterator itPoint=sinkPoints.begin();
                  itPoint != sinkPoints.end();itPoint++)
                {
                  seedX = itPoint->x - subX;
                  seedY = itPoint->y - subY;
                  seedZ = itPoint->z - subZ;
                  dist = (seedX-i)*(seedX-i) + (seedY-j)*(seedY-j) + (seedZ-k)*(seedZ-k);
                  if(dist <= minDist)
                    {
                      printf("[GraphCuts] Sink found %d %d %d\n", i, j, k);
                      weightToSink = K;
                      break;
                    }
                }

#ifdef USE_HISTOGRAM
              if(weightToSource != K)
                {
                  //FIXME : binId !!!!!!!!!!!
                  // Get value from histogram
                  binId = (int)(cube->at(i,j,k)/nbItemsPerBin) - 1;
                  if(binId >= histoSize) {
                    printf("binId %d >= histoSize\n", binId);
                    binId = histoSize - 1;
                  }
                  if(binId < 0) {
                    printf("binId %d < 0\n", binId);
                    binId = 0;
                  }
                  weightToSource = histoSource[binId];
                }
              if(weightToSink != K)
                {
                  // Get value from histogram
                  binId = (int)(cube->at(i,j,k)/nbItemsPerBin) - 1;
                  if(binId >= histoSize) {
                    printf("binId >= histoSize\n");
                    binId = histoSize - 1;
                  }
                  if(binId < 0) {
                    printf("binId < 0\n");
                    binId = 0;
                  }
                  weightToSink = histoSink[binId];
                }
#endif

              //if(weightToSource != 0 || weightToSink != 0)
              //  printf("(%d,%d,%d) : %f/%f\n",i,j,k,weightToSource,weightToSink);
              m_graph->add_tweights(m_node_ids[nodeIdx],weightToSource,weightToSink);


              // Compute boundary term
              // B(p,q) = exp(-(Ip - Iq)^2 / 2*sigma)/dist(p,q)
              if(i+1 < ni && (m_node_ids[nodeIdx] != m_node_ids[at(i+1,j,k)]))
                {
                  //weight = exp(-pow((cube->at(i,j,k)-cube->at(i+1,j,k))*sigma,2.f));
                  weight = pow(cube->at(i,j,k)-cube->at(i+1,j,k),2.0f);
                  weight = ((weight==0)?MAX_WEIGHT:1.0f/weight)*sigma;
                  m_graph->add_edge(m_node_ids[nodeIdx], m_node_ids[at(i+1,j,k)], weight, weight);
                  //if(i==4)
                  //  printf("(%d,%d,%d)-(%d,%d,%d) : %d %d %d %f\n",i,j,k,i+1,j,k,cube->at(i,j,k),cube->at(i+1,j,k),cube->at(i,j,k)-cube->at(i+1,j,k),weight);
                  nEdges++;
                }

              if(j+1 < nj && (m_node_ids[nodeIdx] != m_node_ids[at(i,j+1,k)]))
                {
                  //weight = exp(-pow((cube->at(i,j,k)-cube->at(i,j+1,k))*sigma,2.f));
                  weight = pow(cube->at(i,j,k)-cube->at(i,j+1,k),2.0f);
                  weight = ((weight==0)?MAX_WEIGHT:1.0f/weight)*sigma;
                  m_graph->add_edge(m_node_ids[nodeIdx], m_node_ids[at(i,j+1,k)], weight, weight);
                  //printf("(%d,%d,%d)-(%d,%d,%d) : %f\n",i,j,k,i,j+1,k,weight);
                  nEdges++;
                }
              if(k+1 < nk && (m_node_ids[nodeIdx] != m_node_ids[at(i,j,k+1)]))
                {
                  //weight = exp(-pow((cube->at(i,j,k)-cube->at(i,j,k+1))*sigma,2.f));
                  weight = pow(cube->at(i,j,k)-cube->at(i,j,k+1),2.0f);
                  weight = ((weight==0)?MAX_WEIGHT:1.0f/weight)*sigma;
                  m_graph->add_edge(m_node_ids[nodeIdx], m_node_ids[at(i,j,k+1)], weight, weight);
                  //printf("(%d,%d,%d)-(%d,%d,%d) : %f\n",i,j,k,i,j,k+1,weight);
                  nEdges++;
                }
            }
        }
    }

  printf("[GraphCuts] %d edges added\n", nEdges);

  printf("[GraphCuts] Computing max flow\n");
  int flow = m_graph->maxflow();

#if 0
  // TODO : debug only, get rid of this part
  for(int k = 0;k<cube->depth;k++)
    {
      IplImage* img = cvCreateImage( cvSize(cube->width, cube->height), 8, 1 );
      unsigned char* ptrImage;

      for(int i = 0;i<cube->width;i++)
        for(int j = 0;j<cube->height;j++)
          {
            ptrImage = &((uchar*)(img->imageData + img->widthStep*j))[i];
            if(m_graph->what_segment(m_node_ids[nodeIdx]) == GraphType::SOURCE)
              {
                //printf("Image : SOURCE\n");
                //*ptrImage = 0;
                *ptrImage = cube->at(i,j,k);
              }
            else
              {
                //printf("Image : SINK\n");
                *ptrImage = 255;
              }
          }

      std::string s;
      std::stringstream out;
      out << k;
      s = "graphcut_" + out.str();
      s += ".jpg";
      printf("Saving image %s\n", s.c_str());
      cvSaveImage(s.c_str(), img);
      cvReleaseImage(&img);
    }
#endif

  // Cleaning
#ifdef USE_HISTOGRAM
  delete[] histoSource;
  delete[] histoSink;
#endif

  printf("[GraphCuts] Max flow=%d\n", flow);
  running_maxflow = false;
}
