cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
project(YOLOKeypoint LANGUAGES CXX)

set(CMAKE_UNITY_BUILD ON)

if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

# Debug and Release build targets only (disable others like RelWithDebInfo)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "YOLOKeypoint")

find_package(OpenCV REQUIRED)

find_package(Qt5 REQUIRED
	Core
)

add_executable(YOLOKeypoint
	"main.cpp"
	SHColor.h
	SHRectangle.h
)

target_include_directories(YOLOKeypoint
	PRIVATE ${Qt5Widgets_INCLUDE_DIRS}
	PRIVATE ${OpenCV_INCLUDE_DIRS}
	PRIVATE ${Qt5Core_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(YOLOKeypoint
	Qt5::Core
	${OpenCV_LIBS}
)

