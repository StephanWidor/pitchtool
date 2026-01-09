cmake_minimum_required(VERSION 3.20)
project(jucepitchtool VERSION 0.0.1)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(juce-framework)

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
    IS_SYNTH FALSE
    LV2URI https://widor.site/lv2/pitchtool_juce
    COPY_PLUGIN_AFTER_BUILD FALSE
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

set(RESOURCE_LIB ${PROJECT_NAME}_resources)
juce_add_binary_data(${RESOURCE_LIB}
    SOURCES
    resources/reset_midi.png
    resources/reset_midi.svg
    NAMESPACE ${RESOURCE_LIB}
    )
if(CMAKE_COMPILER_IS_GNUCXX)
    message("detected gcc: setting position independent code for resources lib")
    set_target_properties(${RESOURCE_LIB}
        PROPERTIES
        POSITION_INDEPENDENT_CODE TRUE
        )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${PROJECT_NAME}_resources
    juce::juce_audio_utils
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    pitchtool-backend
    )

if(MSVC)
    set(ALL_WARNINGS_OPTIONS "/W4")
    set(NO_WARNINGS_OPTIONS "/w")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    set(ALL_WARNINGS_OPTIONS "-Wall;-Wextra;-Wpedantic")
    set(NO_WARNINGS_OPTIONS "-Wno-everything")
endif()
target_compile_options(${PROJECT_NAME} PRIVATE ${ALL_WARNINGS_OPTIONS})

set_source_files_properties(
    juce-framework/modules/juce_audio_plugin_client/juce_audio_plugin_client_VST3.cpp
    PROPERTIES
    COMPILE_FLAGS ${NO_WARNINGS_OPTIONS}
    )
