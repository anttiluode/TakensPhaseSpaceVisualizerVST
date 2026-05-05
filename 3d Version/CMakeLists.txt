cmake_minimum_required(VERSION 3.15)
project(Takens3D VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE path
set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE")

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at ${JUCE_PATH}.\n"
        "Run: git clone https://github.com/juce-framework/JUCE.git JUCE")
endif()

add_subdirectory(${JUCE_PATH} JUCE)

juce_add_plugin(Takens3D
    VERSION                     1.0.0
    COMPANY_NAME                "PerceptionLab"
    PLUGIN_MANUFACTURER_CODE    PrLb
    PLUGIN_CODE                 Tk3D   # <-- CHANGED: New 4-letter ID so FL Studio knows it's a different plugin
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "3D Takens" # <-- CHANGED: The name that shows up in your DAW
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx|Visualization"
)

juce_generate_juce_header(Takens3D)

target_sources(Takens3D PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
)

target_compile_definitions(Takens3D PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Takens3D PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)