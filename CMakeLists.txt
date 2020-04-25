cmake_minimum_required( VERSION 3.15 )

project(
	ComicCreator
	VERSION 1.0.0
	LANGUAGES CXX
)

# Requier C++14 and disable extensions for all targets
set( CMAKE_CXX_STANDARD 14 )
set( CMAKE_CXX_STANDARD_REQURED ON )
set( CMAKE_CXX_EXTENSIONS OFF )
add_definitions( -DCOMIC_CREATOR_VERSION=\"v${CMAKE_PROJECT_VERSION}\" )

add_subdirectory( src )

