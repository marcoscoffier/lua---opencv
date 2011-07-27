#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/opencv.c"
#else

//===========================================================================
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
//===========================================================================

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

#include <cv.h>
#include <highgui.h>

#define CV_NO_BACKWARD_COMPATIBILITY

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

static int libopencv_(Main_testTH2IPL8U)(lua_State *L) {
  THTensor * src  = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * dst  = luaT_checkudata(L, 2, torch_(Tensor_id));  
  IplImage * ipl = libopencv_(Main_torch2opencv_8U)(src);
  real *src_data = THTensor_(data)(src);

  libopencv_(Main_opencv8U2torch)(ipl, dst);
  real *dst_data = THTensor_(data)(dst);
  /*
  printf("src (%f,%f,%f) ", src_data[0],src_data[1],src_data[2]);
  printf("ipl (%f,%f,%f)", 
	 ((uchar)ipl->imageData[2])/255.0,
	 ((uchar)ipl->imageData[2+3])/255.0,
	 ((uchar)ipl->imageData[2+6])/255.0);
  printf("dst(%f,%f,%f)\n", dst_data[0],dst_data[1],dst_data[2]);
  */
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
  /*
  printf("src (%f,%f,%f) ", src_data[0],src_data[1],src_data[2]);
  printf("ipl (%f,%f,%f)", 
	 ((uchar)ipl->imageData[2])/255.0,
	 ((uchar)ipl->imageData[2+3])/255.0,
	 ((uchar)ipl->imageData[2+6])/255.0);
  printf("dst(%f,%f,%f)\n", dst_data[0],dst_data[1],dst_data[2]);
  */
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


#if 0
//============================================================
// To load this lib in LUA:
// require 'libopencv'

//============================================================
// Conversion helpers
// these functions just create an IPL header to describe 
// the tensor given (no image allocation is done here)
//
static IplImage * libopencv_(Main_doubleImage) (THTensor *source) {
  // Get size and channels
  int channels = source->size[2];
  CvSize size = cvSize(source->size[0], source->size[1]);
  
  // Create ipl header
  IplImage * ipl = cvCreateImageHeader(size, IPL_DEPTH_64F, channels);

  // Point to tensor data
  ipl->imageData = (char *)source->storage->data;
  
  // in Torch, channels are separate
  if (channels != 1) ipl->dataOrder = 1;

  return ipl;
}

static IplImage * libopencv_(Main_floatImage)(THFloatTensor *source) {
  // Get size and channels
  int channels = source->size[2];
  CvSize size = cvSize(source->size[0], source->size[1]);
  
  // Create ipl header
  IplImage * ipl = cvCreateImageHeader(size, IPL_DEPTH_32F, channels);

  // Point to tensor data
  ipl->imageData = (char *)source->storage->data;
  
  // in Torch, channels are separate
  if (channels != 1) ipl->dataOrder = 1;

  return ipl;
}

static IplImage * libopencv_(Main_charImage) (THCharTensor *source) {
  // Get size and channels
  int channels = source->size[2];
  CvSize size = cvSize(source->size[0], source->size[1]);

  // Create ipl header
  IplImage * ipl = cvCreateImageHeader(size, IPL_DEPTH_8U, channels);

  // Point to tensor data
  ipl->imageData = source->storage->data;
    
  // in Torch, channels are separate
  if (channels != 1) ipl->dataOrder = 1;

  return ipl;
}

static IplImage * libopencv_(Main_shortImage)(THShortTensor *source) {
  // Get size and channels
  int channels = source->size[2];
  CvSize size = cvSize(source->size[0], source->size[1]);

  // Create ipl header
  IplImage * ipl = cvCreateImageHeader(size, IPL_DEPTH_16S, channels);

  // Point to tensor data
  ipl->imageData = (char *)source->storage->data;
    
  // in Torch, channels are separate
  if (channels != 1) ipl->dataOrder = 1;

  return ipl;
}

//============================================================
// Conversion helpers
// These functions create an IplImage, from a torch Tensor, 
// and the other way around
//
static IplImage * libopencv_(Main_torch2opencv_8U)(THTensor *source) {
  // Pointers
  uchar * dest_data;

  // Get size and channels
  int channels = source->size[2];
  int dest_step;
  CvSize dest_size = cvSize(source->size[0], source->size[1]);

  // Create ipl image
  IplImage * dest = cvCreateImage(dest_size, IPL_DEPTH_8U, channels);

  // get pointer to raw data
  cvGetRawData(dest, (uchar**)&dest_data, &dest_step, &dest_size);

  // copy
  int x, y, k;
  // change to a single loop over aligned data
  for (k=0; y<source->size[1]; y++)
    for (x=0; x<source->size[0]; x++)
      for (y=0; k<source->size[2]; k++) {
        dest_data[ y*dest_step + x*dest->nChannels + (dest->nChannels-1)-k ]
          = (uchar)(THTensor_get3d(source, x, y, k) * 255.0);
      }

  // return freshly created IPL image
  return dest;
}

static IplImage * libopencv_(Main_torch2opencv_32F)(THTensor *source) {
  // Pointers
  float * dest_data;

  // Get size and channels
  int channels = source->size[2];
  int dest_step;
  CvSize dest_size = cvSize(source->size[0], source->size[1]);

  // Create ipl image
  IplImage * dest = cvCreateImage(dest_size, IPL_DEPTH_32F, channels);

  // get pointer to raw data
  cvGetRawData(dest, (uchar**)&dest_data, &dest_step, &dest_size);
  dest_step /= sizeof(float);

  // copy
  int x, y, k;
  for (y=0; y<source->size[1]; y++)
    for (x=0; x<source->size[0]; x++)
      for (k=0; k<source->size[2]; k++) {
        dest_data[ y*dest_step + x*dest->nChannels + (dest->nChannels-1)-k ]
          = (float)(THTensor_get3d(source, x, y, k));
      }

  // return freshly created IPL image
  return dest;
}

static THTensor * libopencv_(Main_opencv2torch_8U)(IplImage *source, THTensor *dest) {
  // Pointers
  uchar * source_data; 

  // Get pointers / info
  int source_step;
  CvSize source_size;
  cvGetRawData(source, (uchar**)&source_data, &source_step, &source_size);

  // Resize target
  THTensor_resize3d(dest, source->width, source->height, source->nChannels);

  // copy
  int x, y, k;
  // change to a single for loop
  for (k=0; y<source->height; y++)
    for (x=0; x<source->width; x++)
      for (y=0; k<source->nChannels; k++) {
        THTensor_set3d(dest, x, y, k, 
                       (double)source_data[ y*source_step + x*source->nChannels + (source->nChannels-1)-k ]);
      }

  // return freshly created IPL image
  return dest;
}

static THTensor * libopencv_(Main_opencv2torch_32F)(IplImage *source, THTensor *dest) {
  // Pointers
  float * source_data; 

  // Get pointers / info
  int source_step;
  CvSize source_size;
  cvGetRawData(source, (uchar**)&source_data, &source_step, &source_size);
  source_step /= sizeof(float);

  // Resize target
  THTensor_resize3d(dest, source->width, source->height, source->nChannels);

  // copy
  int x, y, k;
  for (y=0; y<source->height; y++)
    for (x=0; x<source->width; x++)
      for (k=0; k<source->nChannels; k++) {
        THTensor_set3d(dest, x, y, k, 
                       (double)source_data[ y*source_step + x*source->nChannels + (source->nChannels-1)-k ]);
      }

  // return freshly created IPL image
  return dest;
}


static CvPoint2D32f * libopencv_(Main_torch_32F2opencvPoints)(THTensor *src) {

  int count = src->size[0];
  // create output
  CvPoint2D32f * points_cv = 0;
  points_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points_cv[0]));

  // copy
  int p;
  for (p=0; p<count; p++){
    points_cv[p].x = THTensor_get2d(src, p, 0);
    points_cv[p].y = THTensor_get2d(src, p, 1);
  }

  // return freshly created IPL image
  return points_cv;
}


//============================================================
// Wrapper around simple OpenCV functions
// All these functions work on the Lua stack
// Input and output tensors must be provided, usually in the
// correct format (char/float/double...)
//
static int libopencv_(Main_cvCanny) (lua_State *L) {
  // Get Tensor's Info
  THCharTensor * source = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THCharTensor * dest   = luaT_checkudata(L, 2, torch_(Tensor_id));  

  // Generate IPL headers
  IplImage * source_ipl = charImage(source);
  IplImage * dest_ipl   = charImage(dest);

  // Thresholds with default values
  double low_threshold = 0;
  double high_threshold = 1;
  int aperture_size = 3;
  if (lua_isnumber(L, 3)) low_threshold = lua_tonumber(L, 3);
  if (lua_isnumber(L, 4)) high_threshold = lua_tonumber(L, 4);
  if (lua_isnumber(L, 5)) aperture_size = lua_tonumber(L, 5);

  // Simple call to CV function
  cvCanny(source_ipl, dest_ipl, low_threshold, high_threshold, aperture_size );
  cvScale(dest_ipl, dest_ipl, 0.25, 0);

  // Deallocate headers
  cvReleaseImageHeader(&source_ipl);
  cvReleaseImageHeader(&dest_ipl);

  return 0;
}

static int libopencv_(Main_cvSobel) (lua_State *L) {
  // Get Tensor's Info
  THCharTensor * source = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THShortTensor * dest  = luaT_checkudata(L, 2, torch_(Tensor_id));  

  // Generate IPL headers
  IplImage * source_ipl = charImage(source);
  IplImage * dest_ipl   = shortImage(dest);

  // Thresholds with default values
  int dx = 0;
  int dy = 1;
  int aperture_size = 3;
  if (lua_isnumber(L, 3)) dx = lua_tonumber(L, 3);
  if (lua_isnumber(L, 4)) dy = lua_tonumber(L, 4);
  if (lua_isnumber(L, 5)) aperture_size = lua_tonumber(L, 5);

  // Simple call to CV function
  cvSobel(source_ipl, dest_ipl, dx, dy, aperture_size);

  // Deallocate headers
  cvReleaseImageHeader(&source_ipl);
  cvReleaseImageHeader(&dest_ipl);

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
    THTensor_resize1d(ff,count);
  }
  if (!lua_isnil(L,7)) {
    fe = luaT_checkudata(L,7,torch_(Tensor_id));  
    THTensor_resize1d(fe,count);
  }

  CvSize dest_size = cvSize(image1->size[0], image1->size[1]);
  IplImage * image1_ipl = torch2opencv_8U(image1);
  IplImage * image2_ipl = torch2opencv_8U(image2);
  

  IplImage * grey1 = cvCreateImage( dest_size, 8, 1 );
  IplImage * grey2 = cvCreateImage( dest_size, 8, 1 );

  cvCvtColor( image1_ipl, grey1, CV_BGR2GRAY );
  cvCvtColor( image2_ipl, grey2, CV_BGR2GRAY );
  CvPoint2D32f* points1_cv = torch_32F2opencvPoints(points1);
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
  opencvPoints2torch_32F(points2_cv, count, points2);
  int i;
  if (ff != 0){
    for(i=0;i<count;i++){
      THTensor_set1d(ff,i,features_found[i]);
    }
  }
  if (fe != 0){
    for(i=0;i<count;i++){
      THTensor_set1d(fe,i,feature_errors[i]);
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
static int libopencv_(Main_cvCalcOpticalFlowPyrLK) (lua_State *L) {
  // Get Tensor's Info
  THTensor * image1 = luaT_checkudata(L, 1, torch_(Tensor_id));  
  THTensor * image2 = luaT_checkudata(L, 2, torch_(Tensor_id));  
  THTensor * flow_x = luaT_checkudata(L, 3, torch_(Tensor_id));  
  THTensor * flow_y = luaT_checkudata(L, 4, torch_(Tensor_id));  
  THTensor * points = luaT_checkudata(L, 5, torch_(Tensor_id));  
  THTensor * image_out = luaT_checkudata(L, 6, torch_(Tensor_id));  
  

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

  CvSize dest_size = cvSize(image1->size[0], image1->size[1]);
  IplImage * image1_ipl = torch2opencv_8U(image1);
  IplImage * image2_ipl = torch2opencv_8U(image2);
  IplImage * image_out_ipl = torch2opencv_8U(image_out);


  IplImage * grey1 = cvCreateImage( dest_size, 8, 1 );
  IplImage * grey2 = cvCreateImage( dest_size, 8, 1 );

  cvCvtColor( image1_ipl, grey1, CV_BGR2GRAY );
  cvCvtColor( image2_ipl, grey2, CV_BGR2GRAY );
  CvPoint2D32f* points1_cv = 0;
  CvPoint2D32f* points2_cv = 0;


  IplImage* eig = cvCreateImage( dest_size, 32, 1 );
  IplImage* temp = cvCreateImage( dest_size, 32, 1 );

  // FIXME reuse points
  points1_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points1_cv[0]));
  points2_cv = (CvPoint2D32f*)cvAlloc(count*sizeof(points2_cv[0]));

  cvGoodFeaturesToTrack( grey1, eig, temp, points1_cv, &count,
			 quality, min_distance, 0, 3, 0, 0.04 );
  
  cvFindCornerSubPix( grey1, points1_cv, count,
		      cvSize(win_size,win_size), 
		      cvSize(-1,-1),
		      cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,
				     20,0.03));
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
	THTensor_set2d(flow_x,p1.x,p1.y,points1_cv[i].x - points2_cv[i].x);
	THTensor_set2d(flow_y,p1.x,p1.y,points1_cv[i].y - points2_cv[i].y);
      }
    }
  }
  
  // return results
  opencvPoints2torch_32F(points2_cv, count, points);
  opencv2torch_8U(image_out_ipl, image_out);

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
// draws red flow lines on an image (for visualizing the flow)
static int libopencv_(Main_cvDrawFlowlinesOnImage) (lua_State *L) {
  THTensor * points1 = luaT_checkudata(L,1, torch_(Tensor_id));  
  THTensor * points2 = luaT_checkudata(L,2, torch_(Tensor_id));  
  THTensor * image   = luaT_checkudata(L,3, torch_(Tensor_id));  
  THTensor * color   = luaT_checkudata(L,4, torch_(Tensor_id));  
  THTensor * mask    = 0;
  int usemask = 0;
  if (!lua_isnil(L,5)){
    usemask = 1;
    mask = luaT_checkudata(L,5, torch_(Tensor_id));
  }
  IplImage * image_ipl = torch2opencv_8U(image);
  CvScalar color_cv = CV_RGB(THTensor_get1d(color,0),
			     THTensor_get1d(color,1),
			     THTensor_get1d(color,2));
  int count = points1->size[0];
  int i;
  for( i = 0; i < count; i++ ) {
    if ( !usemask || (THTensor_get1d(mask,i) > 0)){
      CvPoint p0 = cvPoint( cvRound( THTensor_get2d(points1,i,0)),
			    cvRound( THTensor_get2d(points1,i,1)));
      CvPoint p1 = cvPoint( cvRound( THTensor_get2d(points2,i,0)),
			    cvRound( THTensor_get2d(points2,i,1)));
      cvLine( image_ipl, p0, p1, color_cv, 2, CV_AA, 0);
    }
  }
  // return results
  opencv2torch_8U(image_ipl, image);
  cvReleaseImage( &image_ipl );
  return 0;
}



//============================================================
// Other converters
// a bit redundant now, these two functions swap from RGB
// to BGR
//
IplImage * libopencv_(Main_torchRGBtoOpenCVBGR)(IplImage * source) {
  uchar * source_data; 
  uchar * dest_data;
  int source_step, dest_step;
  CvSize source_size, dest_size;
  
  // create destination image
  IplImage * dest = cvCreateImage(cvSize(source->width, source->height),
                                  IPL_DEPTH_8U, source->nChannels);

  // get pointer to raw data
  cvGetRawData(source, (uchar**)&source_data, &source_step, &source_size);
  cvGetRawData(dest, (uchar**)&dest_data, &dest_step, &dest_size);

  // copy
  int x, y, k;
  for (y=0; y<source->height; y++)
    for (x=0; x<source->width; x++)
      for (k=0; k<source->nChannels; k++) {
        dest_data[ y*dest_step + x*dest->nChannels + 2-k ]
          = source_data[ k*source->width*source->height + y*source->width + x ];
      }
  
  return dest;
}

void libopencv_(Main_openCVBGRtoTorchRGB)(IplImage * source, IplImage * dest) {
  uchar * source_data; 
  double * dest_data;
  int source_step, dest_step;
  CvSize source_size, dest_size;

  // get pointer to raw data
  cvGetRawData(source, (uchar**)&source_data, &source_step, &source_size);
  cvGetRawData(dest, (uchar**)&dest_data, &dest_step, &dest_size);

  // copy
  int x, y, k;
  for (y=0; y<source->height; y++)
    for (x=0; x<source->width; x++)
      for (k=0; k<source->nChannels; k++) {
        dest_data[ k*dest->width*dest->height + y*dest->width + x ]
          = (double)source_data[ y*source_step + x*source->nChannels + 2-k ] / 255.0;
      }

  // set this flag
  dest->dataOrder = 1;
}

//============================================================
// HaarDetectObjects
// Simple object detector based on haar features
//
static int libopencv_(Main_cvHaarDetectObjects) (lua_State *L) {
  // Generate IPL header from tensor
  THCharTensor * input = luaT_checkudata(L, 1, torch_(Tensor_id));
  IplImage * image = charImage(input);

  // Invert channels 
  IplImage * image_interleaved = torchRGBtoOpenCVBGR(image);

  // Get cascade path
  const char * path_to_cascade = luaL_checkstring(L, 2);;
  CvHaarClassifierCascade * cascade 
    = (CvHaarClassifierCascade *) cvLoad(path_to_cascade, 0, 0, 0);
  if (cascade == NULL) {
    perror("file doesnt exist, exiting");
    lua_pushnil(L);
    return 1;
  }

  /* if the flag is specified, down-scale the input image to get a
     performance boost w/o loosing quality (perhaps) */
  int i, scale = 1;
  IplImage* small_image = image_interleaved;
  int do_pyramids = 1;
  if( do_pyramids ) {
    small_image = cvCreateImage( cvSize(image_interleaved->width/2,image_interleaved->height/2),
                                 IPL_DEPTH_8U, 3 );
    cvPyrDown( image_interleaved, small_image, CV_GAUSSIAN_5x5 );
    scale = 2;
  }

  /* use the fastest variant */
  CvSeq* faces;
  CvMemStorage* storage = cvCreateMemStorage(0);
#if (CV_MINOR_VERSION >= 2)
  faces = cvHaarDetectObjects( small_image, cascade, storage, 1.2, 3, 0,
                               cvSize(30,30), cvSize(small_image->width,small_image->height) );
#else
  faces = cvHaarDetectObjects( small_image, cascade, storage, 1.2, 3, 0, cvSize(30,30));
#endif
  return 0;

  /* extract all the rectangles, and add them on the stack */
  lua_newtable(L);
  for( i = 0; i < faces->total; i++ ) {
    /* extract the rectanlges only */
    CvRect face_rect = *(CvRect*)cvGetSeqElem( faces, i );
    
    printf("face at (%d,%d)-(%d,%d)", 
           face_rect.x*scale,
           face_rect.y*scale,
           face_rect.x*scale + face_rect.width*scale,
           face_rect.y*scale + face_rect.height*scale );
    
    // and push result on the stack
    lua_pushnumber(L,i*4+1);
    lua_pushnumber(L,face_rect.x*scale);
    lua_settable(L,-3);
    lua_pushnumber(L,i*4+2);
    lua_pushnumber(L,face_rect.y*scale);
    lua_settable(L,-3);
    lua_pushnumber(L,i*4+3);
    lua_pushnumber(L,face_rect.width*scale);
    lua_settable(L,-3);
    lua_pushnumber(L,i*4+4);
    lua_pushnumber(L,face_rect.height*scale);
    lua_settable(L,-3);
  }
  
  // Cleanup
  if( small_image != image_interleaved ) cvReleaseImage( &small_image );
  cvReleaseMemStorage( &storage );
  cvReleaseHaarClassifierCascade( &cascade );
  cvReleaseImageHeader( &image );
  cvReleaseImage( &image_interleaved );

  return 1; // the table contains the results
}

//============================================================
// CaptureFromCAM
// wrapper around the Cv camera interface
//
static CvCapture *libopencv_(Main_camera) = 0;
static int libopencv_(Main_cvCaptureFromCAM) (lua_State *L) {
  // Get Tensor's Info
  THTensor * image = luaT_checkudata(L, 1, torch_(Tensor_id));  

  // idx
  int camidx = 0;
  if (lua_isnumber(L, 2)) camidx = lua_tonumber(L, 2);

  // get camera
  if( libopencv_(Main_camera) == 0 ) {
    printf("opencv: starting capture on device %d\n", camidx);
    libopencv_(Main_camera) = cvCaptureFromCAM(camidx);
    if (!libopencv_(Main_camera)) perror("Could not initialize capturing...\n");
  }

  // grab frame
  IplImage *frame = NULL;
  frame = cvQueryFrame(libopencv_(Main_camera));
  if (!frame) perror("Could not get frame...\n");

  // resize given tensor
  if (image->size[1]!=frame->width 
      || image->size[2]!=frame->height || image->size[3]!=3) {
     THTensor_resize3d(image,frame->width,frame->height,3);
  }

  // Generate IPL headers
  IplImage * curr_image = doubleImage(image);

  // copy this frame to torch format
  openCVBGRtoTorchRGB(frame, curr_image);

  // Deallocate headers
  cvReleaseImageHeader(&curr_image);
  
  return 0;
}

static int libopencv_(Main_cvReleaseCAM) (lua_State *L) {
  cvReleaseCapture( &libopencv_(Main_camera) );
  return 0;
}

#endif // 0

//============================================================
// Register functions in LUA
//
static const luaL_reg libopencv_(Main__) [] = 
{
  /* {"canny",                libopencv_(Main_cvCanny)}, */
  /* {"sobel",                libopencv_(Main_cvSobel)}, */
  //  {"HoG", libopencv_(Main_cvhog},
  /* {"captureFromCam",       libopencv_(Main_cvCaptureFromCAM)}, */
  /* {"releaseCam",           libopencv_(Main_cvReleaseCAM)}, */
  /* {"haarDetectObjects",    libopencv_(Main_cvHaarDetectObjects)}, */
  /* {"TrackPyrLK",           libopencv_(Main_cvTrackPyrLK)}, */
  /* {"calcOpticalFlowPyrLK", libopencv_(Main_cvCalcOpticalFlowPyrLK)}, */
  /* {"drawFlowlinesOnImage", libopencv_(Main_cvDrawFlowlinesOnImage)}, */
  {"CalcOpticalFlow",       libopencv_(Main_cvCalcOpticalFlow)},
  {"CornerHarris",          libopencv_(Main_cvCornerHarris)},
  {"GoodFeaturesToTrack",   libopencv_(Main_cvGoodFeaturesToTrack)},
  {"test_torch2IPL32F",     libopencv_(Main_testTH2IPL32F)},
  {"test_torch2IPL8U",      libopencv_(Main_testTH2IPL8U)},
  {NULL, NULL}  /* sentinel */
};

DLL_EXPORT int libopencv_(Main_init) (lua_State *L) {
  luaT_pushmetaclass(L, torch_(Tensor_id));
  luaT_registeratname(L, libopencv_(Main__), "libopencv");
  return 1; 
}

#endif
