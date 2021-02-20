cmake_minimum_required(VERSION 2.8)
project(aise C)
add_executable(aise ./src/main.c ./src/system.c ./src/functions.c)
set_property(TARGET aise PROPERTY C_STANDARD 99)

# Install
include(GNUInstallDirs)
install(TARGETS aise
	DESTINATION "${CMAKE_INSTALL_BINDIR}")

# Show Warnings
if (CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
