cmake_minimum_required(VERSION 3.5)
project(barkplayer VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BARKPLAYER_BUILD_MINIMAL "Build the minimal backend smoke-test binary" ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(GTK IMPORTED_TARGET REQUIRED gtk+-2.0)
pkg_check_modules(XML IMPORTED_TARGET REQUIRED libxml-2.0)
pkg_check_modules(SQLite IMPORTED_TARGET REQUIRED sqlite3)
pkg_check_modules(GST IMPORTED_TARGET REQUIRED gstreamer-0.10)
pkg_check_modules(GTHREAD IMPORTED_TARGET REQUIRED gthread-2.0)

set(BARKPLAYER_SOURCES
    m4b_player.cpp
    music_backend.cpp
    database_manager.cpp
    mpeg4/mp4read.c
    mpeg4/unicode_support.c
)

add_executable(${PROJECT_NAME} ${BARKPLAYER_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::GTK
    PkgConfig::XML
    PkgConfig::SQLite
    PkgConfig::GST
    PkgConfig::GTHREAD
    Threads::Threads
    faad
    faad_drm
    lipc
    dl
    m
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

if(BARKPLAYER_BUILD_MINIMAL)
    add_executable(mb4reader-minimal
        minimal_example.cpp
        music_backend.cpp
        mpeg4/mp4read.c
        mpeg4/unicode_support.c
    )

    target_include_directories(mb4reader-minimal PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(mb4reader-minimal PRIVATE
        PkgConfig::GTK
        PkgConfig::XML
        PkgConfig::GST
        PkgConfig::GTHREAD
        Threads::Threads
        faad
        faad_drm
        dl
        m
    )

    target_compile_options(mb4reader-minimal PRIVATE -Wall -Wextra)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION BARKPlayer)
