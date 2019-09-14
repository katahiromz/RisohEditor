add_executable(ResTest WIN32 ResTest.cpp ResTest_res.rc ResTest.exe.manifest)
target_link_libraries(ResTest comctl32)

# do statically link
set_target_properties(ResTest PROPERTIES LINK_SEARCH_START_STATIC 1)
set_target_properties(ResTest PROPERTIES LINK_SEARCH_END_STATIC 1)
