add_executable(MenuTest WIN32 MenuTest.cpp MenuTest_res.rc)
target_link_libraries(MenuTest comctl32)

# do statically link
set_target_properties(MenuTest PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(MenuTest PROPERTIES LINK_SEARCH_END_STATIC 1)
