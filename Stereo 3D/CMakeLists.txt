cmake_minimum_required(VERSION 3.15)
project(BinocularTakens VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE")

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE not found at ${JUCE_PATH}.\n"
        "Run: git clone https://github.com/juce-framework/JUCE.git JUCE")
endif()

add_subdirectory(${JUCE_PATH} JUCE)

juce_add_plugin(BinocularTakens
    VERSION                     2.0.0
    COMPANY_NAME                "PerceptionLab"
    PLUGIN_MANUFACTURER_CODE    PrLb
    PLUGIN_CODE                 BnTk
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Binocular Takens 3D"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx|Visualization"
)

juce_generate_juce_header(BinocularTakens)

target_sources(BinocularTakens PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
)

target_compile_definitions(BinocularTakens PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BinocularTakens PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)