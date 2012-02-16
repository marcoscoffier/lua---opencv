# a simple wrapper for certain funcs from the OpenCV library

       URL:http://opencv.willowgarage.com/

## currently wrapped packages

 + opencv.GetAffineTransform() [lua]    --> cvGetAffineTransform [C/C++]
 + opencv.WarpAffine() [lua]            --> cvWarpAffine [C/C++]
 + opencv.EqualizeHist() [lua]          --> cvEqualizeHist [C/C++]
 + opencv.Canny() [lua]                 --> cvCanny [C/C++]
 + opencv.EqualizeHist() [lua]          --> cvEqualizeHist [C/C++]
 + opencv.Canny() [lua]                 --> cvCanny [C/C++]
 + opencv.CornerHarris() [lua]          --> cvCornerHarris [C/C++]

 + opencv.CalcOpticalFlow() [lua]       -->
   - cvCalcOpticalFlowBM [C/C++]
   - cvCalcOpticalFlowHS [C/C++]
   - cvCalcOpticalFlowLK [C/C++]

 + opencv.CalcOpticalFlowPyrLK [lua]    --> cvCalcOpticalFlowPyrLK [C/C++]

## who

 + Original wrapper: Clement Farabet.
 + Additional functions: GoodFeatures...(),PLK,etc.: Marco Scoffier
 + Adapted for torch7: Marco Scoffier
