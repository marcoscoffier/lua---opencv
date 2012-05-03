#include <TH.h>
#include <luaT.h>
#include <cv.h>
#include <highgui.h>

#define MAXFOPEN 10
static CvCapture* vfile[MAXFOPEN];
static IplImage* frame[MAXFOPEN];
static int fidx = -1;

static int libopencv_videoLoadFile(lua_State *L) {
  fidx ++;
  // max file open ?
  if (fidx == MAXFOPEN) {
    perror("max nb of files open...\n");
  }

  if (lua_isstring(L, 1)) {
    const char *fname = lua_tostring(L, 1);
    printf("opening video file: %s\n", fname);
    vfile[fidx] = cvCaptureFromFile(fname);
    if( vfile[fidx] == NULL ) {
      perror("could not open: video file");
    }
    frame[fidx] = cvQueryFrame ( vfile[fidx] );

    if ( frame[fidx] == NULL ) {
      perror("failed OpenCV to load first frame");
    }
  } else {
    // ARG error
    perror("Argument error need to pass a filename as first arg");
  }

  /* return the id for the video just opened*/
  lua_pushnumber(L, fidx);
  return 1;
}

/*
 * seek in video file by milli second
 * seeking by frame CV_CAP_PROP_POS_FRAMES is broken
 */
static int libopencv_videoSeek(lua_State *L) {
  double msec = 0, psec = 0;
  double sprop, fps;
  int i;
  IplImage *frame;
  if (lua_isnumber(L,1)){
    msec = lua_tonumber(L,1) * 1000.0;
  }
  psec = msec - 500.0;
  
  int cidx = 0;
  if (lua_isnumber(L,2)){
    cidx = lua_tonumber(L,2);
  }
  // is vid file open ?
  if (cidx > fidx) {
    perror("no open video at index");
  }

  cvSetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC,psec);
  sprop = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC);
  fps = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_FPS);
  
  for (i=0;i<fps+1;i++){
    /* Get new frame */
    frame = cvQueryFrame(vfile[cidx]);
    if (frame == NULL){
      perror("cvQueryFrame failed.");
    }
    /* Check current timecode */
    sprop = cvGetCaptureProperty(vfile[cidx],CV_CAP_PROP_POS_MSEC);
    if (sprop > msec){
      /* Found the correct frame. Save output rgb image. */ 
      break;
    } /* if (sprop > msec) */
  } /* for (i=0;i<fps+1,i++) */
  
  return 0;
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
  if (cidx > fidx) {
    perror("no open video at index");
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
  if (cidx > fidx) {
    perror("no open video at index");
  }
  property_val = cvGetCaptureProperty(vfile[cidx],property_id);

  printf("Prop = %f\n", property_val);
  
  lua_pushnumber(L, property_val);
  return 1;
}

//============================================================
// Register functions in LUA
//
static const luaL_reg libopencv_init [] =
{
  {"videoLoadFile",       libopencv_videoLoadFile},
  //{"videoCloseFile",      libopencv_videoCloseFile},
  {"videoSeek",           libopencv_videoSeek},
  {"videoGetProperty",    libopencv_videoGetProperty},
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

  luaL_register(L, "libopencv", libopencv_init);
  
  torch_FloatTensor_id = luaT_checktypename2id(L, "torch.FloatTensor");
  torch_DoubleTensor_id = luaT_checktypename2id(L, "torch.DoubleTensor");

  libopencv_FloatMain_init(L);
  libopencv_DoubleMain_init(L);

  luaL_register(L, "libopencv.double", libopencv_DoubleMain__);
  luaL_register(L, "libopencv.float", libopencv_FloatMain__);

  return 1;
}
