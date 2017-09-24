add_executable(DialogResTest WIN32 DialogResTest.cpp DialogResTest_res.rc)
target_link_libraries(DialogResTest comctl32)

# do statically link
set_target_properties(DialogResTest PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(DialogResTest PROPERTIES LINK_SEARCH_END_STATIC 1)
