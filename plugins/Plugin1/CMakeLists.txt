# Plugin1.risoh
add_library(Plugin1 SHARED Plugin1.c Plugin1.def)
target_compile_definitions(Plugin1 PRIVATE -DPLUGIN_BUILD)
set_target_properties(Plugin1 PROPERTIES OUTPUT_NAME "Plugin1.risoh")
set_target_properties(Plugin1 PROPERTIES PREFIX "")
set_target_properties(Plugin1 PROPERTIES SUFFIX "")
target_compile_definitions(Plugin1 PRIVATE -DUNICODE -D_UNICODE)
