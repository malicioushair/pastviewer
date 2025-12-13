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

# Add Sentry Android SDK dependency to build.gradle automatically
# Qt regenerates build.gradle during build, so we inject it via a custom command
# Usage: add_sentry_android_dependency(<target_name>)
function(add_sentry_android_dependency TARGET_NAME)
    if (NOT ANDROID)
        return()
    endif()

    set(ANDROID_BUILD_DIR "${CMAKE_BINARY_DIR}/android-build")
    set(BUILD_GRADLE_FILE "${ANDROID_BUILD_DIR}/build.gradle")

    # Create a script that adds Sentry dependency to build.gradle
    set(ADD_SENTRY_SCRIPT "${CMAKE_BINARY_DIR}/add_sentry_to_gradle.sh")
    file(WRITE ${ADD_SENTRY_SCRIPT} "#!/bin/bash
# This script adds Sentry Android SDK dependency to the generated build.gradle
BUILD_GRADLE=\"${BUILD_GRADLE_FILE}\"
if [ ! -f \"\$BUILD_GRADLE\" ]; then
    exit 0  # File doesn't exist yet, will be created by Qt
fi

if grep -q \"io.sentry:sentry-android\" \"\$BUILD_GRADLE\"; then
    exit 0  # Already added
fi

# Create backup
cp \"\$BUILD_GRADLE\" \"\$BUILD_GRADLE.bak\"

# Add Sentry dependency after androidx.core:core line
# Use a more careful sed command that preserves the file structure
sed -i.tmp '/implementation.*androidx.core:core/a\\
    // Sentry Android SDK\\
    implementation '\''io.sentry:sentry-android:7.15.0'\''
' \"\$BUILD_GRADLE\"

# Verify the file is still valid (has android block)
if ! grep -q \"^android {\" \"\$BUILD_GRADLE\"; then
    echo \"Error: build.gradle structure corrupted, restoring from backup\"
    mv \"\$BUILD_GRADLE.bak\" \"\$BUILD_GRADLE\"
    exit 1
fi

# Clean up temp file
rm -f \"\$BUILD_GRADLE.tmp\"
")
    file(COPY ${ADD_SENTRY_SCRIPT} DESTINATION ${CMAKE_BINARY_DIR} FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Checking for Sentry dependency in build.gradle..."
        COMMAND bash -c "if [ -f '${BUILD_GRADLE_FILE}' ] && ! grep -q 'io.sentry:sentry-android' '${BUILD_GRADLE_FILE}'; then bash '${ADD_SENTRY_SCRIPT}'; fi"
        COMMENT "Ensuring Sentry dependency is in build.gradle"
        VERBATIM
    )
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
