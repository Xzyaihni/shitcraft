cmake_minimum_required(VERSION 3.13)
project(shitcraft)
set(CMAKE_CXX_STANDARD 20)

option(YLOCAL "don't use this option" "OFF")
option(YDEBUG "build in debug mode" "OFF")
option(YCROSSCOMPILEW "cross compile on linux for windows" "OFF")

if(${YCROSSCOMPILEW})
	set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
endif()

if(${YLOCAL})
set(CMAKE_PREFIX_PATH /home/yanyana/cppPackages/glm/cmake)
endif()
set(YANDERELIBS glcyan.cpp
yanconv.cpp)


set(SOURCE_FILES main.cpp
character.cpp
world.cpp
noise.cpp
inventory.cpp
physics.cpp
types.cpp
${YANDERELIBS})

if(${YDEBUG})
set(CMAKE_BUILD_TYPE Debug)
else()
set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${PROJECT_SOURCE_DIR})

find_package(Threads REQUIRED)
find_package(TBB REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(X11 REQUIRED)
find_package(Freetype REQUIRED)

add_custom_target(folder_files
${CMAKE_COMMAND}
-D SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
-D DESTINATION_DIR=${CMAKE_CURRENT_BINARY_DIR}
-P ../CopyFolders.cmake
COMMENT "copying asset files" VERBATIM
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/${SOURCE_FILES})

add_dependencies(${PROJECT_NAME} folder_files)

target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR}
${GLUT_INCLUDE_DIR}
${FREETYPE_INCLUDE_DIRS})

if(${YCROSSCOMPILEW})
	target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
endif()

if(${YDEBUG})
target_link_libraries(${PROJECT_NAME} -pg -fsanitize=address)
else()
target_link_libraries(${PROJECT_NAME} -O3)
endif()
target_link_libraries(${PROJECT_NAME} ${THREADS_LIBRARIES} -lpthread)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
target_link_libraries(${PROJECT_NAME} tbb)
target_link_libraries(${PROJECT_NAME} glm::glm)
target_link_libraries(${PROJECT_NAME} ${FREETYPE_LIBRARIES})
