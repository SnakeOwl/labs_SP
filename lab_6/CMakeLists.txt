cmake_minimum_required (VERSION 3.10)
project (program VERSION 0.1 LANGUAGES CXX)
find_package (PkgConfig REQUIRED)
pkg_check_modules (GTK3 REQUIRED gtk+-3.0)
file (COPY main.glade
DESTINATION ./)
set (
SOURCES
main.cpp
)
add_executable (program ${SOURCES})
set_target_properties(
program PROPERTIES
CMAKE_CXX_STANDARD 17
CMAKE_CXX_STANDART_REQUIRED ON
)
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})
target_link_libraries(
program
PUBLIC stdc++fs
PUBLIC pthread
${GTK3_LIBRARIES}
)