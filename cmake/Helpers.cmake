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

function(SignAndDeploy)
    install(CODE [[
        if (APPLE)
            message("Signing...")
            execute_process(
                COMMAND /bin/bash "${CMAKE_CURRENT_LIST_DIR}/../deployScripts/mac/sign.sh" "${CMAKE_CURRENT_LIST_DIR}/install/TorrentPlayer.app"
                RESULT_VARIABLE result
                OUTPUT_VARIABLE out
                ERROR_VARIABLE err
            )
            if (NOT result EQUAL 0)
                message(FATAL_ERROR "SIGNING failed (code ${result})\n${err}\n${out}")
            endif()
            message("Signed successfully")

            message("Creating DMG...")
            execute_process(
                COMMAND /bin/bash "${CMAKE_CURRENT_LIST_DIR}/../deployScripts/mac/restyleDmg.sh" "${CMAKE_CURRENT_LIST_DIR}"
                RESULT_VARIABLE result
                OUTPUT_VARIABLE out
                ERROR_VARIABLE err
            )
            if (NOT result EQUAL 0)
                message(FATAL_ERROR "DMG creation failed (code ${result})\n${err}\n${out}")
            endif()
            message("DMG created!")
        endif()
    ]])
endfunction()
