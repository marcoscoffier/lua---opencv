#include <TH.h>
#include <luaT.h>
#include <cv.h>
#include <highgui.h>

#define MAXFOPEN 100
static CvCapture* vfile[MAXFOPEN];
static IplImage* frame[MAXFOPEN];
static int fidx = 0;

static int libopencv_videoLoadFile(lua_State *L) {
  int ret = 0;
  // max file open ?
  if (fidx == MAXFOPEN) {
    perror("max nb of files open...\n");
    ret = -1;
    goto free_and_return;
  }
  
  if (lua_isstring(L, 1)) {
    const char *fname = lua_tostring(L, 1);
    printf("opening video file: %s\n", fname);
    vfile[fidx] = cvCaptureFromFile(fname);
    if( vfile[fidx] == NULL ) {
      perror("could not open: video file");
      ret = -1;
      goto free_and_return;
    }
    frame[fidx] = cvQueryFrame ( vfile[fidx] );

    if ( frame[fidx] == NULL ) {
      perror("failed OpenCV to load first frame");
      ret = -1;
      goto free_and_return; 
    }
  } else {
    // ARG error
    perror("Argument error need to pass a filename as first arg");
    ret = -1;
    goto free_and_return; 
  }
  
 free_and_return:
  /* return the id for the video just opened*/
  if (ret == 0){
    lua_pushnumber(L, fidx);
    fidx++;
  }else{
    lua_pushnumber(L,-1);
  }
  return 1;
}

/*
 * close a video file
 */
static int libopencv_videoCloseFile(lua_State *L) {
  int cidx = 0;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if ((cidx > fidx)||(vfile[cidx] == NULL)) {
    perror("no open video at index");
    goto free_and_return;
  }
  if (vfile[cidx]) {
    cvReleaseCapture(&vfile[cidx]);
  }
 free_and_return: 
  return 0;
}

/*
 * seek in video file by milli second
 * seeking by frame CV_CAP_PROP_POS_FRAMES is broken
 */
static int libopencv_videoSeek(lua_State *L) {
  double msec = 0, psec = 0;
  double sprop = 0, fps, mspf;
  int i, ret = 0;
  if (lua_isnumber(L,1)){
    msec = lua_tonumber(L,1) * 1000.0;
  }
  psec = msec - 500.0;
  if (psec < 0){
    psec = 0;
  }
  int cidx = 0;
  if (lua_isnumber(L,2)){
    cidx = lua_tonumber(L,2);
  }
  // is vid file open ?
  if ((cidx > fidx)||(vfile[cidx] == NULL)) {
    perror("no open video at index");
    goto free_and_return;
  }

  cvSetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC,psec);
  sprop = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC);
  fps   = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FPS);
  mspf   = 1000/fps;
  
  /* Make sure that the seek has put us before the frame we actually
     want. */
  while ((sprop > msec) && (psec > 500)){
    psec -= 500;
    cvSetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC,psec);
    sprop = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC);
  }
    
  for (i=0;i<fps+1;i++){
    /* Get new frame */
    ret = cvGrabFrame(vfile[cidx]);
    if (ret != 1){
      perror("cvGrabFrame failed.");
      goto free_and_return;
    }
    /* Check current timecode */
    sprop = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC);
    if (sprop + mspf > msec){
      /* Found the correct frame. */ 
      break;
    } /* if (sprop > msec) */
  } /* for (i=0;i<fps+1,i++) */
  
 free_and_return:
  /* return the actual msec of the frame found */
  lua_pushnumber(L, sprop);
  return 1;
}

/*
 * dump video properties
 */
static int libopencv_videoDumpProperties(lua_State *L) {
  
  int cidx = 0;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if ((cidx > fidx)||(vfile[cidx] == NULL)) {
    perror("no open video at index");
    goto free_and_return;
 }

  printf("[%d]CV_CAP_PROP_POS_MSEC: %f\n",
         CV_CAP_PROP_POS_MSEC,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC));
  printf("[%d]CV_CAP_PROP_POS_FRAMES: %f\n",
         CV_CAP_PROP_POS_FRAMES,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_FRAMES));
  printf("[%d]CV_CAP_PROP_POS_AVI_RATIO: %f\n",
         CV_CAP_PROP_POS_AVI_RATIO,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_AVI_RATIO));
  printf("[%d]CV_CAP_PROP_FRAME_WIDTH: %f\n",
         CV_CAP_PROP_FRAME_WIDTH,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FRAME_WIDTH ));
  printf("[%d]CV_CAP_PROP_FRAME_HEIGHT: %f\n",
         CV_CAP_PROP_FRAME_HEIGHT,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FRAME_HEIGHT));
  printf("[%d]CV_CAP_PROP_FPS: %f\n",
         CV_CAP_PROP_FPS,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FPS));
  printf("[%d]CV_CAP_PROP_FOURCC: %f\n",
         CV_CAP_PROP_FOURCC,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FOURCC));
  printf("[%d]CV_CAP_PROP_FRAME_COUNT: %f\n",
         CV_CAP_PROP_FRAME_COUNT,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FRAME_COUNT));
  printf("[%d]CV_CAP_PROP_FORMAT: %f\n",
         CV_CAP_PROP_FORMAT,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FORMAT));
  printf("[%d]CV_CAP_PROP_MODE: %f\n",
         CV_CAP_PROP_MODE,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_MODE));
  printf("[%d]CV_CAP_PROP_BRIGHTNESS: %f\n",
         CV_CAP_PROP_BRIGHTNESS,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_BRIGHTNESS));
  printf("[%d]CV_CAP_PROP_CONTRAST: %f\n",
         CV_CAP_PROP_CONTRAST,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_CONTRAST));
  printf("[%d]CV_CAP_PROP_SATURATION: %f\n",
         CV_CAP_PROP_SATURATION,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_SATURATION));
  printf("[%d]CV_CAP_PROP_HUE: %f\n",
         CV_CAP_PROP_HUE,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_HUE));
  printf("[%d]CV_CAP_PROP_GAIN: %f\n",
         CV_CAP_PROP_GAIN,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_GAIN));
  printf("[%d]CV_CAP_PROP_EXPOSURE: %f\n",
         CV_CAP_PROP_EXPOSURE,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_EXPOSURE));
  printf("[%d]CV_CAP_PROP_CONVERT_RGB: %f\n",
         CV_CAP_PROP_CONVERT_RGB,
         cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_CONVERT_RGB));
 free_and_return:
  return 0;
}

/*
 * get video property
 */
static int libopencv_videoGetProperty(lua_State *L) {
  double property_val =  0;
  int    property_id  = -1;

  if (lua_isnumber(L,1)){
    property_id = lua_tonumber(L,1);
  }
  
  int cidx = 0;
  if (lua_isnumber(L,2)){
    cidx = lua_tonumber(L,2);
  }
  // is vid file open ?
  if ((cidx > fidx)||(vfile[cidx] == NULL)) {
    perror("no open video at index");
    goto free_and_return; 
  }
  property_val = cvGetCaptureProperty(vfile[cidx],property_id);
 free_and_return:
  lua_pushnumber(L, property_val);
  return 1;
}

/*
 * get video FPS
 */
static int libopencv_videoGetFPS(lua_State *L) {
  double property_val =  0;
  
  int cidx = 0;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if ((cidx > fidx)||(vfile[cidx] == NULL)) {
    perror("no open video at index");
    goto free_and_return;
  }
  property_val = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FPS);
 free_and_return:
  lua_pushnumber(L, property_val);
  return 1;
}

/*
 * get video MSEC
 */
static int libopencv_videoGetMSEC(lua_State *L) {
  double property_val =  0;
  
  int cidx = 0;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if ((cidx > fidx)||(vfile[cidx] == NULL)) {
    perror("no open video at index");
    goto free_and_return;
  }
  
  property_val = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC);
 free_and_return:
  lua_pushnumber(L, property_val);
  return 1;
}

//============================================================
// Register functions in LUA
//
static const luaL_reg libopencv_init [] =
{
  {"videoLoadFile",       libopencv_videoLoadFile},
  {"videoCloseFile",      libopencv_videoCloseFile},
  {"videoSeek",           libopencv_videoSeek},
  {"videoGetProperty",    libopencv_videoGetProperty},
  {"videoGetFPS",         libopencv_videoGetFPS},
  {"videoGetMSEC",        libopencv_videoGetMSEC},
  {"videoDumpProperties", libopencv_videoDumpProperties},
  {NULL,NULL}
};

#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_string_(NAME) TH_CONCAT_STRING_3(torch., Real, NAME)
#define libopencv_(NAME) TH_CONCAT_3(libopencv_, Real, NAME)

static const void* torch_FloatTensor_id = NULL;
static const void* torch_DoubleTensor_id = NULL;

#include "generic/opencv.c"
#include "THGenerateFloatTypes.h"

DLL_EXPORT int luaopen_libopencv(lua_State *L)
{

  luaL_register(L, "opencv", libopencv_init);
  
  torch_FloatTensor_id = luaT_checktypename2id(L, "torch.FloatTensor");
  torch_DoubleTensor_id = luaT_checktypename2id(L, "torch.DoubleTensor");

  libopencv_FloatMain_init(L);
  libopencv_DoubleMain_init(L);

  luaL_register(L, "libopencv.double", libopencv_DoubleMain__);
  luaL_register(L, "libopencv.float", libopencv_FloatMain__);

  int i;
  for (i=0;i<MAXFOPEN;i++){
    vfile[i] = NULL;
    frame[i] = NULL;
  }
  return 1;
}
