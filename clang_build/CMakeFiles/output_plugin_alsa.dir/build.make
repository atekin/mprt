# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.11

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
CMAKE_SOURCE_DIR = /media/sf_arch_ortak/mprt/src

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/sf_arch_ortak/mprt/clang_build

# Include any dependencies generated for this target.
include CMakeFiles/output_plugin_alsa.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/output_plugin_alsa.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/output_plugin_alsa.dir/flags.make

CMakeFiles/output_plugin_alsa.dir/core/config.cpp.o: CMakeFiles/output_plugin_alsa.dir/flags.make
CMakeFiles/output_plugin_alsa.dir/core/config.cpp.o: /media/sf_arch_ortak/mprt/src/core/config.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/sf_arch_ortak/mprt/clang_build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/output_plugin_alsa.dir/core/config.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/output_plugin_alsa.dir/core/config.cpp.o -c /media/sf_arch_ortak/mprt/src/core/config.cpp

CMakeFiles/output_plugin_alsa.dir/core/config.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/output_plugin_alsa.dir/core/config.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/sf_arch_ortak/mprt/src/core/config.cpp > CMakeFiles/output_plugin_alsa.dir/core/config.cpp.i

CMakeFiles/output_plugin_alsa.dir/core/config.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/output_plugin_alsa.dir/core/config.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/sf_arch_ortak/mprt/src/core/config.cpp -o CMakeFiles/output_plugin_alsa.dir/core/config.cpp.s

CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.o: CMakeFiles/output_plugin_alsa.dir/flags.make
CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.o: /media/sf_arch_ortak/mprt/src/plugins/output_plugins/output_plugin_alsa.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/sf_arch_ortak/mprt/clang_build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.o -c /media/sf_arch_ortak/mprt/src/plugins/output_plugins/output_plugin_alsa.cpp

CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/sf_arch_ortak/mprt/src/plugins/output_plugins/output_plugin_alsa.cpp > CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.i

CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/sf_arch_ortak/mprt/src/plugins/output_plugins/output_plugin_alsa.cpp -o CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.s

# Object files for target output_plugin_alsa
output_plugin_alsa_OBJECTS = \
"CMakeFiles/output_plugin_alsa.dir/core/config.cpp.o" \
"CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.o"

# External object files for target output_plugin_alsa
output_plugin_alsa_EXTERNAL_OBJECTS =

liboutput_plugin_alsa.so: CMakeFiles/output_plugin_alsa.dir/core/config.cpp.o
liboutput_plugin_alsa.so: CMakeFiles/output_plugin_alsa.dir/plugins/output_plugins/output_plugin_alsa.cpp.o
liboutput_plugin_alsa.so: CMakeFiles/output_plugin_alsa.dir/build.make
liboutput_plugin_alsa.so: /usr/lib/libboost_system.so
liboutput_plugin_alsa.so: /usr/lib/libboost_filesystem.so
liboutput_plugin_alsa.so: /usr/lib/libboost_log.so
liboutput_plugin_alsa.so: /usr/lib/libasound.so
liboutput_plugin_alsa.so: CMakeFiles/output_plugin_alsa.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/media/sf_arch_ortak/mprt/clang_build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX shared library liboutput_plugin_alsa.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/output_plugin_alsa.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/output_plugin_alsa.dir/build: liboutput_plugin_alsa.so

.PHONY : CMakeFiles/output_plugin_alsa.dir/build

CMakeFiles/output_plugin_alsa.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/output_plugin_alsa.dir/cmake_clean.cmake
.PHONY : CMakeFiles/output_plugin_alsa.dir/clean

CMakeFiles/output_plugin_alsa.dir/depend:
	cd /media/sf_arch_ortak/mprt/clang_build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/sf_arch_ortak/mprt/src /media/sf_arch_ortak/mprt/src /media/sf_arch_ortak/mprt/clang_build /media/sf_arch_ortak/mprt/clang_build /media/sf_arch_ortak/mprt/clang_build/CMakeFiles/output_plugin_alsa.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/output_plugin_alsa.dir/depend
