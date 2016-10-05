# CMAKE SCRIPT FOR BUILDING COMETOS CORE

# check for required libs and include headers
if(NOT DEFINED "platform_INCLUDE_DIRS")
	message (FATAL_ERROR "Missing library. Add CometOS 'platform' before 'core' library.")
endif()

include_directories("platform_INCLUDE_DIRS" "$ENV{COMETOS_DIR}/src/addressing" "$ENV{COMETOS_DIR}/src/core/platform")
include_directories("$ENV{COMETOS_DIR}/src/templates")


set(core_INCLUDE_DIRS 
	"$ENV{COMETOS_DIR}/src/core/platform"
	"$ENV{COMETOS_DIR}/src/templates"
	PARENT_SCOPE
) 

file(GLOB core_SRC "$ENV{COMETOS_DIR}/src/core/platform/*.cc")
add_library (core ${core_SRC} "$ENV{COMETOS_DIR}/src/core/platform/memory/BlockAllocator.cc")
target_link_libraries(core platform)


