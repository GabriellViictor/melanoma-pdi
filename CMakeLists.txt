cmake_minimum_required(VERSION 3.20)
project(melanoma_pdi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Procura caminhos comuns do OpenCV no Windows caso não tenha sido especificado
if(NOT OpenCV_FOUND AND NOT DEFINED OpenCV_DIR)
    set(OPENCV_CANDIDATE_PATHS
        "C:/OpenCV-MinGW-Build-OpenCV-4.5.5-x64"
        "C:/opencv/build"
        "C:/opencv"
        "$ENV{OpenCV_DIR}"
        "$ENV{PDI_OPENCV_DIR}"
    )
    foreach(PATH ${OPENCV_CANDIDATE_PATHS})
        if(EXISTS "${PATH}/OpenCVConfig.cmake" OR EXISTS "${PATH}/x64/mingw/lib/OpenCVConfig.cmake")
            set(OpenCV_DIR "${PATH}" CACHE PATH "OpenCV installation directory" FORCE)
            break()
        endif()
    endforeach()
endif()

# Encontra a biblioteca OpenCV
find_package(OpenCV REQUIRED)

message(STATUS "OpenCV encontrada: versao ${OpenCV_VERSION}")
message(STATUS "OpenCV config: ${OpenCV_DIR}")

# Biblioteca base compartilhada entre o CLI e os testes
add_library(melanoma_core STATIC
    src/io/DatasetManager.cpp
    src/core/ImageProcessor.cpp
)

target_include_directories(melanoma_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(melanoma_core PUBLIC ${OpenCV_LIBS})

# Alvo principal CLI
add_executable(melanoma_pdi
    src/main.cpp
)

target_link_libraries(melanoma_pdi PRIVATE melanoma_core)

# Testes automatizados (CTest)
enable_testing()

add_executable(melanoma_tests
    tests/test_dataset_io.cpp
)

target_link_libraries(melanoma_tests PRIVATE melanoma_core)

add_test(NAME core_pipeline_test COMMAND melanoma_tests --test-pipeline)
add_test(NAME dataset_integrity COMMAND melanoma_tests --test-dataset)
add_test(NAME pipeline_smoke_test COMMAND melanoma_tests --test-processing)

# No Windows, copia as DLLs do OpenCV para a pasta dos executaveis apos a compilacao
if(WIN32 AND EXISTS "${OpenCV_DIR}/x64/mingw/bin")
    add_custom_command(TARGET melanoma_pdi POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OpenCV_DIR}/x64/mingw/bin"
            "$<TARGET_FILE_DIR:melanoma_pdi>"
    )
    add_custom_command(TARGET melanoma_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OpenCV_DIR}/x64/mingw/bin"
            "$<TARGET_FILE_DIR:melanoma_tests>"
    )
endif()
