#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/opencv.c"
#else

//======================================================================
// File: opencv
//
// Description: A wrapper for a couple of OpenCV functions.
//
// Created: February 12, 2010, 1:22AM
//
// Install on ubuntu : 
//  http://www.samontab.com/web/2010/04/installing-opencv-2-1-in-ubuntu/
//
// Author: Clement Farabet // clement.farabet@gmail.com
//         Marco Scoffier // github@metm.org (more functions)
//======================================================================

#include <luaT.h>
#include <TH.h>

#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>


static THTensor * libopencv_(Main_opencv8U2torch)(IplImage *source, THTensor *dest) {
  // Pointers
  uchar * source_data; 

  // Get pointers / info
  int source_step;
  CvSize source_size;
  cvGetRawData(source, (uchar**)&source_data, &source_step, &source_size);

  // Resize target
  THTensor_(resize3d)(dest, source->nChannels, source->height, source->width);  
  THTensor *tensor = THTensor_(newContiguous)(dest);

  // Torch stores channels first, opencv last so we select the channel
  // in torch tensor and step through the opencv iplimage.
  int j = 0;
  int k = source->nChannels-1;
  uchar * sourcep = source_data;
  for (j=0;j<source->nChannels;j++){
    sourcep = source_data+k-j; // start at correct channel opencv is BGR
    THTensor *tslice = THTensor_(newSelect)(tensor,0,j);
    // copy
    TH_TENSOR_APPLY(real, tslice, 
		    *tslice_data = ((real)(*sourcep))/255.0;
		    // step through channels of ipl
		    sourcep = sourcep + source->nChannels; 
		    );
  }
  // return freshly created torch tensor
  return tensor;
}

static THTensor * libopencv_(Main_opencv32F2torch)(IplImage *source, THTensor *dest) {
  // Pointers
  float * source_data; 

  // Get pointers / info
  int source_step;
  CvSize source_size;
  cvGetRawData(source, (uchar**)&source_data, &source_step, &source_size);

  // Resize target
  THTensor_(resize3d)(dest, source->nChannels, source->height, source->width);  
  THTensor *tensor = THTensor_(newContiguous)(dest);

  // Torch stores channels first, opencv last so we select the channel
  // in torch tensor and step through the opencv iplimage.
  int j = 0;
  int k = source->nChannels-1;
  float * sourcep = source_data;
  for (j=0;j<source->nChannels;j++){
    sourcep = source_data+k-j; // start at correct channel opencv is BGR
    THTensor *tslice = THTensor_(newSelect)(tensor,0,j);
    // copy
    TH_TENSOR_APPLY(real, tslice, 
		    *tslice_data = (real)(*sourcep);
		    // step through ipl
		    sourcep = sourcep + source->nChannels; 
		    );
  }
  // return freshly created torch tensor
    return tensor;
}

static IplImage * libopencv_(Main_torch2opencv_8U)(THTensor *source) {
  // Pointers
  uchar * dest_data;

  // Get size and channels
  int channels = source->size[0];
  int dest_step;
  CvSize dest_size = cvSize(source->size[2], source->size[1]);

  // Create ipl image
  IplImage * dest = cvCreateImage(dest_size, IPL_DEPTH_8U, channels);

  // get pointer to raw data
  cvGetRawData(dest, (uchar**)&dest_data, &dest_step, &dest_size);
  // copy
  THTensor *tensor = THTensor_(newContiguous)(source);
  // Torch stores channels first, opencv last so we select the channel
  // in torch tensor and step through the opencv iplimage.
  int i = 0,j = 0;
  int k = channels-1;
  uchar * destp = dest_data;
  for (j=0;j<dest->nChannels;j++){
    i=0;
    destp = dest_data+k-j; // start at correct channel opencv is BGR
    THTensor *tslice = THTensor_(newSelect)(tensor,0,j);
    // copy
    TH_TENSOR_APPLY(real, tslice, 
		    *destp = (uchar)(*tslice_data * 255.0);
		    // step through ipl
		    destp = destp + dest->nChannels; 
		    );
  }
  // return freshly created IPL image
  return dest;
}

static IplImage * libopencv_(Main_torch2opencv_32F)(THTensor *source) {
  // Pointers
  float * dest_data;

  // Get size and channels
  int channels = source->size[0];
  int dest_step;
  CvSize dest_size = cvSize(source->size[2], source->size[1]);

  // Create ipl image
  IplImage * dest = cvCreateImage(dest_size, IPL_DEPTH_32F, channels);

  // get pointer to raw data
  cvGetRawData(dest, (uchar**)&dest_data, &dest_step, &dest_size);
  // copy
  THTensor *tensor = THTensor_(newContiguous)(source);
  // Torch stores channels first, opencv last so we select the channel
  // in torch tensor and step through the opencv iplimage.
  int j = 0;
  int k = channels-1;
  float * destp = dest_data;
  for (j=0;j<dest->nChannels;j++){
    destp = dest_data+k-j; // start at correct channel opencv is BGR
    THTensor *tslice = THTensor_(newSelect)(tensor,0,j);
    // copy
    TH_TENSOR_APPLY(real, tslice, 
		    *destp = (float)(*tslice_data);
		    destp = destp + dest->nChannels; // step through ipl
		    );
  }
  // return freshly created IPL image
  return dest;
}

static THTensor * libopencv_(Main_opencvPoints2torch)(CvPoint2D32f * points, int npoints, THTensor *tensor) {

  // Resize target
  THTensor_(resize2d)(tensor, npoints, 2);
  THTensor *tensorc = THTensor_(newContiguous)(tensor);
  real *tensor_data = THTensor_(data)(tensorc);

  // copy
  int p;
  for (p=0; p<npoints; p++){
    *tensor_data++ = (real)points[p].x;
    *tensor_data++ = (real)points[p].y;
  }

  // return freshly created IPL image
  return tensor;
}

static CvPoint2D32f * libopencv_(Main_torch2opencvPoints)(THTensor *src) {

  int count = src->size[0];
  // create output
  CvPoint2D32f * points_cv = NULL;
  points_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points_cv[0]));

  // copy
  int p;
  for (p=0; p<count; p++){
    points_cv[p].x = (float)THTensor_(get2d)(src, p, 0);
    points_cv[p].y = (float)THTensor_(get2d)(src, p, 1);
  }

  // return freshly created IPL image
  return points_cv;
}


static int libopencv_(Main_testTH2IPL8U)(lua_State *L) {
  THTensor * src  = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * dst  = luaT_checkudata(L, 2, torch_(Tensor_id));  
  IplImage * ipl = libopencv_(Main_torch2opencv_8U)(src);
  real *src_data = THTensor_(data)(src);

  libopencv_(Main_opencv8U2torch)(ipl, dst);
  real *dst_data = THTensor_(data)(dst);
  cvReleaseImage(&ipl);
  return 0;
}
static int libopencv_(Main_testTH2IPL32F)(lua_State *L) {
  THTensor * src  = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * dst  = luaT_checkudata(L, 2, torch_(Tensor_id));  
  IplImage * ipl = libopencv_(Main_torch2opencv_32F)(src);
  real *src_data = THTensor_(data)(src);

  libopencv_(Main_opencv32F2torch)(ipl, dst);
  real *dst_data = THTensor_(data)(dst);
  cvReleaseImage(&ipl);
  return 0;
}

//============================================================
static int libopencv_(Main_cvCornerHarris) (lua_State *L) {
  // Get Tensor's Info
  THTensor * image  = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * harris = luaT_checkudata(L, 2, torch_(Tensor_id));  

  if (image->size[0] > 1){
    printf("WARNING: CorverHarris only accepts single channel images\n");
  } else {
    CvSize dest_size = cvSize(image->size[2], image->size[1]);
    IplImage * image_ipl = libopencv_(Main_torch2opencv_8U)(image);
    // Create ipl image
    IplImage * harris_ipl = cvCreateImage(dest_size, IPL_DEPTH_32F, 1);
    int blockSize = 5;
    int aperture_size = 3;
    double k = 0.04;
    
    // User values:
    if (lua_isnumber(L, 3)) {
      blockSize = lua_tonumber(L, 3);
    }
    if (lua_isnumber(L, 4)) {
      aperture_size = lua_tonumber(L, 4);
    }
    if (lua_isnumber(L, 5)) {
      k = lua_tonumber(L, 5);
    }

    cvCornerHarris(image_ipl, harris_ipl, blockSize, aperture_size, k);

    // return results
    libopencv_(Main_opencv32F2torch)(harris_ipl, harris);

    // Deallocate IPL images
    cvReleaseImage(&harris_ipl);
    cvReleaseImage(&image_ipl);
  }

  return 0;
}

//============================================================
// OpticalFlow
// Works on torch.Tensors (double). All the conversions are
// done in C.
//
static int libopencv_(Main_cvCalcOpticalFlow)(lua_State *L) {
  // Get Tensor's Info
  THTensor * curr = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * prev = luaT_checkudata(L, 2, torch_(Tensor_id));  
  THTensor * velx = luaT_checkudata(L, 3, torch_(Tensor_id));  
  THTensor * vely = luaT_checkudata(L, 4, torch_(Tensor_id));  

  // Generate IPL images
  IplImage * curr_ipl = libopencv_(Main_torch2opencv_8U)(curr);
  IplImage * prev_ipl = libopencv_(Main_torch2opencv_8U)(prev);
  IplImage * velx_ipl;
  IplImage * vely_ipl;

  // Default values
  int method = 1;
  int lagrangian = 1;
  int iterations = 5;
  CvSize blockSize = cvSize(7, 7);
  CvSize shiftSize = cvSize(20, 20);
  CvSize max_range = cvSize(20, 20);
  int usePrevious = 0;

  // User values:
  if (lua_isnumber(L, 5)) {
    method = lua_tonumber(L, 5);
  }

  // HS only:
  if (lua_isnumber(L, 6)) {
    lagrangian = lua_tonumber(L, 6);
  }
  if (lua_isnumber(L, 7)) {
    iterations = lua_tonumber(L, 7);
  }

  // BM+LK only:
  if (lua_isnumber(L, 6) && lua_isnumber(L, 7)) { 
    blockSize.width = lua_tonumber(L, 6); 
    blockSize.height = lua_tonumber(L, 7);
  }
  if (lua_isnumber(L, 8) && lua_isnumber(L, 9)) { 
    shiftSize.width = lua_tonumber(L, 8); 
    shiftSize.height = lua_tonumber(L, 9);
  }
  if (lua_isnumber(L, 10) && lua_isnumber(L, 11)) { 
    max_range.width = lua_tonumber(L, 10); 
    max_range.height = lua_tonumber(L, 11);
  }
  if (lua_isnumber(L, 12)) { 
    usePrevious = lua_tonumber(L, 12); 
  }  

  // Compute flow
  if (method == 1) 
    {
      // Alloc outputs
      CvSize osize = cvSize((prev_ipl->width-blockSize.width)/shiftSize.width,
                            (prev_ipl->height-blockSize.height)/shiftSize.height);

      // Use previous results
      if (usePrevious == 1) {
        velx_ipl = libopencv_(Main_torch2opencv_32F)(velx);
        vely_ipl = libopencv_(Main_torch2opencv_32F)(vely);
      } else {
        velx_ipl = cvCreateImage(osize, IPL_DEPTH_32F, 1);
        vely_ipl = cvCreateImage(osize, IPL_DEPTH_32F, 1);
      }

      // Cv Call
      cvCalcOpticalFlowBM(prev_ipl, curr_ipl, blockSize, shiftSize, 
                          max_range, usePrevious, velx_ipl, vely_ipl);
    }
  else if (method == 2) 
    {
      // Alloc outputs
      CvSize osize = cvSize(prev_ipl->width, prev_ipl->height);

      velx_ipl = cvCreateImage(osize, IPL_DEPTH_32F, 1);
      vely_ipl = cvCreateImage(osize, IPL_DEPTH_32F, 1);

      // Cv Call
      cvCalcOpticalFlowLK(prev_ipl, curr_ipl, blockSize, velx_ipl, vely_ipl);
    }
  else if (method == 3) 
    {
      // Alloc outputs
      CvSize osize = cvSize(prev_ipl->width, prev_ipl->height);

      // Use previous results
      if (usePrevious == 1) {
        velx_ipl = libopencv_(Main_torch2opencv_32F)(velx);
        vely_ipl = libopencv_(Main_torch2opencv_32F)(vely);
      } else {
        velx_ipl = cvCreateImage(osize, IPL_DEPTH_32F, 1);
        vely_ipl = cvCreateImage(osize, IPL_DEPTH_32F, 1);
      }

      // Iteration criterion
      CvTermCriteria term = cvTermCriteria(CV_TERMCRIT_ITER, iterations, 0);

      // Cv Call
      cvCalcOpticalFlowHS(prev_ipl, curr_ipl, 
			  usePrevious, velx_ipl, vely_ipl, 
                          lagrangian, term);
    }

  // return results
  libopencv_(Main_opencv32F2torch)(velx_ipl, velx);
  libopencv_(Main_opencv32F2torch)(vely_ipl, vely);

  // Deallocate IPL images
  cvReleaseImage(&prev_ipl);
  cvReleaseImage(&curr_ipl);
  cvReleaseImage(&vely_ipl);
  cvReleaseImage(&velx_ipl);

  return 0;
}

//============================================================
static int libopencv_(Main_cvGoodFeaturesToTrack) (lua_State *L) {
  // Get Tensor's Info
  THTensor * image     = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * points    = luaT_checkudata(L, 2, torch_(Tensor_id));  
  THTensor * image_out = luaT_checkudata(L, 3, torch_(Tensor_id));  
  
  CvSize dest_size         = cvSize(image->size[2], image->size[1]);
  IplImage * image_ipl     = libopencv_(Main_torch2opencv_8U)(image);
  IplImage * image_out_ipl = libopencv_(Main_torch2opencv_8U)(image_out);

  IplImage * grey = cvCreateImage( dest_size, 8, 1 );

  cvCvtColor( image_ipl, grey, CV_BGR2GRAY );
  CvPoint2D32f* points_cv = 0;

  IplImage* eig =  cvCreateImage( dest_size, 32, 1 );
  IplImage* temp = cvCreateImage( dest_size, 32, 1 );

  int count = 500;
  double quality = 0.01;
  double min_distance = 10;
  int win_size = 10;  

  // User values:
  if (lua_isnumber(L, 4)) {
    count = lua_tonumber(L, 4);
  }
  if (lua_isnumber(L, 5)) {
    quality = lua_tonumber(L, 5);
  }
  if (lua_isnumber(L, 6)) {
    min_distance = lua_tonumber(L, 6);
  }
  if (lua_isnumber(L, 7)) {
    win_size = lua_tonumber(L, 7);
  }

  points_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points_cv[0]));

  cvGoodFeaturesToTrack( grey, eig, temp, points_cv, &count,
			 quality, min_distance, 0, 3, 0, 0.04 );
  
  /*
    // this function is Seq Faulting (not sure why...)
  cvFindCornerSubPix( grey, points_cv, count,
  		      cvSize(win_size,win_size),
  		      cvSize(-1,-1),
  		      cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,
  				     20,0.03));
  */
  int i = 0;
  for( i = 0; i < count; i++ ) {
    cvCircle( image_out_ipl, cvPointFrom32f(points_cv[i]), 25, 
	      CV_RGB(0,255,0), 1, 8,0);
  }
  // return results
  points = libopencv_(Main_opencvPoints2torch)(points_cv, count, points);
  libopencv_(Main_opencv8U2torch)(image_out_ipl, image_out);

  // Deallocate points_cv
  cvFree(&points_cv);
  cvReleaseImage( &eig );
  cvReleaseImage( &temp );
  cvReleaseImage( &grey );
  cvReleaseImage( &image_ipl );
  cvReleaseImage( &image_out_ipl );

  return 0;
}

//============================================================
static int libopencv_(Main_cvCalcOpticalFlowPyrLK) (lua_State *L) {
  // Get Tensor's Info
  THTensor * image1 = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * image2 = luaT_checkudata(L, 2, torch_(Tensor_id));  
  THTensor * flow_x = luaT_checkudata(L, 3, torch_(Tensor_id));  
  THTensor * flow_y = luaT_checkudata(L, 4, torch_(Tensor_id));  
  THTensor * points = luaT_checkudata(L, 5, torch_(Tensor_id));  
  THTensor * image_out = luaT_checkudata(L, 6, torch_(Tensor_id));  
  
  printf("Parsed args\n");
  int count = 500;
  double quality = 0.01;
  double min_distance = 10;
  int win_size = 10;  

  // User values:
  if (lua_isnumber(L, 7)) {
    count = lua_tonumber(L, 7);
  }
  if (lua_isnumber(L, 8)) {
    quality = lua_tonumber(L, 8);
  }
  if (lua_isnumber(L, 9)) {
    min_distance = lua_tonumber(L, 9);
  }
  if (lua_isnumber(L, 10)) {
    win_size = lua_tonumber(L, 10);
  }
  printf("updated defaults\n");
  printf("size: (%ld,%ld)\n",image1->size[2], image1->size[1]);
  CvSize dest_size = cvSize(image1->size[2], image1->size[1]);
  IplImage * image1_ipl    = libopencv_(Main_torch2opencv_8U)(image1);
  IplImage * image2_ipl    = libopencv_(Main_torch2opencv_8U)(image2);
  THTensor_(resize3d)(image_out, 
		      image1->size[0],image1->size[1],image1->size[2]);
  IplImage * image_out_ipl = libopencv_(Main_torch2opencv_8U)(image_out);
  printf("converted images\n");

  IplImage * grey1 = cvCreateImage( dest_size, 8, 1 );
  IplImage * grey2 = cvCreateImage( dest_size, 8, 1 );

  cvCvtColor( image1_ipl, grey1, CV_BGR2GRAY );
  cvCvtColor( image2_ipl, grey2, CV_BGR2GRAY );
  CvPoint2D32f* points1_cv = 0;
  CvPoint2D32f* points2_cv = 0;

  printf("Created IPL structures\n");
  IplImage* eig = cvCreateImage( dest_size, 32, 1 );
  IplImage* temp = cvCreateImage( dest_size, 32, 1 );

  // FIXME reuse points
  points1_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points1_cv[0]));
  points2_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points2_cv[0]));

  printf("Malloced points\n");
  cvGoodFeaturesToTrack( grey1, eig, temp, points1_cv, &count,
			 quality, min_distance, 0, 3, 0, 0.04 );
  printf("got good features for points1\n");
  /*
  cvFindCornerSubPix( grey1, points1_cv, count,
		      cvSize(win_size,win_size), 
		      cvSize(-1,-1),
		      cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,
				     20,0.03));
  printf("Found SubPixel\n");
  */
  // Call Lucas Kanade algorithm
  char features_found[ count ];
  float feature_errors[ count ];
  CvSize pyr_sz = cvSize( image1_ipl->width+8, image1_ipl->height/3 );

  IplImage* pyrA = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
  IplImage* pyrB = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
  
  cvCalcOpticalFlowPyrLK( grey1, grey2, 
			  pyrA, pyrB, 
			  points1_cv, points2_cv, 
			  count, 
			  cvSize( win_size, win_size ), 
			  5, features_found, feature_errors,
			  cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 ), 0 );
  // make image
  int i;
  for( i = 0; i < count; i++ ) {
    if (features_found[i] >0){
      CvPoint p0 = cvPoint( cvRound( points1_cv[i].x), 
			    cvRound( points1_cv[i].y));
      CvPoint p1 = cvPoint( cvRound( points2_cv[i].x), 
			    cvRound( points2_cv[i].y));
      cvLine( image_out_ipl, p0, p1, CV_RGB(255,0,0), 1, CV_AA, 0);
      //create the flow vectors to be compatible with the other
      //opticalFlows
      if (((p1.x > 0) && (p1.x < flow_x->size[0])) &&
	  ((p1.y > 0) && (p1.y < flow_x->size[1]))) {
	THTensor_(set2d)(flow_x,p1.x,p1.y,points1_cv[i].x - points2_cv[i].x);
	THTensor_(set2d)(flow_y,p1.x,p1.y,points1_cv[i].y - points2_cv[i].y);
      }
    }
  }
  
  // return results
  libopencv_(Main_opencvPoints2torch)(points2_cv, count, points);
  libopencv_(Main_opencv8U2torch)(image_out_ipl, image_out);

  // Deallocate points_cv
  cvFree(&points1_cv);
  cvFree(&points2_cv);
  cvReleaseImage( &eig );
  cvReleaseImage( &temp );
  cvReleaseImage( &pyrA );
  cvReleaseImage( &pyrB );
  cvReleaseImage( &grey1 );
  cvReleaseImage( &grey2);
  cvReleaseImage( &image1_ipl );
  cvReleaseImage( &image2_ipl );
  cvReleaseImage( &image_out_ipl );

  return 0;
}

//============================================================
static int libopencv_(Main_cvTrackPyrLK) (lua_State *L) {
  // Get Tensor's Info
  THTensor * image1  = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * image2  = luaT_checkudata(L, 2, torch_(Tensor_id));  
  THTensor * points1 = luaT_checkudata(L, 3, torch_(Tensor_id));  
  THTensor * points2 = luaT_checkudata(L, 4, torch_(Tensor_id));  
  THTensor * ff = 0;
  THTensor * fe = 0;

  int count = points1->size[0];
  int win_size = 10;  

  // User values:
  if (lua_isnumber(L, 5)) {
    win_size = lua_tonumber(L, 5);
  }

  if (!lua_isnil(L,6)) {
    ff = luaT_checkudata(L,6,torch_(Tensor_id));  
    THTensor_(resize1d)(ff,count);
  }
  if (!lua_isnil(L,7)) {
    fe = luaT_checkudata(L,7,torch_(Tensor_id));  
    THTensor_(resize1d)(fe,count);
  }

  CvSize dest_size = cvSize(image1->size[2], image1->size[1]);
  IplImage * image1_ipl = libopencv_(Main_torch2opencv_8U)(image1);
  IplImage * image2_ipl = libopencv_(Main_torch2opencv_8U)(image2);
  

  IplImage * grey1 = cvCreateImage( dest_size, 8, 1 );
  IplImage * grey2 = cvCreateImage( dest_size, 8, 1 );

  cvCvtColor( image1_ipl, grey1, CV_BGR2GRAY );
  cvCvtColor( image2_ipl, grey2, CV_BGR2GRAY );
  CvPoint2D32f* points1_cv = libopencv_(Main_torch2opencvPoints)(points1);
  CvPoint2D32f* points2_cv = 0;
  points2_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points2_cv[0]));


  // Call Lucas Kanade algorithm
  char features_found[ count ];
  float feature_errors[ count ];
  CvSize pyr_sz = cvSize( image1_ipl->width+8, image1_ipl->height/3 );

  IplImage* pyrA = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
  IplImage* pyrB = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
  
  cvCalcOpticalFlowPyrLK( grey1, grey2, 
			  pyrA, pyrB, 
			  points1_cv, points2_cv, 
			  count, 
			  cvSize( win_size, win_size ), 
			  5, features_found, feature_errors,
			  cvTermCriteria( CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, 0.3 ), 0 );
  
  // return results
  libopencv_(Main_opencvPoints2torch)(points2_cv, count, points2);
  int i;
  if (ff != 0){
    for(i=0;i<count;i++){
      THTensor_(set1d)(ff,i,features_found[i]);
    }
  }
  if (fe != 0){
    for(i=0;i<count;i++){
      THTensor_(set1d)(fe,i,feature_errors[i]);
    }
  }
  // Deallocate points_cv
  cvFree(&points1_cv);
  cvFree(&points2_cv);
  cvReleaseImage( &pyrA );
  cvReleaseImage( &pyrB );
  cvReleaseImage( &grey1 );
  cvReleaseImage( &grey2);
  cvReleaseImage( &image1_ipl );
  cvReleaseImage( &image2_ipl );

  return 0;
}

//============================================================
// draws red flow lines on an image (for visualizing the flow)
static int libopencv_(Main_cvDrawFlowlinesOnImage) (lua_State *L) {
  THTensor * points1 = luaT_checkudata(L,1, torch_(Tensor_id));  
  THTensor * points2 = luaT_checkudata(L,2, torch_(Tensor_id));  
  THTensor * image   = luaT_checkudata(L,3, torch_(Tensor_id));  
  THTensor * color   = luaT_checkudata(L,4, torch_(Tensor_id));  
  THTensor * mask    = NULL;
  int usemask = 0;
  if (!lua_isnil(L,5)){
    usemask = 1;
    mask = luaT_checkudata(L,5, torch_(Tensor_id));
  }
  IplImage * image_ipl = libopencv_(Main_torch2opencv_8U)(image);
  CvScalar color_cv = CV_RGB(THTensor_(get1d)(color,0),
			     THTensor_(get1d)(color,1),
			     THTensor_(get1d)(color,2));
  int count = points1->size[0];
  int i;
  for( i = 0; i < count; i++ ) {
    if ( !usemask || (THTensor_(get1d)(mask,i) > 0)){
      CvPoint p0 = cvPoint( cvRound( THTensor_(get2d)(points1,i,0)),
			    cvRound( THTensor_(get2d)(points1,i,1)));
      CvPoint p1 = cvPoint( cvRound( THTensor_(get2d)(points2,i,0)),
			    cvRound( THTensor_(get2d)(points2,i,1)));
      cvLine( image_ipl, p0, p1, color_cv, 2, CV_AA, 0);
    }
  }
  // return results
  libopencv_(Main_opencv8U2torch)(image_ipl, image);
  cvReleaseImage( &image_ipl );
  return 0;
}
/*
 * to create a smooth flow map from the dense tracking:
 *  -- compute voronoi tessalation around sparse input points
 *  -- interpolate to fill each triangle
 *  -- return dense field
 */
static int libopencv_(Main_smoothVoronoi) (lua_State *L) {
  THTensor * points1 = luaT_checkudata(L,1, torch_(Tensor_id));  
  THTensor * data    = luaT_checkudata(L,2, torch_(Tensor_id));  
  THTensor * output   = luaT_checkudata(L,3, torch_(Tensor_id));

  CvRect rect = { 0, 0, 100+output->size[2], 100+output->size[1] };
  printf("rect: (%d,%d,%d,%d)\n",
         rect.x,rect.y,rect.width,rect.height);
  CvMemStorage* storage;
  CvSubdiv2D* subdiv;

  storage = cvCreateMemStorage(0);
  subdiv = cvCreateSubdiv2D( CV_SEQ_KIND_SUBDIV2D, sizeof(*subdiv),
                             sizeof(CvSubdiv2DPoint),
                             sizeof(CvQuadEdge2D),
                             storage );
  cvInitSubdivDelaunay2D( subdiv, rect );

  int count = points1->size[0];
  int i;
  for( i = 0; i < count; i++ ) {
    CvPoint2D32f fp = cvPoint2D32f((double)THTensor_(get2d)(points1,i,0),
                                   (double)THTensor_(get2d)(points1,i,1));
    printf("[%d] (%f, %f)\n",i,fp.x,fp.y);
    cvSubdivDelaunay2DInsert( subdiv, fp );
  }
  cvCalcSubdivVoronoi2D( subdiv );

  // now do smoothing
  cvReleaseMemStorage( &storage );

  return 0;
}
//============================================================
// Register functions in LUA
//
static const luaL_reg libopencv_(Main__) [] = 
{
  {"smoothVoronoi",        libopencv_(Main_smoothVoronoi)},
  {"drawFlowlinesOnImage", libopencv_(Main_cvDrawFlowlinesOnImage)},
  {"TrackPyrLK",           libopencv_(Main_cvTrackPyrLK)},
  {"CalcOpticalFlowPyrLK", libopencv_(Main_cvCalcOpticalFlowPyrLK)},
  {"CalcOpticalFlow",      libopencv_(Main_cvCalcOpticalFlow)},
  {"CornerHarris",         libopencv_(Main_cvCornerHarris)},
  {"GoodFeaturesToTrack",  libopencv_(Main_cvGoodFeaturesToTrack)},
  {"test_torch2IPL32F",    libopencv_(Main_testTH2IPL32F)},
  {"test_torch2IPL8U",     libopencv_(Main_testTH2IPL8U)},
  {NULL, NULL}  /* sentinel */
};

DLL_EXPORT int libopencv_(Main_init) (lua_State *L) {
  luaT_pushmetaclass(L, torch_(Tensor_id));
  luaT_registeratname(L, libopencv_(Main__), "libopencv");
  return 1; 
}

#endif
