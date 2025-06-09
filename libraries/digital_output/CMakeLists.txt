target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/digital_output.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
