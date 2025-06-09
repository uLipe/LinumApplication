target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/mask_format.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
