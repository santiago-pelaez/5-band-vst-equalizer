cmake_minimum_required(VERSION 3.24)

project(FiveBandEQ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "C:/JUCE" CACHE PATH "Path to the JUCE source tree")
endif()

if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE was not found. Configure with -DJUCE_DIR=C:/path/to/JUCE.")
endif()

add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

set(FIVE_BAND_EQ_SOURCES
    5BandEQ/Source/PluginEditor.cpp
    5BandEQ/Source/PluginProcessor.cpp
    5BandEQ/Source/DSP/BiquadFilter.cpp
    5BandEQ/Source/DSP/EQBand.cpp
    5BandEQ/Source/GUI/BandControlPanel.cpp
    5BandEQ/Source/GUI/FrequencyResponseDisplay.cpp
)

juce_add_plugin(FiveBandEQ
    COMPANY_NAME "Santiago Pelaez"
    BUNDLE_ID com.santiagopelaez.fivebandeq
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE SpEQ
    PLUGIN_CODE FBEQ
    FORMATS Standalone VST3
    PRODUCT_NAME "5-Band EQ"
    DESCRIPTION "Five-band parametric equalizer"
    VST3_CATEGORIES "Fx" "EQ"
)

target_sources(FiveBandEQ PRIVATE ${FIVE_BAND_EQ_SOURCES})

target_include_directories(FiveBandEQ PRIVATE
    5BandEQ/Source
)

target_compile_definitions(FiveBandEQ PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_link_libraries(FiveBandEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(FiveBandEQ_VST3 PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)

option(FIVE_BAND_EQ_BUILD_TESTS "Build the DSP and parameter tests" ON)

if(FIVE_BAND_EQ_BUILD_TESTS)
    enable_testing()

    add_executable(FiveBandEQ_Tests
        Tests/DspTests.cpp
        5BandEQ/Source/DSP/BiquadFilter.cpp
    )

    add_executable(FiveBandEQ_ParameterTests
        Tests/ParameterStateTests.cpp
    )

    target_include_directories(FiveBandEQ_Tests PRIVATE
        5BandEQ/Source
    )

    target_compile_definitions(FiveBandEQ_Tests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_link_libraries(FiveBandEQ_Tests PRIVATE
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    target_include_directories(FiveBandEQ_ParameterTests PRIVATE
        5BandEQ/Source
    )

    target_compile_definitions(FiveBandEQ_ParameterTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_link_libraries(FiveBandEQ_ParameterTests PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    add_test(NAME FiveBandEQ_DspTests COMMAND FiveBandEQ_Tests)
    add_test(NAME FiveBandEQ_ParameterTests COMMAND FiveBandEQ_ParameterTests)
endif()
