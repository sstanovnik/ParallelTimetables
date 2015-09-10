cmake_minimum_required(VERSION 3.0) # not latest because the ubuntu package is currently on 3.0.2
project(ParallelTimetables)

find_package(Boost COMPONENTS mpi serialization REQUIRED)
find_package(MPI REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${MPI_CXX_INCLUDE_PATH})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -pipe -Wno-unused-parameter -O3 -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3 -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

set(TINYXML2
    tinyxml2/tinyxml2.h tinyxml2/tinyxml2.cpp
)

set(JSON
    json/json.hpp
)

set(GENETIC
    genetic/crossover.h genetic/crossover.cpp
    genetic/fitness.h   genetic/fitness.cpp
    genetic/mutation.h  genetic/mutation.cpp
    genetic/selection.h genetic/selection.cpp
)

set(SHARED_FILES
    ${TINYXML2}
    ${JSON}
    ${GENETIC}
    timetable_types.h
    timetable.h      timetable.cpp
    performance.h    performance.cpp
    settings.h       settings.cpp
    utils.h          utils.cpp
    import.h         import.cpp
    custom_mpi.h     custom_mpi.cpp
)

add_executable(main_launch ${SHARED_FILES} main.cpp)

set_target_properties(main_launch PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)

# it is very important to have this ; separated instead of space-separated
target_link_libraries(main_launch "${Boost_LIBRARIES};${MPI_CXX_LIBRARIES}")
