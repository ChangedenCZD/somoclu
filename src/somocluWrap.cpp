
#include"somocluWrap.h"
#include <cmath>
#include <iostream>
#include <string.h>

using namespace std;

void trainWrapper(float *data, int data_length,
                  unsigned int nEpoch,
                  unsigned int nSomX, unsigned int nSomY,
                  unsigned int nDimensions, unsigned int nVectors,
                  unsigned int radius0, unsigned int radiusN,
                  string radiusCooling,
                  float scale0, float scaleN,
                  string scaleCooling, unsigned int kernelType, string mapType,
                  float *initialCodebook, int initialCodebook_size,
                  float *codebook, int codebook_size,
                  int *globalBmus, int globalBmus_size,
                  float *uMatrix, int uMatrix_size)
{

  int itask = 0;
  svm_node ** sparseData = NULL;
  core_data coreData;
  coreData.codebook_size = nSomY*nSomX*nDimensions;
  coreData.codebook = new float[coreData.codebook_size];
  coreData.globalBmus = NULL;
  coreData.uMatrix = NULL;
  unsigned int nVectorsPerRank = nVectors;
  coreData.globalBmus_size = nVectorsPerRank*int(ceil(nVectors/(double)nVectorsPerRank))*2;
  if (itask == 0) {
      coreData.globalBmus = new int[coreData.globalBmus_size];

      if (initialCodebook_size == 1){
          initializeCodebook(0, coreData.codebook, nSomX, nSomY, nDimensions);
      } else {
          if (initialCodebook_size != nSomX*nSomY*nDimensions) {
              cerr << "Dimension of initial codebook does not match data! " << initialCodebook_size << " " << nSomX*nSomY*nDimensions << "\n";
              initializeCodebook(0, coreData.codebook, nSomX, nSomY, nDimensions);
          } else { 
              memcpy(coreData.codebook, initialCodebook, sizeof(float)*nSomX*nSomY*nDimensions);
          }
      }
  }

#ifdef CUDA
    if(kernelType==DENSE_GPU){
        int rank = 0;
        int nProcs = 1;
        setDevice(rank, nProcs);
        initializeGpu(data, nVectorsPerRank, nDimensions, nSomX, nSomY);
    }
#endif

  ///
  /// Parameters for SOM
  ///
  if (radius0 == 0) {
      radius0 = nSomX / 2.0f;              /// init radius for updating neighbors
  }
  if (radiusN == 0) {
      radiusN = 1;
  }
  if (scale0 == 0) {
    scale0 = 1.0;
  }

  unsigned int currentEpoch = 0;             /// 0...nEpoch-1

  ///
  /// Training
  ///

  while ( currentEpoch < nEpoch ) {

      coreData = trainOneEpoch(itask, data, sparseData,
                               coreData, nEpoch, currentEpoch,
                               false,
                               nSomX, nSomY,
                               nDimensions, nVectors,
                               nVectorsPerRank,
                               radius0, radiusN,
                               radiusCooling,
                               scale0, scaleN,
                               scaleCooling,
                               kernelType, mapType);

      currentEpoch++;
    }

  if (itask == 0) {
      coreData.uMatrix = calculateUMatrix(coreData.codebook, nSomX, nSomY, nDimensions, mapType);
      coreData.uMatrix_size = nSomX * nSomY;
  }
#ifdef CUDA
  if (kernelType == DENSE_GPU) {
      freeGpu();
  }
#endif
  if(coreData.codebook != NULL){
      memcpy(codebook, coreData.codebook, sizeof(float) *  codebook_size);
      delete [] coreData.codebook;
    }
  if(coreData.globalBmus != NULL){
      memcpy(globalBmus, coreData.globalBmus, sizeof(int) *  globalBmus_size);
      delete [] coreData.globalBmus;
    }
  if(coreData.uMatrix != NULL){
      memcpy(uMatrix, coreData.uMatrix, sizeof(float) *  uMatrix_size);
      delete [] coreData.uMatrix;
    }
}
