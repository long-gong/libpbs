# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /media/saber/easystore/libpbs

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/saber/easystore/libpbs/build-new

# Include any dependencies generated for this target.
include CMakeFiles/iblt.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/iblt.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/iblt.dir/flags.make

CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o: CMakeFiles/iblt.dir/flags.make
CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o: ../3rd/include/iblt/iblt.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/saber/easystore/libpbs/build-new/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o -c /media/saber/easystore/libpbs/3rd/include/iblt/iblt.cpp

CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/saber/easystore/libpbs/3rd/include/iblt/iblt.cpp > CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.i

CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/saber/easystore/libpbs/3rd/include/iblt/iblt.cpp -o CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.s

CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.requires:

.PHONY : CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.requires

CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.provides: CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.requires
	$(MAKE) -f CMakeFiles/iblt.dir/build.make CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.provides.build
.PHONY : CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.provides

CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.provides.build: CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o


iblt: CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o
iblt: CMakeFiles/iblt.dir/build.make

.PHONY : iblt

# Rule to build all files generated by this target.
CMakeFiles/iblt.dir/build: iblt

.PHONY : CMakeFiles/iblt.dir/build

CMakeFiles/iblt.dir/requires: CMakeFiles/iblt.dir/3rd/include/iblt/iblt.cpp.o.requires

.PHONY : CMakeFiles/iblt.dir/requires

CMakeFiles/iblt.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/iblt.dir/cmake_clean.cmake
.PHONY : CMakeFiles/iblt.dir/clean

CMakeFiles/iblt.dir/depend:
	cd /media/saber/easystore/libpbs/build-new && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/saber/easystore/libpbs /media/saber/easystore/libpbs /media/saber/easystore/libpbs/build-new /media/saber/easystore/libpbs/build-new /media/saber/easystore/libpbs/build-new/CMakeFiles/iblt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/iblt.dir/depend

