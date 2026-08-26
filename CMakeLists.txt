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

# Diretórios de inclusão
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}
)

# Alvo principal
add_executable(melanoma_pdi
    src/main.cpp
)

# Vincula as bibliotecas do OpenCV ao executável
target_link_libraries(melanoma_pdi PRIVATE ${OpenCV_LIBS})

# No Windows, copia as DLLs do OpenCV para a pasta do executavel apos a compilacao
if(WIN32 AND EXISTS "${OpenCV_DIR}/x64/mingw/bin")
    add_custom_command(TARGET melanoma_pdi POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OpenCV_DIR}/x64/mingw/bin"
            "$<TARGET_FILE_DIR:melanoma_pdi>"
    )
endif()
