
typedef unsigned char uchar;

void exportTIFCube(uchar* rawData,
                   const char* filename,
                   int cubeDepth,
                   int cubeHeight,
                   int cubeWidth);

void cubeFloat2Uchar(float* inputData, uchar*& outputData,
                     int nx, int ny, int nz);
