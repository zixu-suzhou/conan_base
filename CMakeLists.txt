cmake_minimum_required(VERSION 3.10.1)

project(test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")


add_definitions(-std=c++11)
#conan
include($ENV{MF_SYSTEM_ROOT_DIR}/package/utils.cmake)
activate_common_configuration()
auto_build_type_and_compile_flags() # release type 加了 -O3，debug type 加了 -Og -g
configure_output_directories()
set(CONAN_INSTALL_ARGS "--generator cmake")
conan_install()
include(${CONAN_BUILD_INFO_CMAKE})
conan_basic_setup()
include_directories(SYSTEM ${CONAN_INCLUDE_DIRS})
configure_version_h()
configure_output_directories()
set_property(GLOBAL PROPERTY CONANBUILDINFO_FILE "${PROJECT_BINARY_DIR}/conanbuildinfo.cmake" )

function(print_all_variables)
    get_cmake_property(vars VARIABLES)
    list(SORT vars)
    message(STATUS "all variables:")
    foreach (var ${vars})
        message(STATUS "-   ${var}=${${var}}")
    endforeach()
endfunction()
#print_all_variables()


include_directories(./include)

add_executable ( ${PROJECT_NAME} src/main.cpp src/status_monitor.cpp)
target_link_libraries (
  ${PROJECT_NAME}
  pthread
  )
install(
  TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)

