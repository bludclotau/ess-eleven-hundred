juce_add_gui_app(S1100Emu
    PRODUCT_NAME "ESS Eleven Hundred"
    COMPANY_NAME "BludClot"
    VERSION ${PROJECT_VERSION})

target_sources(S1100Emu PRIVATE
    Main.cpp
    MainComponent.cpp
    MainComponent.h
    SampleLoader.cpp
    SampleLoader.h)

target_compile_definitions(S1100Emu PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    S1100_SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples"
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:S1100Emu,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:S1100Emu,JUCE_VERSION>")

target_link_libraries(S1100Emu
    PRIVATE
        s1100_core
        juce::juce_audio_formats
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
