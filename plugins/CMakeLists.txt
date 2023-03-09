# libPluginFramework.a
add_library(PluginFramework STATIC PluginFramework.cpp)
target_compile_definitions(PluginFramework PRIVATE -DUNICODE -D_UNICODE)

# Plugins
# TODO: Add more plugins
add_subdirectory(Plugin1)
