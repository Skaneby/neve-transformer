cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum OS X version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build for both Silicon and Intel")
endif()

project(NeveTransformer VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory
add_subdirectory(JUCE)

# Define our plugin/standalone app
juce_add_gui_app(NeveTransformer
    PRODUCT_NAME "Neve Transformer"
    COMPANY_NAME "HERRSTROM"
    BUNDLE_ID "com.juneskaneby.nevetransformer"
    MICROPHONE_PERMISSION_ENABLED TRUE
    NEEDS_MICROPHONE TRUE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# Source files
target_sources(NeveTransformer
    PRIVATE
        Source/Main.cpp
        Source/UI/MainComponent.cpp
        Source/DSP/NeveTransformerDSP.cpp
        Source/DSP/BiquadFilter.cpp
        Source/DSP/Waveshaper.cpp
        Source/DSP/DynamicAllpass.cpp
        Source/DSP/Oversampler.cpp
)

# Include directories
target_include_directories(NeveTransformer
    PRIVATE
        Source
        Source/DSP
        Source/UI
)

# Link JUCE modules
target_link_libraries(NeveTransformer
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compile definitions
target_compile_definitions(NeveTransformer
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NeveTransformer,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:NeveTransformer,JUCE_VERSION>"
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_COREGRAPHICS_RENDER_WITH_METAL=0
        JUCE_METAL_AVAILABLE=0
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(NeveTransformer PRIVATE -O3)
endif()

# Ad-hoc sign the app bundle after build (required for running locally on ARM and newer macOS)
# We use a custom target to ensure signing runs AFTER all bundle resources are copied
if(APPLE)
    add_custom_target(SignNeveTransformer ALL
        COMMAND codesign --force --deep -s - "$<TARGET_BUNDLE_DIR:NeveTransformer>"
        COMMENT "Ad-hoc signing the application bundle"
    )
    add_dependencies(SignNeveTransformer NeveTransformer)

    # Copy signed app bundle to Google Drive output directory with date-time stamp
    add_custom_target(DeployNeveTransformer ALL
        COMMAND bash "${CMAKE_SOURCE_DIR}/deploy.sh" "$<TARGET_BUNDLE_DIR:NeveTransformer>"
        COMMENT "Deploying timestamped app to Google Drive"
    )
    add_dependencies(DeployNeveTransformer SignNeveTransformer)
endif()
