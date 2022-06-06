add_executable(ToolbarTest WIN32 ToolbarTest.cpp ToolbarTest_res.rc)
target_link_libraries(ToolbarTest comctl32)

# do statically link
set_target_properties(ToolbarTest PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(ToolbarTest PROPERTIES LINK_SEARCH_END_STATIC 1)
