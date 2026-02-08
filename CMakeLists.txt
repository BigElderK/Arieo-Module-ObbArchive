cmake_minimum_required (VERSION 3.31)
    arieo_engine_project(
        arieo_obb_archive_module
        ALIAS arieo::modules::archive::obb
        PROJECT_TYPE module
        SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/private/src/*.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/private/src/*/*.cpp
        INTERFACES
            arieo::interface::archive
        PACKAGES
            zlib
        LIBS
            arieo::base
            arieo::core
            ZLIB::ZLIB
    )