function(AbsToRelPath OUT_VAR BASE_DIR)
    set(result)
    foreach(file IN LISTS ARGN)
        if (IS_ABSOLUTE "${file}")
            file(RELATIVE_PATH rel "${BASE_DIR}" "${file}")
        else()
            set(rel "${file}")
        endif()
        list(APPEND result "${rel}")
    endforeach()
    set(${OUT_VAR} "${result}" PARENT_SCOPE)
endfunction()

# Function: include_sources
# Usage: include_sources(OUT_VAR GLOB_PATTERN1 [GLOB_PATTERN2 ...])
# This function will GLOB_RECURSE sources using the given patterns,
# then exclude platform-specific directories except for the current platform.
function(include_sources OUT_VAR)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs PATTERNS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_PATTERNS)
        set(ARG_PATTERNS ${ARGN})
    endif()

    set(_PLATFORMS android mac ios) # @TODO: extend the list for other platforms or think on making it more general
    set(_IS_ANDROID OFF)
    set(_IS_MAC OFF)
    set(_IS_IOS OFF)
    if (ANDROID)
        set(_IS_ANDROID ON)
    elseif (IOS)
        set(_IS_IOS ON)
    elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(_IS_MAC ON)
    endif()

    # Collect all sources recursively from given patterns
    set(_sources)
    foreach(_pattern IN LISTS ARG_PATTERNS)
        file(GLOB_RECURSE _pattern_sources CONFIGURE_DEPENDS "${_pattern}")
        list(APPEND _sources ${_pattern_sources})
    endforeach()

    # Exclude platform-specific directories that do not match the current platform
    foreach(_platform IN LISTS _PLATFORMS)
        string(TOLOWER "${_platform}" _platform_lower)
        set(_is_current OFF)
        if ("${_platform}" STREQUAL "android" AND _IS_ANDROID)
            set(_is_current ON)
        elseif("${_platform}" STREQUAL "ios" AND _IS_IOS)
            set(_is_current ON)
        elseif("${_platform}" STREQUAL "mac" AND _IS_MAC)
            set(_is_current ON)
        endif()
        if (NOT _is_current)
            list(FILTER _sources EXCLUDE REGEX ".*/platform/${_platform_lower}/.*")
        endif()
    endforeach()

    set(${OUT_VAR} "${_sources}" PARENT_SCOPE)
endfunction()
