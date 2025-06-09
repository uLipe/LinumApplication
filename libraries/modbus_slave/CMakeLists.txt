target_sources(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/mdb_table_parse.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)
