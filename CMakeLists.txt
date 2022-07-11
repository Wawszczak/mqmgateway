cmake_minimum_required(VERSION 3.13)
project(modmqttd VERSION 1.0.0)

find_package(PkgConfig)
pkg_check_modules(LIBMODBUS REQUIRED libmodbus)

find_package(Threads)

find_package(yaml-cpp REQUIRED)
find_package(RapidJSON REQUIRED)

find_path(EXPRTK_INCLUDE_DIR "exprtk.hpp")

message(STATUS "EXPRTK_INCLUDE_DIR => ${EXPRTK_INCLUDE_DIR}")

include_directories(${YAML_CPP_INCLUDE_DIR} ${RAPIDJSON_INCLUDE_DIRS} .)
include_directories("/usr/local/include")

add_subdirectory(libmodmqttconv)
add_subdirectory(libmodmqttsrv)
add_subdirectory(stdconv)
add_subdirectory(modmqttd)
add_subdirectory(unittests)
if (EXPRTK_INCLUDE_DIR)
    include_directories(${EXPRTK_INCLUDE_DIR})
    add_subdirectory(exprconv)    
    target_compile_definitions(tests PRIVATE HAVE_EXPRTK)

    add_dependencies(modmqttd exprconv)
    add_dependencies(tests exprconv)

    install(TARGETS exprconv DESTINATION lib/modmqttd)
endif()

add_dependencies(modmqttd stdconv)
add_dependencies(tests stdconv)

install (TARGETS modmqttd DESTINATION bin)

install (TARGETS modmqttsrv DESTINATION lib)
install (DIRECTORY "${PROJECT_SOURCE_DIR}/libmodmqttconv" DESTINATION include FILES_MATCHING PATTERN "*.hpp")

install(TARGETS stdconv DESTINATION lib/modmqttd)
