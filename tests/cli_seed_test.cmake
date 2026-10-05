foreach(value IN ITEMS nan inf -inf 30junk "" 1e999)
    execute_process(COMMAND "${PANEL_EXECUTABLE}" --seed-hue "${value}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 1 OR NOT error MATCHES "finite number")
        message(FATAL_ERROR "Invalid seed '${value}' was not rejected correctly: ${result} ${error}")
    endif()
endforeach()
foreach(value IN ITEMS 0 -6 360 720.25)
    execute_process(COMMAND "${PANEL_EXECUTABLE}" --seed-hue "${value}" --self-test
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Valid seed '${value}' failed: ${result} ${error}")
    endif()
endforeach()
