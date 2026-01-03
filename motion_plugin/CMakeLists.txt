cmake_minimum_required(VERSION 3.22)

# Version is needed by JUCE.
project(ParettoMotion VERSION 0.1.0)

# Adding a directory with the library/application name as a subfolder of the
# include folder is a good practice. It helps avoid name clashes later on.
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/ParettoMotion")

juce_add_plugin(
  ${PROJECT_NAME}
  COMPANY_NAME
  "Harring Audio Technology"
  BUNDLE_ID
  "com.harringaudiotechnology.parettomotion"
  IS_SYNTH
  FALSE
  NEEDS_MIDI_INPUT
  FALSE
  NEEDS_MIDI_OUTPUT
  FALSE
  PLUGIN_MANUFACTURER_CODE
  Mcmp
  PLUGIN_CODE
  Motn
  FORMATS
  Standalone
  VST3
  PRODUCT_NAME
  "ParettoMotion"
)

set(SOURCE_FILES source/PluginEditor.cpp source/PluginProcessor.cpp)
set(HEADER_FILES ${INCLUDE_DIR}/PluginEditor.h ${INCLUDE_DIR}/PluginProcessor.h)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

target_link_libraries_system(${PROJECT_NAME} PUBLIC juce::juce_audio_utils)

target_link_libraries(
  ${PROJECT_NAME} PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
                         juce::juce_recommended_warning_flags
                         ParettoCore
)

target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")

# Note: packaging/install rules can be added later (CPack currently targets ParettoGate).

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)
