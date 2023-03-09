# Plugin1.risoh
add_library(Plugin1 SHARED Plugin1.cpp Plugin1.def)
target_compile_definitions(Plugin1 PRIVATE -DPLUGIN_BUILD)
set_target_properties(Plugin1 PROPERTIES OUTPUT_NAME "Plugin1")
set_target_properties(Plugin1 PROPERTIES PREFIX "")
set_target_properties(Plugin1 PROPERTIES SUFFIX ".risoh")
target_compile_definitions(Plugin1 PRIVATE -DUNICODE -D_UNICODE)
