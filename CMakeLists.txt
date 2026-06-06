cmake_minimum_required(VERSION 3.16)
project(log-query VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(CLI11 INTERFACE)
target_include_directories(CLI11 INTERFACE ${CMAKE_SOURCE_DIR}/third_party)
add_library(CLI11::CLI11 ALIAS CLI11)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/third_party)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)

set(SOURCES
    src/main.cpp
    src/config/log_format.cpp
    src/parser/field.cpp
    src/parser/line_parser.cpp
    src/filter/filter_chain.cpp
    src/filter/field_filter.cpp
    src/filter/regex_filter.cpp
    src/filter/time_range_filter.cpp
    src/output/color_renderer.cpp
)

set(HEADERS
    include/log_query/log_line.h
    include/log_query/filter.h
    include/log_query/renderer.h
    src/config/log_format.h
    src/parser/field.h
    src/parser/line_parser.h
    src/filter/filter_chain.h
    src/filter/field_filter.h
    src/filter/regex_filter.h
    src/filter/time_range_filter.h
    src/output/color_renderer.h
    src/util/ansi.h
    src/util/file_reader.h
    src/util/time_parse.h
)

add_executable(log-query ${SOURCES} ${HEADERS})

target_include_directories(log-query PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(log-query PRIVATE
    CLI11::CLI11
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_compile_definitions(log-query PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

install(TARGETS log-query RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/log-query/config)
