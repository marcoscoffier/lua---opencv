--
-- Note: this bit of code is a simple wrapper around the OpenCV library
--       http://opencv.willowgarage.com/
--
-- For now, it contains wrappers for:
--  + opencv.GetAffineTransform() [lua]    --> cvGetAffineTransform [C/C++]
--  + opencv.WarpAffine() [lua]            --> cvWarpAffine [C/C++]
--  + opencv.EqualizeHist() [lua]          --> cvEqualizeHist [C/C++]
--  + opencv.Canny() [lua]                 --> cvCanny [C/C++]
--  + opencv.CornerHarris() [lua] --> cvCornerHarris [C/C++]
--
--  + opencv.CalcOpticalFlow() [lua] -->
--    - cvCalcOpticalFlowBM
--    - cvCalcOpticalFlowHS
--    - cvCalcOpticalFlowLK
--
--  + opencv.GoodFeaturesToTrack() [lua] --> cvGoodFeaturesToTrack [C/C++]
--
-- Wrapper: Clement Farabet.
-- Additional functions: GoodFeatures...(),PLK,etc.: Marco Scoffier
-- Adapted for torch7: Marco Scoffier
--

require 'torch'
require 'dok'
require 'image'

opencv = {}

-- load C lib
require 'libopencv'

function opencv.StereoCorrespondenceBM(...)
   local _, left, right, minDisparity, numberOfDisparities, textureThreshold = dok.unpack(
      {...},
      'opencv.StereoCorrespondenceBM',
      [[Implements opencv StereoCorrespondenceBM using block matching algorithm
            return Tensor of computed disparities]],
      {arg='left', type='torch.Tensor',
       help='single channel left image', req=true},
      {arg='right', type='torch.Tensor',
       help='single channel right image', req=true},
      {arg='minDisparity',type='number',
       help='minimum disparity', default=1},
      {arg='numberOfDisparities',type='number',
       help='maximum disparity - minimum disparity must be a multiple of 16', default=64},
      {arg='textureThreshold',type='number',
       help='areas with no texture are ignored', default=10}
   )
   -- check left
   local leftImg = torch.Tensor(left)
   if left:nDimension() == 2 then
      leftImg = leftImg:resize(1,left:size(1),left:size(2))
   elseif leftImg:nDimension() == 3 and leftImg:size(1) == 3
   then
      leftImg=image.rgb2y(left)
   elseif leftImg:size(1) ~= 1 then
      xerror([[ *** ERROR: opencv.StereoCorrespondenceBM
                   left img must be RBG or grey scale]])
   end
   -- check right
   local rightImg = torch.Tensor(right)
   if right:nDimension() == 2 then
      rightImg = rightImg:resize(1,right:size(1),right:size(2))
   elseif rightImg:nDimension() == 3 and rightImg:size(1) == 3
   then
      rightImg=image.rgb2y(right)
   elseif rightImg:size(1) ~= 1 then
      xerror([[ *** ERROR: opencv.StereoCorrespondenceBM
                   right img must be RBG or grey scale]])
   end
   local dest = torch.Tensor()
   left.libopencv.StereoCorrespondenceBM(leftImg,rightImg,dest,
                                           minDisparity, numberOfDisparities,
                                           textureThreshold)
   return dest
   end

function opencv.StereoCorrespondenceGC(...)
   local _, left, right, maxIters, numberOfDisparities = dok.unpack(
      {...},
      'opencv.StereoCorrespondenceGC',
      [[Implements opencv StereoCorrespondenceBM using block matching algorithm
            return left disparities, right disparities]],
      {arg='left', type='torch.Tensor',
       help='single channel left image', req=true},
      {arg='right', type='torch.Tensor',
       help='single channel right image', req=true},
      {arg='maxIters',type='number',
       help='maximum # of iterations', default=2},
      {arg='numberOfDisparities',type='number',
       help='maximum disparity - minimum disparity must be a multiple of 16', default=16}
   )
   -- check left
   local leftImg = torch.Tensor(left)
   if left:nDimension() == 2 then
      leftImg = leftImg:resize(1,left:size(1),left:size(2))
   elseif leftImg:nDimension() == 3 and leftImg:size(1) == 3
   then
      leftImg=image.rgb2y(left)
   elseif leftImg:size(1) ~= 1 then
      xerror([[ *** ERROR: opencv.StereoCorrespondenceGC
                   left img must be RBG or grey scale]])
   end
   -- check right
   local rightImg = torch.Tensor(right)
   if right:nDimension() == 2 then
      rightImg = rightImg:resize(1,right:size(1),right:size(2))
   elseif rightImg:nDimension() == 3 and rightImg:size(1) == 3
   then
      rightImg=image.rgb2y(right)
   elseif rightImg:size(1) ~= 1 then
      xerror([[ *** ERROR: opencv.StereoCorrespondenceGC
                   right img must be RBG or grey scale]])
   end
   local Ldisp = torch.Tensor()
   local Rdisp = torch.Tensor()
   left.libopencv.StereoCorrespondenceGC(leftImg,rightImg,Ldisp,Rdisp,
                                           maxIters, numberOfDisparities)
   return Ldisp,Rdisp
end

-- Canny
function opencv.Canny(...)
   local _, source, percent, low_threshold, high_threshold, blursize, aperturesize = dok.unpack(
      {...},
      'opencv.Canny',
      [[Implements the Canny algorithm for edge detection.
            return Tensor of edges, low_threshold,high_threshold (thresholds used for the computation)]],
      {arg='source', type='torch.Tensor',
       help='image in which to perform edge detection', req=true},
      {arg='percent',type='number',
       help='determine automatically low and high threshold'},
      {arg='low',type='number',
       help=[[The smallest value between low and high is used
             for edge linking, the largest value is used to find the initial segments
             of strong edges. note that in opencv the pixels are in [0 255].
             also note that using percent param will ignore low and high values ]], default=50},
      {arg='high',type='number',
       help='cf. low',default=150},
      {arg='blursize',type='number',
       help='Guassian blur kernel size (<1 means no blurring)', default=0},
      {arg='aperturesize',type='number',
       help='Sobel aperture size', default=3}
   )
   local img = source
   if source:size(1) == 3 then
      print('WARNING: opencv.Canny converting image to grey')
      img=image.rgb2y(source)
   elseif source:size(1) ~= 1 then
      xerror(' *** ERROR: opencv.Canny works only on RBG or grey img')
   end
   if blursize>1 and blursize % 2 == 0 then
      print('WARNING: blursize (Guassian blur kernel size) must be odd')
      blursize = blursize -1
   end
   if aperturesize % 2 == 0 or aperturesize < 1 or aperturesize > 7 then
      print('WARNING: aperturesize (Sobel kernel size) must be odd >= 1 and <= 7')
      aperturesize = math.max(1,math.min(aperturesize -1,7))
   end
   local dest = torch.Tensor():resizeAs(img)
   low_threshold,high_threshold = img.libopencv.Canny(img,dest,low_threshold,high_threshold,blursize,aperturesize,percent)
   return dest,low_threshold,high_threshold
end


function opencv.GetAffineTransform(...)
   local args,  points_src, points_dst  = dok.unpack(
      {...},
      'opencv.GetAffineTransform',
      [[Calculates the affine transform from 3 corresponding points. ]],
      {arg='points_src',type='torch.Tensor',
       help='source points', req=true},
      {arg='points_dst',type='torch.Tensor',
       help='destination points', req=true}
   )
   local warp = torch.Tensor()
   warp.libopencv.GetAffineTransform(points_src,points_dst,warp)
   return warp
end

-- test function:
function opencv.GetAffineTransform_testme()
   src = torch.Tensor(3,2)
   dst = torch.Tensor(3,2)
   src[1][1]=0
   src[1][2]=0
   src[2][1]=511
   src[2][2]=0
   src[3][1]=0
   src[3][2]=511

   dst[1][1]=0
   dst[1][2]=512*0.25
   dst[2][1]=512*0.9
   dst[2][2]=512*0.15
   dst[3][1]=512*0.1
   dst[3][2]=512*0.75

   warp = opencv.GetAffineTransform(src,dst)
   print('Warp matrix:')
   print(warp)
end

-- WarpAffine
function opencv.WarpAffine(...)
   local _, source,warp = dok.unpack(
      {...},
      'opencv.WarpAffine',
      [[Implements the affine transform which allows the user to warp,
            stretch, rotate and resize an image.]],
      {arg='source', type='torch.Tensor',
       help='image in which to perform Histogram Equalization', req=true},
      {arg='warp', type='torch.Tensor',
       help='2x3 transformation matrix', req=true}
   )
   local img = source
   if warp:size(1) ~= 2 or warp:size(2) ~= 3 then
      xerror(' *** ERROR: opencv.WarpAffine warp Tensor must be 2x3')
   end
   local dest = torch.Tensor():resizeAs(img)
   img.libopencv.WarpAffine(img,dest,warp)
   return dest
end

-- test function:
function opencv.WarpAffine_testme(img)
   if not img then
      img = image.lena()
      image.display{image=img,legend='Original image'}
   end

   local src = torch.Tensor(3,2)
   local dst = torch.Tensor(3,2)
   src[1][1]=0
   src[1][2]=0
   src[2][1]=511
   src[2][2]=0
   src[3][1]=0
   src[3][2]=511

   dst[1][1]=0
   dst[1][2]=512*0.15
   dst[2][1]=512*0.9
   dst[2][2]=512*0.05
   dst[3][1]=512*0.1
   dst[3][2]=512*0.75

   local warp = opencv.GetAffineTransform(src,dst)
   print('warp',warp)
   local warpImg = opencv.WarpAffine(img,warp)
   image.display{image=warpImg,legend='Warped image'}
end


-- EqualizeHist
function opencv.EqualizeHist(...)
   local _, source = dok.unpack(
      {...},
      'opencv.EqualizeHist',
      'Implements the Histogram Equalization algorithm.',
      {arg='source', type='torch.Tensor',
       help='image in which to perform Histogram Equalization', req=true}
   )
   local img = source
   if source:size(1) == 3 then
      print('WARNING: opencv.EqualizeHist converting image to grey')
      img=image.rgb2y(source)
   elseif source:size(1) ~= 1 then
      xerror(' *** ERROR: opencv.EqualizeHist works only on RBG or grey img')
   end
   local dest = torch.Tensor():resizeAs(img)
   img.libopencv.EqualizeHist(img,dest)
   return dest
end

-- CornerHarris
function opencv.CornerHarris(...)
   local _, img, blocksize, aperturesize, k = dok.unpack(
      {...},
      'opencv.CornerHarris',
      'Computes the Harris Corner features of an image the input image will be converted to a WxHx1 tensor',
      {arg='img', type='torch.Tensor',
       help='image in which to detect Haar points', req=true},
      {arg='blocksize',type='number',
       help='neighborhood size', default=9},
      {arg='aperturesize',type='number',
       help='Sobel aperture size', default=3},
      {arg='k',type='number',
       help='the Harris detector free parameter',default=0.04}
   )

   local img = img
   if img:size(1) > 1 then
      print('WARNING: computing harris corners on first feature')
      img=img:narrow(1,1,1)
   end
   if aperturesize % 2 == 0 then
      print('WARNING: aperturesize (Sobel kernel size) must be odd and not larger than 31')
      aperturesize = aperturesize -1
   end
   local harris = torch.Tensor():resizeAs(img)
   img.libopencv.CornerHarris(img,harris,blocksize,aperturesize,k)
   return harris
end

function opencv.imgL()
   return image.load(sys.concat(sys.fpath(), 'img1.jpg'))
end

function opencv.imgR()
   return image.load(sys.concat(sys.fpath(), 'img2.jpg'))
end


-- test function:
function opencv.CornerHarris_testme(img)
   if not img then
      img = opencv.imgL()
      image.display{image=img,legend='Original image (Left)'}
   end
   local harris = opencv.CornerHarris(img,5,3,0.05)
   image.display{image=harris,legend='Harris Corners (Left)'}
end


-- OpticalFlow:
function opencv.CalcOpticalFlow(...)
   local args, pair, method, block_w, block_h,
   shift_x, shift_y, window_w, window_h,
   lagrangian, iterations, autoscale,
   raw, reuse, flow_x, flow_y = dok.unpack(
      {...},
      'opencv.CalcOpticalFlow',
      [[
  Computes the optical flow of a pair of images, and returns 4 maps:
  the flow field intensities, the flow field directions, and
  the raw X and Y components

  The flow field is computed using one of 3 methods:
    Block Matching (BM), Lucas-Kanade (LK) or Horn-Schunck (HS).

  The input images must be a pair of WxHx1 tensors.
      ]],
      {arg='pair', type='table',
       help='a pair of images (2 WxHx1 tensor)', req=true},
      {arg='method', type='string',
       help='method used: BM | HS | LK', default='BM'},
      {arg='block_w', type='number',
       help='matching block width (BM+LK)', default=9},
      {arg='block_h', type='number',
       help='matching block height (BM+LK)', default=9},
      {arg='shift_x', type='number',
       help='shift step in x (BM only)', default=4},
      {arg='shift_y', type='number',
       help='shift step in y (BM only)', default=4},
      {arg='window_w', type='number',
       help='matching window width (BM only)', default=30},
      {arg='window_h', type='number',
       help='matching window height (BM only)', default=30},
      {arg='lagrangian', type='number',
       help='lagrangian multiplier (HS only)', default=1},
      {arg='iterations', type='number',
       help='nb of iterations (HS only)', default=5},
      {arg='autoscale', type='boolean',
       help='auto resize results', default=true},
      {arg='raw', type='boolean',
       help='if set, returns the raw X,Y fields', default=false},
      {arg='reuse', type='boolean',
       help='reuse last flow computed (HS+BM)', default=false},
      {arg='flow_x', type='torch.Tensor',
       help='existing (previous) X-field (WxHx1 tensor)'},
      {arg='flow_y', type='torch.Tensor',
       help='existing (previous) Y-field (WxHx1 tensor)'}
   )

   if pair[1]:nDimension() ~= 3 then
      dok.error('inconsistent input size'..args.usage,
		 'opencv.CalcOpticalFlow')
   end

   local imageP = pair[1]
   local imageN = pair[2]

   if imageP:size(1) > 1 then
      print('WARNING: computing flow on first feature')
      imageP=imageP:narrow(1,1,1)
   end
   if imageN:size(1) > 1 then
      print('WARNING: computing flow on first feature')
      imageN=imageN:narrow(1,1,1)
   end

   local flow_x = flow_x or torch.Tensor()
   local flow_y = flow_y or torch.Tensor()

   if method == 'BM' then
      imageP.libopencv.CalcOpticalFlow(imageN, imageP, flow_x, flow_y, 1,
				block_w, block_h,
				shift_x, shift_y,
				window_w, window_h,
				reuse)
   elseif method == 'LK' then
      imageP.libopencv.CalcOpticalFlow(imageN, imageP, flow_x, flow_y, 2,
				block_w, block_h)
   elseif method == 'HS' then
      imageP.libopencv.CalcOpticalFlow(imageN, imageP, flow_x, flow_y, 3,
				       lagrangian, iterations,
					  -1,-1,-1,-1,
				       reuse)
   else
      print('Unkown method')
      error(args.usage)
   end
   if raw then
      if autoscale then
	 local flow_x_s = torch.Tensor():resizeAs(imageP)
	 local flow_y_s = torch.Tensor():resizeAs(imageP)
	 print('flow_x_s:size()',flow_x_s:size())
	 print('flow_y_s:size()',flow_y_s:size())
	 image.scale(flow_x, flow_x_s, 'simple')
	 image.scale(flow_y, flow_y_s, 'simple')
	 return flow_x_s, flow_y_s
      else
	 return flow_x, flow_y
      end
   else
      local flow_norm = torch.Tensor()
      local flow_angle = torch.Tensor()
      -- compute norm:
      local x_squared = torch.Tensor():resizeAs(flow_x):copy(flow_x):cmul(flow_x)
      flow_norm:resizeAs(flow_y):copy(flow_y):cmul(flow_y):add(x_squared):sqrt()
      -- compute angle:
      flow_angle:resizeAs(flow_y):copy(flow_y):cdiv(flow_x):abs():atan():mul(180/math.pi)
      flow_angle:map2(flow_x, flow_y,
		      function(h,x,y)
			 if x == 0 and y >= 0 then
			    return 90
			 elseif x == 0 and y <= 0 then
			    return 270
			 elseif x >= 0 and y >= 0 then
			    -- all good
			 elseif x >= 0 and y < 0 then
			    return 360 - h
			 elseif x < 0 and y >= 0 then
			    return 180 - h
			 elseif x < 0 and y < 0 then
			    return 180 + h
			 end
		      end)
      if autoscale then
	 local flow_norm_s = torch.Tensor():resizeAs(imageP)
	 local flow_angle_s = torch.Tensor():resizeAs(imageP)
	 local flow_x_s = torch.Tensor():resizeAs(imageP)
	 local flow_y_s = torch.Tensor():resizeAs(imageP)
	 image.scale(flow_angle, flow_angle_s, 'simple')
	 image.scale(flow_norm, flow_norm_s, 'simple')
	 image.scale(flow_x, flow_x_s, 'simple')
	 image.scale(flow_y, flow_y_s, 'simple')
	 return flow_norm_s, flow_angle_s, flow_x_s, flow_y_s
      else
	 return flow_norm, flow_angle, flow_x, flow_y
      end
   end
end

function opencv.display(...)
   local args = {}
   xlua.unpack_class(args, {...}, 'opencv.display',
      'displays a single image, with optional parameters',
      {arg='image', type='torch.Tensor | table',
       help='image or table of images 3xHxW', req=true},
      {arg='min', type='number', help='lower-bound for range', default=nil},
      {arg='max', type='number', help='upper-bound for range', default=nil},
      {arg='win', type='string', help='window legend (and descriptor)', default=nil}
   )
   
   function cloneImg(src, dst)
      if src:size(1) == 1 then
	 src = src[1]
      end
      if src:nDimension() == 2 then
	 dst[1]:copy(src)
	 dst[2]:copy(src)
	 dst[3]:copy(src)
      else
	 dst:copy(src)
      end
   end
   local image
   if type(args.image) == 'table' then
      local n = #args.image
      local h = args.image[1]:size(2)
      local w = args.image[1]:size(3)
      image = torch.Tensor(3, h, n*w)
      for i = 1,n do
	 cloneImg(args.image[i], image:narrow(3, (i-1)*w+1, w))
      end
   else
      local h, w
      if args.image:nDimension() == 2 then
	 h = args.image:size(1)
	 w = args.image:size(2)
      else
	 h = args.image:size(2)
	 w = args.image:size(3)
      end
      image = torch.Tensor(3, h, w)
      cloneImg(args.image, image)
   end
   
   if not args.min then args.min = image:min() end
   if not args.max then args.max = image:max() end
   image:add(-args.min)
   image:div(args.max-args.min)

   opencv.windownames = opencv.windownames or {}
   if not args.win then
      local i = 1
      while opencv.windownames[string.format("window %d", i)] do
	 i = i + 1
      end
      args.win = string.format("window %d", i)
      opencv.windownames[args.win] = true
   end
   
   image.libopencv.display(image, args.win)
   return args.win
end

-- testers:
function opencv.CalcOpticalFlow_testme(img1, img2)
   local img1 = img1
   local img2 = img2
   if not img1 then
      img1 = opencv.imgL()
      image.display{image=img1,legend='Original image (Left)'}
   end
   if not img2 then
      img2 = opencv.imgR()
      image.display{image=img2,legend='Original image (Right)'}
   end
   img1 = image.scale(img1,img1:size(3)/2,img1:size(2)/2)
   img2 = image.scale(img2,img2:size(3)/2,img2:size(2)/2)
   local methods = {'LK', 'HS', 'BM'}
   for i,method in ipairs(methods) do
      print(i,method)
      local norm, angle, flow_x, flow_y =
	 opencv.CalcOpticalFlow{pair={img1,img2}, method=method}
      local hsl = torch.Tensor(3,img1:size(2), img1:size(3))
      hsl:select(1,1):copy(angle):div(360)
      hsl:select(1,2):copy(norm):div(math.max(norm:max(),1e-2))
      hsl:select(1,3):fill(0.5)
      local rgb = image.hsl2rgb(hsl)
      image.display{image={img1,img2,rgb},
		    legend='cvOpticalFLow, method = ' .. method}
      image.display{image={norm,angle,flow_x,flow_y},
                    scaleeach=true,
		    legend='cvOpticalFLow, method = ' .. method}
   end
end

-- GoodFeaturesToTrack
opencv.GoodFeaturesToTrack
   = function(...)
	local args, image, count, quality, min_distance, win_size, mask  =
	   dok.unpack(
	   {...},
	   'opencv.GoodFeaturesToTrack',
	   [[
 Computes the GoodFeatures algorithm of opencv.

   + returns a points tensor (npoints x 2) of the pixel positions 
     (x,y in image space) of the features.

]],
	   {arg='image', type='torch.Tensor', 
            help='image in which to detect Good Feature points',req=true},
	   {arg='count',type='number', 
            help='number of points to return', default=500},
	   {arg='quality',type='number', 
            help='quality', default=0.01},
	   {arg='min_distance',type='number', 
            help='min spatial distance (in pixels) between returned feature points', default=10},
	   {arg='win_size',type='number', 
            help='window size over which to run heuristics', default=10},
           {arg='mask', type='torch.Tensor', 
            help="a tensor of 0 and 1s which specifies a region of the image on which to run the goodfeatures",default=nil}
	)
	local img = image
	local points = torch.Tensor(2,count)
	img.libopencv.GoodFeaturesToTrack(img,
                                          points,
                                          count,
                                          quality,
                                          min_distance,
                                          win_size,
                                          mask)
	return points
     end

-- testers:
function opencv.GoodFeaturesToTrack_testme(img)
   if not img then
      img = opencv.imgL()
   end
   sys.tic()
   local pts = opencv.GoodFeaturesToTrack{image=img,count=125}
   local s = sys.toc()
   print("Found "..pts:size(1).." points in "..s.." secs")
end


opencv.CalcOpticalFlowPyrLK
   = function(...)
	local args, image_from, image_to =
	   dok.unpack(
	   {...},
	   'opencv.CalcOpticalFlowPyrLK',
	   [[
Computes the Pyramidal Lucas-Kanade optical flow algorithm of opencv.
  + input two images
  + returns a points tensor of the pixel positions of the features
and a copy of the input image with red lines indicating the flow from
the interest points
           ]],
	   {arg='image_from', type='torch.Tensor',
	    help='image in which calculate from flow',req=true},
	   {arg='image_to', type='torch.Tensor',
	    help='image in which calculate to flow',req=true}
	)

   -- need to clean this up can be internal to C function
   local flowx = torch.Tensor(image_from:size(1),image_from:size(2)):zero()
   local flowy = torch.Tensor(image_from:size(1),image_from:size(2)):zero()
   local points = torch.Tensor(500,2)
   local image_out = torch.Tensor():resizeAs(image_to):copy(image_to)
   image_from.libopencv.CalcOpticalFlowPyrLK(image_from,image_to,flowx,flowy,points,image_out)
   return points, image_out
end

function opencv.CalcOpticalFlowPyrLK_testme(imgL,imgR)
   if not imgL then
      imgL = opencv.imgL()
   end
   if not imgR then
      imgR = opencv.imgR()
   end
   local points, image_out = opencv.CalcOpticalFlowPyrLK(imgL,imgR)
   image.display(image_out)
end

function opencv.LowLevelConversions_testme(img)
   if not img then
      img = opencv.imgL()
   end
   local imgn = img:narrow(1,1,1)
   local dst = torch.Tensor()

   print('Testing torch>IPL8U ... 1 Channel')
   img.libopencv.test_torch2IPL8U(imgn,dst)
   local err = (imgn-dst):max()
   if err > 1/255 then
      print ('  ERROR '..err)
   else
      print ('  OK')
   end
   print('Testing torch>IPL8U ... 3 Channels')
   img.libopencv.test_torch2IPL8U(img,dst)
   local err = (img-dst):max()
   if err > 1/255 then
      print ('  ERROR '..err)
   else
      print ('  OK')
   end
   print('Testing torch>IPL32F ... 1 Channel')
   dst = torch.Tensor()
   img.libopencv.test_torch2IPL32F(imgn,dst)
   err = (imgn-dst):max()
   if  err > 0 then
      print ('  ERROR '..err)
   else
      print ('  OK')
   end
   print('Testing torch>IPL32F ... 3 Channels')
   dst = torch.Tensor()
   img.libopencv.test_torch2IPL32F(img,dst)
   err = (img-dst):max()
   if  err > 0 then
      print ('  ERROR '..err)
   else
      print ('  OK')
   end
end


-- Pyramidal Lucas-Kanade
opencv.TrackPyrLK
   = function(...)
	local args, pair, points_in, points_out, win_size  = dok.unpack(
	   {...},
	   'opencv.TrackPyrLK',
	   [[
Runs pyramidal Lucas-Kanade, on two input images and a set of
points which are meant to be tracked.  

Returns 3 tensors:
 - points : the npoints x 2 list of matching points in the second image
 - feature_found : binary npoints whether the feature was found
 - feature_error : how close the found feature matches original
           ]],
	   {arg='pair', type='table',
	    help='a pair of images (2 WxHx1 tensor)', req=true},
	   {arg='points_in',type='torch.Tensor',
	    help='points to track', req=true},
	   {arg='points_out',type='torch.Tensor',
	    help='tensor to return location of tracked points in output'},
	   {arg='win_size',type='number',
	    help='over how large of a window can the LK track', default= 25}
	)
        if not points_out then
           points_out = torch.Tensor():resizeAs(points_in):zero()
        end
	local feature_found = torch.Tensor(points_in:size(1)):zero()
	local feature_error = torch.Tensor(points_in:size(1)):zero()
	pair[1].libopencv.TrackPyrLK(pair[1], pair[2],
                                     points_in, points_out, win_size,
                                     feature_found, feature_error)

	return points_out, feature_found, feature_error
     end

opencv.drawFlowlinesOnImage
   = function (...)
	local args, pair, image, color, mask = dok.unpack(
	   {...},
	   'opencv.drawFlowlinesOnImage',
	   [[ utility to visualize sparse flows ]],
	   {arg='pair', type='table',
	    help='a pair of point tensors (2 nPointsx(2 or 3) tensor)', req=true},
	   {arg='image', type='torch.Tensor',
	    help='image on which to draw the flowlines', req=true},
	   {arg='color', type='torch.Tensor',
	    help='color of flow line eg. R = [255,0,0]'},
	   {arg='mask', type='torch.Tensor',
	    help='mask tensor 1D npoints 0 when not to draw point'}
	)
        -- default color is red
	if not color then
	   color = torch.Tensor(3):zero()
	   color[1] = 255
	end
	pair[1].libopencv.drawFlowlinesOnImage(pair[1],pair[2],image,color,mask)
     end

opencv.circlePoints
   = function (...)
	local args, points, image, color, size = dok.unpack(
	   {...},
	   'opencv.circlePoints',
	   [[ draw circles around interest points ]],
	   {arg='points', type='torch.Tensor',
	    help='a point tensor (nPointsx(2or3) tensor)', req=true},
	   {arg='image', type='torch.Tensor',
	    help='image on which to draw the circles', req=true},
	   {arg='color', type='torch.Tensor',
	    help='color of flow line eg. R = [255,0,0]'},
	   {arg='size', type='number',
	    help='size of the circles to draw'}
	)
        -- default color is red
	if not color then
	   color = torch.Tensor(3):zero()
	   color[1] = 255
	end
        if not size then
           size = 10
        end
	points.libopencv.circlePoints(points,image,color,size)
     end

opencv.drawPoly
   = function (...)
	local args, points, image, fill, color = dok.unpack(
	   {...},
	   'opencv.drawPoly',
	   [[ draw polygon on an image ]],
	   {arg='points', type='torch.Tensor',
	    help='a point tensor (nPointsx(2) tensor)', req=true},
	   {arg='image', type='torch.Tensor',
	    help='image on which to draw the polygon', req=true},
           {arg="fill", type="boolean",
            help="fill polygon or draw outline"},
	   {arg='color', type='torch.Tensor',
	    help='color of fill or outline eg. RGB = [255,0,0] or RGBA = [255,255,255,50]'}
	)
        -- default color is red
	if not color then
	   color = torch.Tensor(3):zero()
	   color[1] = 255
	end
        local fillint
        if not fill then
           fillint = 0
        else
           fillint = 1
        end
	points.libopencv.drawPoly(points,image,fillint,color)
     end

function opencv.TrackPyrLK_testme(imgL,imgR)
   if not imgL then
      imgL = opencv.imgL()
   end
   if not imgR then
      imgR = opencv.imgR()
   end
   local ptsin = opencv.GoodFeaturesToTrack{image=imgL,count=imgL:nElement()}

   local ptsout = opencv.TrackPyrLK{pair={imgL,imgR},points_in=ptsin}
   opencv.drawFlowlinesOnImage({ptsin,ptsout},imgR)
   image.display{image={imgL,imgR},
		 legend='opencv: Optical Flow Pyramidal LK Tracking',
		 win_w=imgL:size(1)*2,win_h=imgL:size(2)}
end

function opencv.TrackPyrLK_wmask_testme(imgL,imgR,mask)
   if not imgL then
      imgL = opencv.imgL()
   end
   if not imgR then
      imgR = opencv.imgR()
   end
   if not mask then
      mask = torch.Tensor(imgL:size(2),imgL:size(3)):fill(0)
      mask:narrow(1,200,200):narrow(2,100,250):fill(1)
   end
   local ptsin = opencv.GoodFeaturesToTrack{image=imgL,
                                            count=imgL:nElement(),
                                            mask=mask}

   local ptsout = opencv.TrackPyrLK{pair={imgL,imgR},points_in=ptsin}
   opencv.drawFlowlinesOnImage({ptsin,ptsout},imgR)
   image.display{image={imgL,imgR},
		 legend='opencv: Optical Flow Pyramidal LK Tracking',
		 win_w=imgL:size(1)*2,win_h=imgL:size(2)}
end

-- opencv.smoothVoronoi
--    = function (...)
-- 	local args, points, data, output = dok.unpack(
-- 	   {...},
-- 	   'opencv.smoothVoronoi',
-- 	   [[ dense interpolation of sparse flows ]],
-- 	   {arg='points', type='torch.Tensor',
-- 	    help='nPoints x 2 tensor -- locations', req=true},
-- 	   {arg='data', type='torch.Tensor',
-- 	    help='nPoints x n tensor -- data', req=true},
-- 	   {arg='output', type='torch.Tensor',
-- 	    help='bounding rectangle in which dense flows will be stored'}
-- 	)
-- 	if not output then
-- 	   local width  = points:select(2,1):max()
-- 	   local height = points:select(2,2):max()
--            output = torch.Tensor(data:size(2),height,width)
-- 	end
--         sys.tic()
--         data.libopencv.smoothVoronoi(points,data,output)
--         print("time to compute dense flow: ",sys.toc())
-- 	return output
--      end

-- function opencv.smoothVoronoi_testme(imgL,imgR)
--    if not imgL then
--       imgL = opencv.imgL()
--    end
--    if not imgR then
--       imgR = opencv.imgR()
--    end
--    ptsin = opencv.GoodFeaturesToTrack{image=imgL,count=imgL:nElement()}
--    ptsout = opencv.TrackPyrLK{pair={imgL,imgR},points_in=ptsin}
--    output = opencv.smoothVoronoi(ptsout,ptsout-ptsin)
--    image.display{image={output:select(1,1),output:select(1,2)}}
-- end

opencv.findFundamental
   = function (...)
	local args, points1, points2 = dok.unpack(
	   {...},
	   'opencv.FindFundamental',
	   [[ find fundamental matrix in 2 sets of points]],
	   {arg='points1', type='torch.Tensor',
	    help='nPoints x 2 tensor -- locations', req=true},
	   {arg='points2', type='torch.Tensor',
	    help='nPoints x 2 tensor -- locations', req=true}
	)
	if not output then
           output = torch.Tensor(3,3)
	end
	if not status then
           status = torch.Tensor(points1:size(1))
	end
        points1.libopencv.FindFundamental(points1,points2,output,status)
	return output,status
     end

function opencv.findFundamental_testme(imgL,imgR)
   if not imgL then
      imgL = opencv.imgL()
   end
   if not imgR then
      imgR = opencv.imgR()
   end
   sys.tic()
   local ptsin  = opencv.GoodFeaturesToTrack{image=imgL,count=imgL:nElement()}
   local ptsout = opencv.TrackPyrLK{pair={imgL,imgR},points_in=ptsin}
   print("time to compute good features: ",sys.toc())
   sys.tic()
   local matrix,status = opencv.findFundamental{points1=ptsin,points2=ptsout}
   print("time to compute fundamental matrix: ",sys.toc())
   print(matrix)
end

opencv.findEssential
   = function (...)
	local args, fundamental, calibration = dok.unpack(
	   {...},
	   'opencv.FindEssential',
	   [[ find essential matrix from fundamental and calibration]],
	   {arg='fundamental', type='torch.Tensor',
	    help='3x3 fundamental matrix', req=true},
	   {arg='calibration', type='torch.Tensor',
	    help='3x3 tensor -- camera calibration', req=true}
	)
        return calibration:transpose(1,2) * fundamental * calibration
     end

function opencv.findEssential_testme(imgL,imgR)
   if not imgL then
      imgL = opencv.imgL()
   end
   if not imgR then
      imgR = opencv.imgR()
   end
   sys.tic()
   local ptsin  = opencv.GoodFeaturesToTrack{image=imgL,count=imgL:nElement()}
   local ptsout = opencv.TrackPyrLK{pair={imgL,imgR},points_in=ptsin}
   print("time to compute good features: ",sys.toc())
   sys.tic()
   local fundmat,status = opencv.findFundamental{points1=ptsin,points2=ptsout}
   print("time to compute fundamental matrix: ",sys.toc())
   local k = torch.Tensor(3,3):fill(0)
   k[1][1] = 602 -- focal length in pixels
   k[2][2] = 602 -- focal length in pixels
   k[1][3] = 1280/2 -- center width
   k[2][3] = 720/2 -- center height
   k[3][3] = 1
   sys.tic()
   local essenmat = opencv.findEssential(fundmat, k)
   print("time to compute essential matrix: ",sys.toc())
   print(essenmat)
end

opencv.getExtrinsicsFromEssential
   = function (...)
	local args, essential, point = dok.unpack(
	   {...},
	   'opencv.getExtrinsicsFromEssential',
	   [[ get camera extrinsic matrix from essential matrix and a single point]],
	   {arg='essential', type='torch.Tensor',
	    help='3x3 essential matrix', req=true},
	   {arg='point', type='torch.Tensor',
	    help='3x1 tensor -- 3D point', req=true}
	)
        -- u*torch.diag(s)*v:t()
        local u,s,v = torch.svd(essential)
        -- if ((math.abs(s[1] - s[2]) > 0.1) or math.abs(s[3]) > 0.01) then
        --    dok.error("bad essential matrix")
        -- end
        print("U")
        print(u)
        print("S - diagonal")
        print(s)
        print("V -- which you transpose")
        print(v)
        local w = torch.Tensor(3,3)
        w[1][2] = -1
        w[2][1] =  1
        w[3][3] =  1
        local z = torch.Tensor(3,3)
        z[1][2] =  1
        z[2][1] = -1
        extr = torch.Tensor(3,4)
        extr:narrow(2,1,3):copy(u*w*v:t())
        extr:narrow(2,3,1):copy(u:narrow(1,3,1))
        print(extr)
     end

function opencv.getExtrinsicsFromEssential_testme(imgL,imgR)
   if not imgL then
      imgL = opencv.imgL()
   end
   if not imgR then
      imgR = opencv.imgR()
   end
   sys.tic()
   local ptsin  = opencv.GoodFeaturesToTrack{image=imgL,count=imgL:nElement()}
   local ptsout = opencv.TrackPyrLK{pair={imgL,imgR},points_in=ptsin}
   print("time to compute good features: ",sys.toc())
   sys.tic()
   local fundmat,status = opencv.findFundamental{points1=ptsin,points2=ptsout}
   print("time to compute fundamental matrix: ",sys.toc())
   local k = torch.Tensor(3,3):fill(0)
   k[1][1] = 602 -- focal length in pixels
   k[2][2] = 602 -- focal length in pixels
   k[1][3] = 1280/2 -- center width
   k[2][3] = 720/2 -- center height
   k[3][3] = 1
   sys.tic()
   local essenmat = opencv.findEssential(fundmat, k)
   print("time to compute essential matrix: ",sys.toc())
   sys.tic()
   local extrmat = opencv.getExtrinsicsFromEssential(essenmat,ptsout[1])
   print("time to get extrinsics from essential: ",sys.toc())

end

opencv.videoForward =
   function (...)
      local args, videoid, tensor  = dok.unpack(
         {...},
         'opencv.videoForward',
         [[ Grabs next frame from an open video stream (returns 0 at EOF)]],
         {arg='videoid', type='int',
          help='id of video file (can have multiple open video files)',
          req=true},
         {arg='tensor', type='torch.Tensor',
          help='tensor in which you want to store the frame'}
      )
      if not tensor then
         tensor=torch.Tensor()
      end
      local ret = tensor.libopencv.videoGetFrame(videoid,tensor)
      return tensor, ret
   end

opencv.videoWriteFrame = 
   function (...)
      local args, videoid, tensor  = dok.unpack(
         {...},
         'opencv.videoWriteFrame',
         [[ writes frame to an open video writer (returns 1 at EOF)]],
         {arg='videoid', type='int', 
          help='id of video writer (can have multiple open video writers)',
          req=true},
         {arg='tensor', type='torch.Tensor',
          help='tensor which you want to write',
          req=true}
      )
      
      return tensor.libopencv.videoWriteFrame(videoid,tensor)
   end


opencv.camera_testme = 
   function (...)
	local args, idx, duration = dok.unpack(
	   {...},
	   'opencv.camera_testme',
	   [[ open camera device and play]],
	   {arg='idx', type='int',
	    help='device id', default=-1},
	   {arg='duration', type='int',
	    help='number of frames to capture', default=100}
	)
        local vid = opencv.videoOpenCamera(idx)
        print("Opened " .. idx)
        print(" Video id     : " .. vid )
        opencv.videoDumpProperties(vid)
        local img = torch.Tensor()
        print(" Playing for  : " .. duration .. " frames")
        opencv.videoForward(vid,img)
        local win = image.display{image=img, win=win}
        for i = 1,duration do
           sys.tic()
           opencv.videoForward(vid,img)
           win = image.display{image=img, win=win}
           print("FPS: "..1/sys.toc())
        end
        print(" Closing id   : " .. vid)
        opencv.videoCloseFile(vid)
     end

opencv.video_testme =
   function (...)
	local args, fname, seekto, duration = dok.unpack(
	   {...},
	   'opencv.video_testme',
	   [[ open video file, seek and play]],
	   {arg='fname', type='string',
	    help='filename of video file', req=true},
	   {arg='seekto', type='float',
	    help='position in sec (float)', default=10},
	   {arg='duration', type='float',
	    help='number of seconds to play', default=10}
	)
        local vid = opencv.videoLoadFile(fname)
        print("Opened " .. fname)
        print(" Video id     : " .. vid )
        local fps = opencv.videoGetFPS(vid)
        print(" FPS          : " .. fps)
        local msec = opencv.videoSeek(vid,seekto)
        print(" Seek request : " .. seekto .. "s")
        print(" Seeked to    : " .. msec*0.001 .."s")
        print(" Playing for  : " .. duration .. "s")
        local img,ret = opencv.videoForward(vid)           
        local win = image.display{image=img, win=win}
        for i = 1,duration*fps do
           sys.tic()
           img, ret = opencv.videoForward(vid,img)
           if (ret<1) then
              break;
           else
              win = image.display{image=img, win=win}
              if (i == 1) then
                 print(" 1st frame at : " ..
                       opencv.videoGetMSEC(vid)*0.001 .."s")
              end
           end
           print("Frame[",i,"] FPS: ", 1/sys.toc())
        end
        print(" Closing id   : " .. vid)
        opencv.videoCloseFile(vid)
     end

opencv.videowriter_testme = 
   function (...)
      local args, finput, foutput, fourcc, seekto, duration = dok.unpack(
         {...},
         'opencv.videowriter_testme',
         [[ open video file, seek and write frames to another video ]],
         {arg='finput', type='string',
          help='filename of video input file', req=true},
         {arg='foutput', type='string',
          help='filename of video output file', default="out.avi"},
         {arg='fourcc', type='string',
          help='fourcc for video output file', default="mjpg"},
         {arg='seekto', type='float',
	    help='position in sec (float)', default=10},
	   {arg='duration', type='float',
	    help='number of seconds to play', default=1}
	)
        local vid = opencv.videoLoadFile(finput)
        print("Opened " .. finput)
        print(" Video id     : " .. vid )
        local img = torch.Tensor()
        local fps = opencv.videoGetFPS(vid)
        print(" FPS          : " .. fps)
        local msec = opencv.videoSeek(vid,seekto)
        print(" Seek request : " .. seekto .. "s")
        print(" Seeked to    : " .. msec*0.001 .."s")
        opencv.videoForward(vid,img)
        local width = img:size(3)
        local height = img:size(2)
        print(" Frame in size : "..width.."x"..height)
        local imgs = image.scale(img,width,height)
        local wrt = 
           opencv.videoWriter(foutput,width,height,fps,fourcc)
        print("Opened " .. foutput)
        print(" Writer id     : " .. wrt )
        opencv.videoWriteFrame(wrt,imgs)
        local win = image.display{image=img, win=win}
        print(" Playing for  : " .. duration .. "s")
        for i = 1,duration*fps do
           sys.tic()
           opencv.videoForward(vid,img)
           win = image.display{image=img, win=win}
           opencv.videoWriteFrame(wrt,img)
           if (i == 1) then
              print(" 1st frame at : " .. 
                    opencv.videoGetMSEC(vid)*0.001 .."s") 
           end
           print("FPS: "..1/sys.toc())
        end
        print(" Closing writer   : " .. wrt)
        opencv.videoCloseWriter(wrt)
        print(" Closing id       : " .. vid)
        opencv.videoCloseFile(vid)
     end

opencv.video_multi_testme = 
   function (...)
	local args, fname1, fname2, seekto1, seekto2, duration = dok.unpack(
	   {...},
	   'opencv.video_testme',
	   [[ open video file, seek and play]],
	   {arg='fname1', type='string',
	    help='filename of video file', req=true},
	   {arg='fname2', type='string',
	    help='filename of video file', req=true},
	   {arg='seekto1', type='float',
	    help='position in sec (float)', default=10},
	   {arg='seekto2', type='float',
	    help='position in sec (float)', default=10},
	   {arg='duration', type='float',
	    help='number of seconds to play', default=10}
	)
        -- video 1
        local vid1 = opencv.videoLoadFile(fname1)
        print("Opened " .. fname1)
        print(" Video id     : " .. vid1 )
        local img1 = torch.Tensor()
        local fps1 = opencv.videoGetFPS(vid1)
        print(" FPS1         : " .. fps1)
        local msec1 = opencv.videoSeek(vid1,seekto1)
        print(" Seek request : " .. seekto1 .. "s")
        print(" Seeked to    : " .. msec1*0.001 .."s")
        -- video 2
        local vid2 = opencv.videoLoadFile(fname2)
        print("Opened " .. fname2)
        print(" Video id     : " .. vid2 )
        local img2 = torch.Tensor()
        local fps2 = opencv.videoGetFPS(vid2)
        print(" FPS2         : " .. fps2)
        local msec2 = opencv.videoSeek(vid2,seekto2)
        print(" Seek request : " .. seekto2 .. "s")
        print(" Seeked to    : " .. msec2*0.001 .."s")
        print(" Playing for  : " .. duration .. "s")
        opencv.videoForward(vid1,img1)
        opencv.videoForward(vid2,img2)
        local win = image.display{image={img1, img2}, win=win}
        for i = 1,duration*fps1 do
           opencv.videoForward(vid1,img1)
           opencv.videoForward(vid2,img2)
           win = image.display{image={img1, img2}, win=win}
           if (i == 1) then
              print(" 1st frame at : " ..
                    opencv.videoGetMSEC(vid1)*0.001 .."s")
              print(" 1st frame at : " ..
                    opencv.videoGetMSEC(vid2)*0.001 .."s")
           end
        end
        print(" Closing id   : " .. vid1)
        opencv.videoCloseFile(vid1)
        print(" Closing id   : " .. vid2)
        opencv.videoCloseFile(vid2)
     end

function opencv.testme()
   local imgL = opencv.imgL()
   local imgR = opencv.imgR()
   opencv.LowLevelConversions_testme(imgL)
   opencv.CornerHarris_testme(imgL)
   opencv.CalcOpticalFlow_testme(imgL,imgR)
   opencv.GoodFeaturesToTrack(imgL)
   opencv.TrackPyrLK_testme(imgL,imgR)
end
