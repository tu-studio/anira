cmake_minimum_required(VERSION 3.16)
project(minimal-tflite C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(minimal-tflite
  minimal-tflite.cpp
)

target_link_libraries(minimal-tflite
  anira::anira
  tensorflowlite_c
)

if (MSVC)
    foreach(DLL ${ANIRA_SHARED_LIBS_WIN})
        add_custom_command(TARGET minimal-tflite
                PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL}
                $<TARGET_FILE_DIR:minimal-tflite>)
    endforeach()
endif (MSVC)