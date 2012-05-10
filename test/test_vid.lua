vnames = {
   "/home/data/gopro/carapp/IMG_0020.MOV.mp4.webm",
   "/home/data/gopro/carapp/IMG_0021.MOV.mp4.webm",
   "/home/data/gopro/carapp/IMG_0022.MOV.mp4.webm",
   "/home/data/gopro/carapp/IMG_0023.MOV.mp4.webm",
   "/home/data/gopro/carapp/IMG_0025.MOV.mp4.webm",
   "/home/data/gopro/carapp/IMG_0026.MOV.mp4.webm",
   "/home/data/gopro/carapp/IMG_0027.MOV.mp4.webm"
}

function openvid (fname,seek)
   -- video 1
   local vid = opencv.videoLoadFile(fname)
   print("Opened " .. fname)
   print(" Video id     : " .. vid)
   local fps = opencv.videoGetFPS(vid)
   -- print(" FPS          : " .. fps)
   local msec = opencv.videoSeek(seek,vid)
   -- print(" Seek request : " .. seek .. "s")
   -- print(" Seeked to    : " .. msec*0.001 .."s")
   return vid
end

function closevid(vid)
   print(" Closing id   : " .. vid)
   opencv.videoCloseFile(vid)
end
v = {}

for i = 1,7 do
   opencv.videoPrintNextList()
   v[i] = openvid(vnames[i],i*11)
   print("lua vid: " .. v[i])
end

closevid(v[2])
opencv.videoPrintNextList()
closevid(v[5])
opencv.videoPrintNextList()
closevid(v[1])
opencv.videoPrintNextList()
closevid(v[7])
opencv.videoPrintNextList()

v2 = {}
for i = 1,7 do
   opencv.videoPrintNextList()
   v2[i] = openvid(vnames[i],i*11)
   print("lua vid: " .. v[i])
end

-- can test max nb files open by uncommenting
-- openvid(vnames[1],111)

opencv.videoPrintNextList()
for i = 0,10 do
   closevid(i)
   opencv.videoPrintNextList()
end