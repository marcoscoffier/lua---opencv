#include <TH.h>
#include <luaT.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>

/* These are containers for the VideoFromFile and VideoFromCam code */
#define MAXFOPEN 100
static CvCapture* vcapture[MAXFOPEN];
static IplImage*     frame[MAXFOPEN];
static int         nextvid[MAXFOPEN];
static int           fidx = 0;

/* These are containers for the VideoWriting */
#define MAXWOPEN 10
static CvVideoWriter* vwriter[MAXWOPEN];
static int         nextwriter[MAXWOPEN];
static int              widx = 0;

#ifdef DEBUG
static int print_nextvid(lua_State *L){
  int i;
  for (i=0;i<MAXFOPEN;i++){
    i==fidx?printf("[%02d]",nextvid[i]):printf("<%02d>",nextvid[i]);
  }
  printf("\n");
  return 0;
}
#endif

static int libopencv_videoOpenCamera(lua_State *L) {
  int ret = 0;
  // max file open ?
  if (fidx == MAXFOPEN) {
    THError("max nb of devices open...");
    ret = -1;
    goto free_and_return;
  }
  int idx = -1;
  if (lua_isnumber(L, 1)) {
    idx = lua_tonumber(L, 1);
    printf("opening camera device: %d\n", idx);
  }
  vcapture[fidx] = cvCaptureFromCAM(idx);
  if( vcapture[fidx] == NULL ) {
    THError("Could not open video file");
    ret = -1;
    goto free_and_return;
  }

  frame[fidx] = cvQueryFrame ( vcapture[fidx] );

  if ( frame[fidx] == NULL ) {
    THError("Failed to load first frame");
    ret = -1;
    goto free_and_return;
  }

 free_and_return:
  /* return the id for the video just opened*/
  if (ret == 0){
    lua_pushnumber(L, fidx);
    fidx = nextvid[fidx];
  }else{
    lua_pushnumber(L,-1);
  }
  return 1;
}

static int libopencv_videoLoadFile(lua_State *L) {
  int ret = 0;
  // max file open ?
  if (fidx == MAXFOPEN) {
    THError("max nb of files open...");
    ret = -1;
    goto free_and_return;
  }

  if (lua_isstring(L, 1)) {
    const char *fname = lua_tostring(L, 1);
    printf("opening video file: %s\n", fname);
    vcapture[fidx] = cvCaptureFromFile(fname);
    if( vcapture[fidx] == NULL ) {
      THError("Could not open video file");
      ret = -1;
      goto free_and_return;
    }
    frame[fidx] = cvQueryFrame ( vcapture[fidx] );

    if ( frame[fidx] == NULL ) {
      THError("Failed to load first frame");
      ret = -1;
      goto free_and_return;
    }
  } else {
    // ARG error
    THError("Argument error need to pass a filename as first arg");
    ret = -1;
    goto free_and_return;
  }

 free_and_return:
  /* return the id for the video just opened*/
  if (ret == 0){
    lua_pushnumber(L, fidx);
    fidx = nextvid[fidx];
  }else{
    lua_pushnumber(L,-1);
  }
  return 1;
}


static int fileopen(int cidx){
  // is vid file open ?
  if ((cidx > MAXFOPEN)||(vcapture[cidx] == NULL)) {
    return 0;
  } else {
    return 1;
  }
}

/*
 * close a video file
 */
static int libopencv_videoCloseFile(lua_State *L) {
  int cidx = MAXFOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!fileopen(cidx)) {
    THError("Can't close file: no open video at index");
    goto free_and_return;
  }
  if (vcapture[cidx] != NULL) {
    cvReleaseCapture(&vcapture[cidx]);
    /* return vid so that it can be reused */
    vcapture[cidx]   = NULL;
    nextvid[cidx] = fidx;
    fidx          = cidx;
  }
 free_and_return:
  return 0;
}

static int libopencv_videoWriter(lua_State *L) {
  int ret = 0;
  const char *fname;
  const char *fccstr;
  double fps = 10.0;
  int fourcc;
  int width, height;
  CvSize frame_size;
  // max file open ?
  if (widx == MAXWOPEN) {
    THError("max nb of files open...");
    ret = -1;
    goto free_and_return;
  }
  /* Parse arguments */
  if (lua_isstring(L, 1)) {
    fname      = lua_tostring(L, 1);
  } else {
    // ARG error
    THError("\n\nUsage: videoWriter(<filename>,<w>,<h>,[<fps>,<fourcc>])\n 1st arg is a filename\n");
    ret = -1;
    goto free_and_return; 
  }
    
  printf("Opening video output file: %s\n", fname);
  if (lua_isnumber(L,2) && lua_isnumber(L,3)) {
    width      = lua_tonumber(L,2);
    height     = lua_tonumber(L,3);
  } else {
    // ARG error
    THError("\n\nUsage: videoWriter(<filename>,<w>,<h>,[<fps>,<fourcc>])\n 2,3 args are width and height\n");
    ret = -1;
    goto free_and_return; 
  }
  frame_size = cvSize(width,height);
  printf(" (width,height) = (%d,%d)\n", width, height);

  /* optional arguments */
  
  if (lua_isnumber(L,4)){
    fps        = lua_tonumber(L,4);
  }
  printf("Using FPS: %f\n",fps);
  
  /* http://ffmpeg.org/doxygen/trunk/isom_8c-source.html */
  if (lua_isstring(L, 5)) {
    fccstr     = lua_tostring(L, 5);
    fourcc     = CV_FOURCC(fccstr[0],fccstr[1],fccstr[2],fccstr[3]); 
    printf("Using fourcc: %s\n",fccstr);
  } else {
    fourcc     = CV_FOURCC('m','j','p','g');
    printf("Using fourcc: 'mjpg'\n");
  }
  vwriter[widx] = cvCreateVideoWriter(fname, fourcc, fps, frame_size, 1);
  if( vwriter[widx] == NULL ) {
      THError("\nCould not open video file");
      ret = -1;
      goto free_and_return;
    }
  
 free_and_return:
  /* return the id for the video just opened*/
  if (ret == 0){
    lua_pushnumber(L, widx);
    widx = nextwriter[widx];
  }else{
    lua_pushnumber(L,-1);
  }
  return 1;
}

static int writeropen(int cidx){
  // is vid file open ?
  if ((cidx > MAXWOPEN)||(vwriter[cidx] == NULL)) {
    return 0;
  } else {
    return 1;
  }
}

/*
 * close a video writer file
 */
static int libopencv_videoCloseWriter(lua_State *L) {
  int cidx = MAXWOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!writeropen(cidx)) {
    THError("Can't close file: no open video at index");
    goto free_and_return;
  }
  if (vwriter[cidx] != NULL) {
    cvReleaseVideoWriter(&vwriter[cidx]);
    /* return vid so that it can be reused */
    vwriter[cidx]    = NULL;
    nextwriter[cidx] = widx;
    widx             = cidx;
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
  int cidx = MAXFOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!fileopen(cidx)) {
    THError("can't seek: no open video at index");
    goto free_and_return;
  }

  if (lua_isnumber(L,2)){
    msec = lua_tonumber(L,2) * 1000.0;
  }
  psec = msec - 500.0;
  if (psec < 0){
    psec = 0;
  }

  cvSetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC,psec);
  sprop = cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC);
  fps   = cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FPS);
  mspf   = 1000/fps;

  /* Make sure that the seek has put us before the frame we actually
     want. */
  while ((sprop > msec) && (psec > 500)){
    psec -= 500;
    cvSetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC,psec);
    sprop = cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC);
  }

  for (i=0;i<fps+1;i++){
    /* Get new frame */
    ret = cvGrabFrame(vcapture[cidx]);
    if (ret != 1){
      THError("cvGrabFrame failed.");
      goto free_and_return;
    }
    /* Check current timecode */
    sprop = cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC);
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

  int cidx = MAXFOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!fileopen(cidx)) {
    THError("Can't dump properties: no open video at index");
    goto free_and_return;
  }

  printf("[%d]CV_CAP_PROP_POS_MSEC: %f\n",
         CV_CAP_PROP_POS_MSEC,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC));
  printf("[%d]CV_CAP_PROP_POS_FRAMES: %f\n",
         CV_CAP_PROP_POS_FRAMES,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_FRAMES));
  printf("[%d]CV_CAP_PROP_POS_AVI_RATIO: %f\n",
         CV_CAP_PROP_POS_AVI_RATIO,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_AVI_RATIO));
  printf("[%d]CV_CAP_PROP_FRAME_WIDTH: %f\n",
         CV_CAP_PROP_FRAME_WIDTH,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FRAME_WIDTH ));
  printf("[%d]CV_CAP_PROP_FRAME_HEIGHT: %f\n",
         CV_CAP_PROP_FRAME_HEIGHT,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FRAME_HEIGHT));
  printf("[%d]CV_CAP_PROP_FPS: %f\n",
         CV_CAP_PROP_FPS,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FPS));
  printf("[%d]CV_CAP_PROP_FOURCC: %f\n",
         CV_CAP_PROP_FOURCC,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FOURCC));
  printf("[%d]CV_CAP_PROP_FRAME_COUNT: %f\n",
         CV_CAP_PROP_FRAME_COUNT,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FRAME_COUNT));
  printf("[%d]CV_CAP_PROP_FORMAT: %f\n",
         CV_CAP_PROP_FORMAT,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FORMAT));
  printf("[%d]CV_CAP_PROP_MODE: %f\n",
         CV_CAP_PROP_MODE,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_MODE));
  printf("[%d]CV_CAP_PROP_BRIGHTNESS: %f\n",
         CV_CAP_PROP_BRIGHTNESS,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_BRIGHTNESS));
  printf("[%d]CV_CAP_PROP_CONTRAST: %f\n",
         CV_CAP_PROP_CONTRAST,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_CONTRAST));
  printf("[%d]CV_CAP_PROP_SATURATION: %f\n",
         CV_CAP_PROP_SATURATION,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_SATURATION));
  printf("[%d]CV_CAP_PROP_HUE: %f\n",
         CV_CAP_PROP_HUE,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_HUE));
  printf("[%d]CV_CAP_PROP_GAIN: %f\n",
         CV_CAP_PROP_GAIN,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_GAIN));
  printf("[%d]CV_CAP_PROP_EXPOSURE: %f\n",
         CV_CAP_PROP_EXPOSURE,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_EXPOSURE));
  printf("[%d]CV_CAP_PROP_CONVERT_RGB: %f\n",
         CV_CAP_PROP_CONVERT_RGB,
         cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_CONVERT_RGB));
 free_and_return:
  return 0;
}

/*
 * get video property
 */
static int libopencv_videoGetProperty(lua_State *L) {
  double property_val =  0;
  int    property_id  = -1;

  int cidx = MAXFOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!fileopen(cidx)) {
    THError("Can't get Property: no open video at index");
    goto free_and_return;
  }
  if (lua_isnumber(L,2)){
    property_id = lua_tonumber(L,2);
  }
  property_val = cvGetCaptureProperty(vcapture[cidx],property_id);
 free_and_return:
  lua_pushnumber(L, property_val);
  return 1;
}

/*
 * get video FPS
 */
static int libopencv_videoGetFPS(lua_State *L) {
  double property_val =  0;

  int cidx = MAXFOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!fileopen(cidx)) {
    THError("Can't get FPS: no open video at index");
    goto free_and_return;
  }
  property_val = cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_FPS);
 free_and_return:
  lua_pushnumber(L, property_val);
  return 1;
}

/*
 * get video MSEC
 */
static int libopencv_videoGetMSEC(lua_State *L) {
  double property_val =  0;

  int cidx = MAXFOPEN;
  if (lua_isnumber(L,1)){
    cidx = lua_tonumber(L,1);
  }
  // is vid file open ?
  if (!fileopen(cidx)) {
    THError("Can't get msec: no open video at index");
    goto free_and_return;
  }
  property_val = cvGetCaptureProperty(vcapture[cidx],CV_CAP_PROP_POS_MSEC);
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
  {"videoOpenCamera",     libopencv_videoOpenCamera},
  {"videoCloseFile",      libopencv_videoCloseFile},
  {"videoSeek",           libopencv_videoSeek},
  {"videoGetProperty",    libopencv_videoGetProperty},
  {"videoGetFPS",         libopencv_videoGetFPS},
  {"videoGetMSEC",        libopencv_videoGetMSEC},
  {"videoDumpProperties", libopencv_videoDumpProperties},
  {"videoWriter",         libopencv_videoWriter},
  {"videoCloseWriter",    libopencv_videoCloseWriter},
#ifdef DEBUG
  {"videoPrintNextList",  print_nextvid},
#endif
  {NULL,NULL}
};

#define torch_(NAME) TH_CONCAT_3(torch_, Real, NAME)
#define torch_Tensor        TH_CONCAT_STRING_3(torch.,Real,Tensor)
#define libopencv_(NAME) TH_CONCAT_3(libopencv_, Real, NAME)

#include "generic/opencv.c"
#include "THGenerateFloatTypes.h"

DLL_EXPORT int luaopen_libopencv(lua_State *L)
{

  luaL_register(L, "opencv", libopencv_init);

  libopencv_FloatMain_init(L);
  libopencv_DoubleMain_init(L);

  /* set up pointers to store capture objects and frames */
  int i;
  for (i=0;i<MAXFOPEN;i++){
    vcapture[i]   = NULL;
    frame[i]      = NULL;
    nextvid[i] = i+1;
  }
  for (i=0;i<MAXWOPEN;i++){
    vwriter[i]    = NULL;
    nextwriter[i] = i+1;
  }
  return 1;
}
