cmake_minimum_required(VERSION 3.16)
project(matchroom-scoreboard VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Qt compilation tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Discover OBS development dependencies
find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)

# Discover the exact Qt module structures
find_package(Qt6 COMPONENTS Widgets Core REQUIRED)

# Compile the single unified target library
add_library(matchroom-scoreboard MODULE
    src/plugin-main.cpp
)

target_link_libraries(matchroom-scoreboard
    PRIVATE
        libobs
        obs-frontend-api
        Qt6::Widgets
        Qt6::Core
)

# Automated installer destination definition
install(TARGETS matchroom-scoreboard DESTINATION "obs-plugins/64bit")
