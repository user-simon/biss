cmake_minimum_required (VERSION 3.20)

# Init project

file (GLOB_RECURSE biss_src CONFIGURE_DEPENDS "*.h" "*.cpp")
add_executable (biss ${biss_src})
target_include_directories (biss PRIVATE ".")
if (MSVC)
	source_group (TREE ".." FILES ${biss_src})
endif()