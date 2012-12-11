
typedef unsigned char uchar;

void exportTIFCube(uchar* rawData,
                   const char* filename,
                   int cubeDepth,
                   int cubeHeight,
                   int cubeWidth);

void cubeFloat2Uchar(float* inputData, uchar*& outputData,
                     int nx, int ny, int nz);

template <typename TInputPixelType, typename LabelImageType>
typename LabelImageType::Pointer getLabelImage(TInputPixelType* inputData,
                                               long nx, long ny, long nz);
