project(TerragenFileManager)

SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DGL_DEBUG -std=c++11")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -std=c++11")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -DGL_DEBUG -std=c++11")
SET(CMAKE_BUILD_TYPE Debug)
#SET(CMAKE_BUILD_TYPE Release)

SET(SOURCE_FILES terragen_file_manager terragen_file)

#add_executable(TerFileManager ${SOURCE_FILES})
add_library(TerragenFileManager ${SOURCE_FILES})
