# CMAKE SCRIPT FOR BUILDING PLATFORM ABSTRACTION LAYER

# load configuration script for building board
include("boards/${BOARD}/board.cmake") 

# define pal library
add_library (platform ${platform_SRC} "common/TaskScheduler.cc")

# add variable to parent scope
set(platform_INCLUDE_DIRS ${platform_INCLUDE_DIRS} 
	"$ENV{COMETOS_DIR}/src/platform/interfaces"
	"$ENV{COMETOS_DIR}/src/platform/common"
	PARENT_SCOPE
) 

