cmake_minimum_required(VERSION 3.15)
project(jucepitchtool VERSION 0.0.1)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AudioPluginHost from extras can be useful for debugging
#set(JUCE_BUILD_EXTRAS ON CACHE BOOL "Add build targets for the Projucer and other tools")
add_subdirectory(juce_lv2)

set(JUCE_PLUGIN_FORMATS Standalone VST3)

if(UNIX AND NOT APPLE)
    set(JUCE_PLUGIN_FORMATS ${JUCE_PLUGIN_FORMATS} LV2)
endif()

juce_add_plugin(${PROJECT_NAME}
    PLUGIN_MANUFACTURER_CODE SwPl
    PLUGIN_CODE swPT
    FORMATS ${JUCE_PLUGIN_FORMATS}
    COMPANY_NAME "SW"
    PRODUCT_NAME "pitchtool"
    PLUGIN_NAME "pitchtool"
    VST3_CATEGORIES "Fx"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    LV2_URI https://widor.site/lv2/pitchtool_juce
    )

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_sources(${PROJECT_NAME}
    PRIVATE
    sw/juce/pitchtool/editor.cpp
    sw/juce/pitchtool/editor.h
    sw/juce/pitchtool/processor.cpp
    sw/juce/pitchtool/processor.h
    sw/juce/ui/groupcomponent.cpp
    sw/juce/ui/groupcomponent.h
    sw/juce/ui/notedisplay.cpp
    sw/juce/ui/notedisplay.h
    sw/juce/ui/plot.cpp
    sw/juce/ui/plot.h
    sw/juce/ui/roundslider.cpp
    sw/juce/ui/roundslider.h
    sw/juce/ui/utils.h
    )

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    )

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    juce::juce_audio_utils
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    pitchtool-backend
    )
