cmake_minimum_required (VERSION 3.31)

include($ENV{ARIEO_BUILDENV_PACKAGE_INSTALL_FOLDER}/cmake/build_environment.cmake)
project(ArieoEngine)

arieo_engine_project(
    arieo_obb_archive_module
    ALIAS arieo::modules::archive::obb
    PROJECT_TYPE module
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/private/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/private/src/*/*.cpp
    INTERFACES
        Arieo-Interface-Archive::arieo_archive_interface
    PACKAGES
        zlib
    PRIVATE_LIBS
        arieo::base
        arieo::core
        ZLIB::ZLIB
)