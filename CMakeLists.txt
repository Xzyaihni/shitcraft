cmake_minimum_required(VERSION 3.13)
project(shitcraft)
set(CMAKE_CXX_STANDARD 20)

option(Y_DEBUG "build in debug mode" "OFF")
option(Y_SANITIZE "build with address sanitizer" "OFF")

set(YANDERELIBS "yanderegllib/glcyan.cpp"
"yanderegllib/glcore.cpp"
"yanderegllib/ycamera.cpp"
"yanderegllib/yanconv.cpp"
"yanderegllib/ygui.cpp")


set(SOURCE_FILES main.cpp
character.cpp
chunk.cpp
cmap.cpp
cmodel.cpp
wgen.cpp
wctl.cpp
wblock.cpp
noise.cpp
inventory.cpp
physics.cpp
types.cpp
textures.cpp
${YANDERELIBS})

if(${Y_DEBUG})
	set(CMAKE_BUILD_TYPE Debug)
else()
	set(CMAKE_BUILD_TYPE Release)
endif()

find_package(TBB REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)

add_custom_target(folder_files
${CMAKE_COMMAND}
-D SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
-D DESTINATION_DIR=${CMAKE_CURRENT_BINARY_DIR}
-P ${PROJECT_SOURCE_DIR}/CopyFolders.cmake
COMMENT "copying asset files" VERBATIM
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/${SOURCE_FILES})

add_dependencies(${PROJECT_NAME} folder_files)

target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/yanderegllib"
${GLFW_INCLUDE_DIRS}
${OPENGL_INCLUDE_DIR}
${GLUT_INCLUDE_DIR}
${GLM_INCLUDE_DIRS}
${FREETYPE_INCLUDE_DIRS})



if(${Y_DEBUG})
	add_definitions(-DDEBUG)
	target_link_libraries(${PROJECT_NAME} -O1 -pg -Wall -Werror -pedantic-errors)
	
	if(${Y_SANITIZE})
		target_link_libraries(${PROJECT_NAME} -fsanitize=address)
	endif()
else()
	target_link_libraries(${PROJECT_NAME} -O3)
endif()

target_link_libraries(${PROJECT_NAME} tbb)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES})
target_link_libraries(${PROJECT_NAME} GLEW::glew)
target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
target_link_libraries(${PROJECT_NAME} Freetype::Freetype)
