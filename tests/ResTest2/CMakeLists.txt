add_executable(ResTest2 WIN32 ResTest2.cpp ResTest2_res.rc)
target_link_libraries(ResTest2 comctl32)

# do statically link
set_target_properties(ResTest2 PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(ResTest2 PROPERTIES LINK_SEARCH_END_STATIC 1)
