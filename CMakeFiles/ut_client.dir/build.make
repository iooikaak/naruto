# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.15.5/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.15.5/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/wangzhenkui/cproj/naruto

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/wangzhenkui/cproj/naruto

# Include any dependencies generated for this target.
include CMakeFiles/ut_client.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/ut_client.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/ut_client.dir/flags.make

CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.o: CMakeFiles/ut_client.dir/flags.make
CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.o: unittest/ut_client_main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/wangzhenkui/cproj/naruto/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.o"
	/Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.o -c /Users/wangzhenkui/cproj/naruto/unittest/ut_client_main.cpp

CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/wangzhenkui/cproj/naruto/unittest/ut_client_main.cpp > CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.i

CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/wangzhenkui/cproj/naruto/unittest/ut_client_main.cpp -o CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.s

# Object files for target ut_client
ut_client_OBJECTS = \
"CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.o"

# External object files for target ut_client
ut_client_EXTERNAL_OBJECTS =

ut_client: CMakeFiles/ut_client.dir/unittest/ut_client_main.cpp.o
ut_client: CMakeFiles/ut_client.dir/build.make
ut_client: CMakeFiles/ut_client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/wangzhenkui/cproj/naruto/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable ut_client"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/ut_client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/ut_client.dir/build: ut_client

.PHONY : CMakeFiles/ut_client.dir/build

CMakeFiles/ut_client.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ut_client.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ut_client.dir/clean

CMakeFiles/ut_client.dir/depend:
	cd /Users/wangzhenkui/cproj/naruto && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/wangzhenkui/cproj/naruto /Users/wangzhenkui/cproj/naruto /Users/wangzhenkui/cproj/naruto /Users/wangzhenkui/cproj/naruto /Users/wangzhenkui/cproj/naruto/CMakeFiles/ut_client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ut_client.dir/depend

