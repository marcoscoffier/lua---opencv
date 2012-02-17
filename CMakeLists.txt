CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
CMAKE_POLICY(VERSION 2.6)
FIND_PACKAGE(Torch REQUIRED)
find_package (OpenCV REQUIRED)

if (OpenCV_FOUND)
  MESSAGE(STATUS "Found OpenCV")
  MESSAGE(STATUS "OpenCV libs: ${OpenCV_LIBS}")
  MESSAGE(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
  INCLUDE_DIRECTORIES (${OpenCV_INCLUDE_DIRS})
  set(src opencv.c)
  set(luasrc init.lua img1.jpg img2.jpg)
  ADD_TORCH_PACKAGE(opencv "${src}" "${luasrc}" "OpenCV wrapper")
  TARGET_LINK_LIBRARIES(opencv luaT TH)

endif(OpenCV_FOUND)
   