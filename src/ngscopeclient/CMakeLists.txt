cmake_minimum_required(VERSION 3.24)

project(ngscopeclient CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # error if compiler doesn't support c++14
set(CMAKE_CXX_EXTENSIONS OFF) # use c++14 instead of gnu++14

find_package(sigc++-3 CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS glslang)
find_path(VKFFT_INCLUDE_DIRS "VkFFT/vkFFT.h")
find_package(glslang CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

#Set up include paths
include_directories(
	SYSTEM
	#${SIGCXX_INCLUDE_DIRS}
#	${GTKMM_INCLUDE_DIRS}
	${VULKAN_SDK}/include
	${VKFFT_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}/../imgui/
	${CMAKE_CURRENT_SOURCE_DIR}/../imgui/misc/cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/include/glslang/Include
	${Vulkan_glslang_LIBRARY}/Include
	)
#link_directories(${SIGCXX_LIBRARY_DIRS})
add_subdirectory("${PROJECT_SOURCE_DIR}/log")

###############################################################################
#C++ compilation
add_executable(ngscopeclient
	../imgui/imgui.cpp
	../imgui/imgui_demo.cpp
	../imgui/imgui_draw.cpp
	../imgui/imgui_tables.cpp
	../imgui/imgui_widgets.cpp
	../imgui/backends/imgui_impl_glfw.cpp
	../imgui/backends/imgui_impl_vulkan.cpp
	../imgui/misc/cpp/imgui_stdlib.cpp

	Dialog.cpp
	PipelineCacheManager.cpp
	log/log.cpp
	FileSystem.cpp
	scopehal.cpp

	MainWindow.cpp
	VulkanWindow.cpp
	pthread_compat.cpp

	AddScopeDialog.cpp
	MainWindow.cpp
	Session.cpp
	ScopeThread.cpp

	main.cpp
)

###############################################################################
#Linker settings
target_link_libraries(ngscopeclient
	Vulkan::Vulkan
#	scopehal
#	scopeprotocols
#	scopeexports
	glfw
	HLSL SPIRV glslang OGLCompiler
	yaml-cpp
#	${GTKMM_LIBRARIES}
	sigc-3.0
	#${SIGCXX_LIBRARIES}
	)

