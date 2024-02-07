include_directories(SYSTEM ${CAIROMM_INCLUDE_DIRS} ${SIGCXX_INCLUDE_DIRS})
link_directories(${CAIROMM_LIBRARY_DIRS} ${SIGCXX_LIBRARY_DIRS})

find_library(LXI_LIB lxi)
find_library(TIRPC_LIB tirpc)
find_library(LINUXGPIB_LIB gpib)
find_package(glfw3 REQUIRED)
find_package(PNG REQUIRED)

#Set up versioning (with a dummy string for now if Git isn't present)
if(Git_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} describe --always --tags
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		OUTPUT_VARIABLE thunderscope-driver_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
	set(thunderscope-driver_VERSION "unknown")
endif()
include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(thunderscope-driver-version.h.in thunderscope-driver-version.h)

# Additional Windows/Linux libraries
if(WIN32)
	set(WIN_LIBS shlwapi)
	# The packaging for yaml-cpp on mingw64 seems to be broken, not providing a
	# libyaml-cpp.dll.a to be found by find_library inside of find_package(YAML)
	# when using FindYAML.cmake, just set the library directly
	set(YAML_LIBRARIES yaml-cpp)
else()
	set(LIN_LIBS dl)
	find_package(YAML REQUIRED)
endif()

if(LINUX AND CMAKE_SYSTEM_VERSION MATCHES "\.fc[0-9]+\.")
	set(FEDORA TRUE)
endif()

if(WIN32 OR FEDORA)
	set(SPIRV_LIBRARIES SPIRV SPIRV-Tools-opt SPIRV-Tools)
else()
	set(SPIRV_LIBRARIES "")
endif()

# Apple has their own idea about stat structs
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dst_mtim=st_mtimespec")
endif()

set(THUNDERSCOPEDRIVER_SOURCES
	PluginInit.cpp
	SCPILitePCIeTransport.cpp

	LiteXOscilloscope.cpp	
    )

#configure_file(config.h.in config.h)

add_library(thunderscope-driver SHARED
	${THUNDERSCOPEDRIVER_SOURCES})

if(WIN32)
# mingw64 build using
# https://sdk.lunarg.com/sdk/download/1.3.224.1/windows/VulkanSDK-1.3.224.1-Installer.exe
# https://github.com/KhronosGroup/glslang.git (tags/sdk-1.3.224.1) static lib (SPIRV* glslang OGLCompiler GenericCodeGen MachineIndependent OSDependent)
target_link_libraries(thunderscope-driver
	scopehal
	scopeprotocols
	nfd
	glfw
	PNG::PNG
	cairomm-1.0
	cairo
	${SIGCXX_LIBRARIES}
	${CAIROMM_LIBRARIES}
	xptools
	log
	${YAML_LIBRARIES}
	${LXI_LIBRARIES}
	${LINUXGPIB_LIBRARIES}
	${WIN_LIBS}
	${LIN_LIBS}
	${LIBFFTS_LIBRARIES}
	${OpenMP_CXX_LIBRARIES}
	${Vulkan_LIBRARIES}
	${SPIRV_LIBRARIES}
	glslang
	Vulkan::Vulkan
	-Wl,--start-group
	OGLCompiler
	GenericCodeGen
	MachineIndependent
	-Wl,--end-group
	OSDependent
	glfw
	)
elseif(APPLE_SILICON)
target_link_libraries(thunderscope-driver
	${SIGCXX_LIBRARIES}
	${CAIROMM_LIBRARIES}
	xptools
	log
	${YAML_LIBRARIES}
	${LXI_LIBRARIES}
	${LINUXGPIB_LIBRARIES}
	${WIN_LIBS}
	${LIN_LIBS}
	${OpenMP_CXX_LIBRARIES}
	${Vulkan_LIBRARIES}
	${SPIRV_LIBRARIES}
	Vulkan::Vulkan
	Vulkan::glslang
	Vulkan::shaderc_combined
	glfw
	)
else()
target_link_libraries(thunderscope-driver
	${SIGCXX_LIBRARIES}
	${CAIROMM_LIBRARIES}
	xptools
	log
	${YAML_LIBRARIES}
	${LXI_LIBRARIES}
	${LINUXGPIB_LIBRARIES}
	${WIN_LIBS}
	${LIN_LIBS}
	${LIBFFTS_LIBRARIES}
	${OpenMP_CXX_LIBRARIES}
	${Vulkan_LIBRARIES}
	${SPIRV_LIBRARIES}
	Vulkan::Vulkan
	Vulkan::glslang
	Vulkan::shaderc_combined
	glfw
	)
endif()

if(APPLE_SILICON)
	target_include_directories(thunderscope-driver
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		${YAML_INCLUDES}

		# TODO: this needs to come from FindPackage etc
		/usr/include/glslang/Include
		# for macOS:
		/usr/local/include/glslang/Include
		)
else()
	target_include_directories(thunderscope-driver
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		${LIBFFTS_INCLUDE_DIR}
		${YAML_INCLUDES}

		# TODO: this needs to come from FindPackage etc
		/usr/include/glslang/Include
		# for macOS:
		/usr/local/include/glslang/Include
		)
endif()

target_include_directories(thunderscope-driver
PUBLIC SYSTEM
	${CMAKE_CURRENT_SOURCE_DIR}/../../lib/VkFFT/vkFFT
	)

target_include_directories(thunderscope-driver
PUBLIC SYSTEM
	${OpenMP_CXX_INCLUDE_DIR}
	)

# Don't include yaml-cpp in Windows PCH because this seems to give errors about path resolution
# TODO: fix this for faster builds
if(${WIN32})
	set(YAML_PCH_PATH "")
else()
	set(YAML_PCH_PATH "${YAML_INCLUDES}/yaml-cpp/yaml.h")
endif()

target_precompile_headers(thunderscope-driver
PUBLIC

	# libstdc++ headers used by vulkan and/or very widely throughout the project
	#${CMAKE_CURRENT_SOURCE_DIR}/thunderscope-driver-pch.h

	${Vulkan_INCLUDE_DIR}/vulkan/vulkan_raii.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../lib/VkFFT/vkFFT/vkFFT.h
	${YAML_PCH_PATH}
	)


install(TARGETS thunderscope-driver LIBRARY)

