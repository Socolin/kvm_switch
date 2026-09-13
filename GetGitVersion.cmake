execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --dirty=-custom
        WORKING_DIRECTORY ${SOURCE_DIR}
        OUTPUT_VARIABLE LATEST_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
)

if(NOT LATEST_HASH)
    set(LATEST_HASH "unknown")
endif()

set(FILE_CONTENT "#pragma once\n#define GIT_HASH \"${LATEST_HASH}\"\n")
set(TARGET_FILE "${BINARY_DIR}/generated/git_version.h")

# Read the old file if it exists and ONLY write to the file if the hash changed.
# This prevents your project from completely rebuilding every single time you type 'make'.
if(EXISTS ${TARGET_FILE})
    file(READ ${TARGET_FILE} OLD_CONTENT)
endif()

if(NOT "${FILE_CONTENT}" STREQUAL "${OLD_CONTENT}")
    file(WRITE ${TARGET_FILE} "${FILE_CONTENT}")
endif()