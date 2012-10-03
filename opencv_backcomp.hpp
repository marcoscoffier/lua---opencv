#ifndef __OPENCV_BACKCOMP_HPP__
#define __OPENCV_BACKCOMP_HPP__

#include <opencv/cv.h>

#if CV_MAJOR_VERSION == 2
#if CV_MINOR_VERSION >= 4

#define OPENCV_BACKCOMP

extern void cvCalcOpticalFlowBM(const CvArr* prev, const CvArr* curr, CvSize blockSize,
				CvSize shiftSize, CvSize maxRange, int usePrevious,
				CvArr* velx, CvArr* vely);

extern void cvCalcOpticalFlowHS(const CvArr* prev, const CvArr* curr, int usePrevious,
				CvArr* velx, CvArr* vely, double lambda,
				CvTermCriteria criteria);

extern void cvCalcOpticalFlowLK(const CvArr* prev, const CvArr* curr, CvSize winSize,
				CvArr* velx, CvArr* vely);

// CVAPI(CvSubdiv2D*) cvCreateSubdiv2D(int subdiv_type, int header_size,
// 				    int vtx_size, int quadedge_size, CvMemStorage* storage);

// CVAPI(void) cvInitSubdivDelaunay2D(CvSubdiv2D* subdiv, CvRect rect);

// CVAPI(CvSubdiv2DPoint*) cvSubdivDelaunay2DInsert(CvSubdiv2D* subdiv, CvPoint2D32f pt);

// CVAPI(void) cvCalcSubdivVoronoi2D(CvSubdiv2D* subdiv);

// CVAPI(CvSubdiv2DPointLocation) cvSubdiv2DLocate(CvSubdiv2D* subdiv, CvPoint2D32f pt,
// 						CvSubdiv2DEdge* edge, CvSubdiv2DPoint** vertex);

#ifndef OPENCV_BACKCOMP_CPP
#if CV_MINOR_VERSION != 4
#warning "cvSubdiv2DGetEdge* : not tested with OpenCV version > 2.4"
#endif

// CV_INLINE  CvSubdiv2DEdge  cvSubdiv2DGetEdge( CvSubdiv2DEdge edge, CvNextEdgeType type )
// {
//   CvQuadEdge2D* e = (CvQuadEdge2D*)(edge & ~3);
//   edge = e->next[(edge + (int)type) & 3];
//   return  (edge & ~3) + ((edge + ((int)type >> 4)) & 3);
// }


// CV_INLINE  CvSubdiv2DPoint*  cvSubdiv2DEdgeOrg( CvSubdiv2DEdge edge )
// {
//   CvQuadEdge2D* e = (CvQuadEdge2D*)(edge & ~3);
//   return (CvSubdiv2DPoint*)e->pt[edge & 3];
// }


// CV_INLINE  CvSubdiv2DPoint*  cvSubdiv2DEdgeDst( CvSubdiv2DEdge edge )
// {
//   CvQuadEdge2D* e = (CvQuadEdge2D*)(edge & ~3);
//   return (CvSubdiv2DPoint*)e->pt[(edge + 2) & 3];
// }

#endif

#ifndef OPENCV_BACKCOMP_CPP
typedef struct CvStereoGCState
{
  int Ithreshold;
  int interactionRadius;
  float K, lambda, lambda1, lambda2;
  int occlusionCost;
  int minDisparity;
  int numberOfDisparities;
  int maxIters;
    
  CvMat* left;
  CvMat* right;
  CvMat* dispLeft;
  CvMat* dispRight;
  CvMat* ptrLeft;
  CvMat* ptrRight;
  CvMat* vtxBuf;
  CvMat* edgeBuf;
} CvStereoGCState;
#endif

CVAPI(CvStereoGCState*) cvCreateStereoGCState(int numberOfDisparities, int maxIters);

CVAPI(void) cvFindStereoCorrespondenceGC(const CvArr* left, const CvArr* right,
					 CvArr* disparityLeft, CvArr* disparityRight,
					 CvStereoGCState* state, int useDisparityGuess);

CVAPI(void) cvReleaseStereoGCState( CvStereoGCState** state );

#endif
#endif

#endif
