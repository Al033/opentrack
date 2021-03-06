set(new-hier-path "#pragma once
#ifndef OPENTRACK_NO_QT_PATH

#   include <QCoreApplication>
#   include <QString>

#   define OPENTRACK_BASE_PATH (([]() -> const QString& { \\
        const static QString const__path___ = QCoreApplication::applicationDirPath(); \\
        return const__path___; \\
        })())
#endif
#define OPENTRACK_LIBRARY_PATH \"${opentrack-hier-path}\"
#define OPENTRACK_DOC_PATH \"${opentrack-hier-doc}\"
#define OPENTRACK_CONTRIB_PATH \"${opentrack-hier-doc}contrib/\"
")

set(hier-path-filename "${CMAKE_BINARY_DIR}/opentrack-library-path.h")
set(orig-hier-path "")
if(EXISTS ${hier-path-filename})
    file(READ ${hier-path-filename} orig-hier-path)
endif()
if(NOT (orig-hier-path STREQUAL new-hier-path))
    file(WRITE ${hier-path-filename} "${new-hier-path}")
endif()

install(FILES "${CMAKE_SOURCE_DIR}/CMakeLists.txt" DESTINATION "${opentrack-doc-src-pfx}")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/cmake" DESTINATION "${opentrack-doc-src-pfx}")

function(opentrack_set_globs n)
    set(dir ${PROJECT_SOURCE_DIR})
    file(GLOB ${n}-c ${dir}/*.cpp ${dir}/*.c ${dir}/*.h ${dir}/*.hpp)
    file(GLOB ${n}-res ${dir}/*.rc)
    foreach(f ${n}-res)
        set_source_files_properties(${f} PROPERTIES LANGUAGE RC)
    endforeach()
    file(GLOB ${n}-ui ${dir}/*.ui)
    file(GLOB ${n}-rc ${dir}/*.qrc)
    foreach(i c res ui rc)
        set(${n}-${i} ${${n}-${i}} PARENT_SCOPE)
    endforeach()
endfunction()

function(opentrack_qt n)
    qt5_wrap_cpp(${n}-moc ${${n}-c} OPTIONS --no-notes)
    QT5_WRAP_UI(${n}-uih ${${n}-ui})
    QT5_ADD_RESOURCES(${n}-rcc ${${n}-rc})
    set(${n}-all ${${n}-c} ${${n}-rc} ${${n}-rcc} ${${n}-uih} ${${n}-moc} ${${n}-res})
    foreach(i moc uih rcc all)
        set(${n}-${i} ${${n}-${i}} PARENT_SCOPE)
    endforeach()
endfunction()

function(opentrack_compat target)
    if(MSVC)
        set(msvc-subsystem "/VERSION:5.1 /SUBSYSTEM:WINDOWS,5.01")
        set_target_properties(${target} PROPERTIES LINK_FLAGS "${msvc-subsystem} /DEBUG /OPT:ICF")
    endif()
    if(NOT MSVC)
        set_property(SOURCE ${${target}-moc} APPEND_STRING PROPERTY COMPILE_FLAGS "-w -Wno-error")
    endif()
    if(WIN32)
        target_link_libraries(${target} dinput8 dxguid strmiids)
    endif()
endfunction()

include(CMakeParseArguments)

function(opentrack_boilerplate__ n files_ no-library_ static_ no-compat_ compile_ link_ stage2_ bin_)
    if((NOT no-library_) AND (NOT stage2_))
        set(link-mode SHARED)
        if (static_)
            set(link-mode STATIC)
        endif()
        add_library(${n} ${link-mode} ${files_})
        set(all-files ${${n}-c} ${${n}-res} ${${n}-ui} ${${n}-rc})
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt" DESTINATION "${opentrack-doc-src-pfx}/${n}")
        install(FILES ${all-files} DESTINATION "${opentrack-doc-src-pfx}/${n}")
        message(STATUS "module ${n}")
    endif()
    if(NOT no-library_)
        opentrack_compat(${n})
    endif()
    if(NOT no-compat_)
        target_link_libraries(${n} opentrack-api opentrack-compat)
    endif()
    target_link_libraries(${n} ${MY_QT_LIBS})
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(c-props "-fvisibility=hidden -fuse-cxa-atexit")
    endif()
    if(CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)
        set(l-props "-Wl,--as-needed")
    endif()
    if(MSVC)
        set(l-props "${msvc-subsystem} /DEBUG /OPT:ICF")
    endif()
    get_target_property(orig-compile ${n} COMPILE_FLAGS)
    if(NOT orig-compile)
        set(orig-compile "")
    endif()
    get_target_property(orig-link ${n} LINK_FLAGS)
    if(NOT orig-link)
        set(orig-link "")
    endif()
    set_target_properties(${n} PROPERTIES
        COMPILE_FLAGS "${c-props} ${orig-compile} ${compile_}"
        LINK_FLAGS "${l-props} ${orig-link} ${link_}"
    )
    string(REGEX REPLACE "^opentrack-" "" n_ ${n})
    string(REPLACE "-" "_" n_ ${n_})
    target_compile_definitions(${n} PRIVATE "BUILD_${n_}")
    if((NOT static_) AND (NOT no-library_))
        if(bin_ AND WIN32)
            install(TARGETS ${n} RUNTIME DESTINATION . LIBRARY DESTINATION .)
        else()
            install(TARGETS ${n} ${opentrack-hier-str})
        endif()
    endif()
endfunction()

macro(opentrack_boilerplate n)
    cmake_parse_arguments(${n}-args
        "NO-LIBRARY;STATIC;NO-COMPAT;STAGE2;BIN"
        "LINK;COMPILE"
        ""
        ${ARGN}
    )
    if(NOT "${${n}-args_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "opentrack_boilerplate bad formals ${${n}-args_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ${n}-args_STAGE2)
        project(${n} C CXX)
        opentrack_set_globs(${n})
        opentrack_qt(${n})
    endif()
    if((NOT ${n}-args_NO-LIBRARY) OR ${n}-args_STAGE2)
        opentrack_boilerplate__("${n}" "${${n}-all}"
                                       "${${n}-args_NO-LIBRARY}"
                                       "${${n}-args_STATIC}"
                                       "${${n}-args_NO-COMPAT}"
                                       "${${n}-args_COMPILE}"
                                       "${${n}-args_LINK}"
                                       "${${n}-args_STAGE2}"
                                       "${${n}-args_BIN}")
    endif()
endmacro()
