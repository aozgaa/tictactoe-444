cmake_minimum_required(VERSION 3.28)

if(NOT DEFINED PREPARE_BUILD_TYPE)
    set(PREPARE_BUILD_TYPE Release)
endif()

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(PREFIX_DIR "${PROJECT_ROOT}/env")
set(BUILD_ROOT "${PROJECT_ROOT}/build/prepare")

include(CMakeParseArguments)

function(configure_build_install name source_dir)
    set(options)
    set(one_value_args)
    set(multi_value_args CONFIGURE_ARGS TARGETS)
    cmake_parse_arguments(
        cbi
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    set(build_dir "${BUILD_ROOT}/${name}")
    if(EXISTS "${build_dir}/CMakeCache.txt")
        file(REMOVE_RECURSE "${build_dir}")
    endif()
    execute_process(
        COMMAND
            ${CMAKE_COMMAND} -S "${source_dir}" -B "${build_dir}"
            -DCMAKE_BUILD_TYPE=${PREPARE_BUILD_TYPE}
            -DCMAKE_INSTALL_PREFIX=${PREFIX_DIR} ${cbi_CONFIGURE_ARGS}
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "configure failed for ${name}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build "${build_dir}" -- ${cbi_TARGETS}
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "build failed for ${name}")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --install "${build_dir}"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "install failed for ${name}")
    endif()
endfunction()

configure_build_install(doctest "${PROJECT_ROOT}/ext/doctest"
  CONFIGURE_ARGS
    -DDOCTEST_WITH_TESTS=OFF
    -DDOCTEST_WITH_MAIN_IN_STATIC_LIB=OFF
)

configure_build_install(sdl2 "${PROJECT_ROOT}/ext/sdl"
  CONFIGURE_ARGS
    -DSDL_SHARED=OFF
    -DSDL_STATIC=ON
    -DSDL_TEST=OFF
    -DSDL_TESTS=OFF
    -DSDL_METAL=ON
    -DSDL2_DISABLE_SDL2MAIN=OFF
)
