cmake_minimum_required(VERSION 3.22)
project(JuceAutosampler VERSION 0.0.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(AUTOSAMPLER_BUILD_ID "" CACHE STRING "CI build label shown in the window title. Empty for a local build.")

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(JuceAutosampler
    PRODUCT_NAME "JUCE Autosampler"
    COMPANY_NAME "Restricted Space"
    BUNDLE_ID "pl.restrictor.app"
)

target_sources(JuceAutosampler PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/Sampling/MidiOutputManager.cpp
    Source/Sampling/RecordingManager.cpp
    Source/Sampling/SamplerEngine.cpp
    Source/Sampling/StartDetector.cpp
    Source/Preview/SamplePreviewManager.cpp
)

target_include_directories(JuceAutosampler PRIVATE
    Source
)

target_compile_definitions(JuceAutosampler PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:JuceAutosampler,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:JuceAutosampler,JUCE_VERSION>"
    "AUTOSAMPLER_BUILD_ID=\"${AUTOSAMPLER_BUILD_ID}\""
)

target_link_libraries(JuceAutosampler PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_core
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
