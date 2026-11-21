cmake_minimum_required(VERSION 3.22)
project(AuroraSynth VERSION 1.0.0)

# Runtime C++ lié statiquement : pas besoin d'installer de redistribuable, DLL plus portable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AuroraSynth
    COMPANY_NAME "Aurora"
    PLUGIN_MANUFACTURER_CODE Aurx
    PLUGIN_CODE Aur1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Aurora Synth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES "Instrument" "Synth"
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(AuroraSynth PRIVATE AuroraSynth.cpp)

target_compile_definitions(AuroraSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AuroraSynth
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags)
