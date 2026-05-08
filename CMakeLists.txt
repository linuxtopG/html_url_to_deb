cmake_minimum_required(VERSION 3.21)
project(webapp-converter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# AUTOMOC disabled intentionally - embedded templates contain "Q_OBJECT" text
# for generated projects, which would confuse the MOC scanner

find_package(Qt6 REQUIRED COMPONENTS Core)

set(SOURCES
    src/main.cpp
    src/ProjectGenerator.cpp
)

set(HEADERS
    src/ProjectGenerator.h
)

qt_add_executable(webapp-converter ${SOURCES} ${HEADERS})

target_link_libraries(webapp-converter PRIVATE Qt6::Core)

install(TARGETS webapp-converter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "WebApp Converter Team")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "webapp-converter-${PROJECT_VERSION}-linux-amd64")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Convert web content to standalone Debian desktop applications")

include(CPack)
