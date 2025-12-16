#pragma once

#include <memory>

#include "PluginProcessor.h"

namespace audio_plugin {

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
  ~AudioPluginAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  AudioPluginAudioProcessor& processorRef;

  juce::ToggleButton syncToggle_;
  juce::ComboBox rateChoice_;
  juce::ComboBox stepsChoice_;
  juce::ComboBox grooveChoice_;

  juce::Label rateHzLabel_;
  juce::Slider rateHzSlider_;

  juce::Label depthLabel_;
  juce::Slider depthSlider_;

  juce::Label floorDbLabel_;
  juce::Slider floorDbSlider_;

  juce::Label smoothMsLabel_;
  juce::Slider smoothMsSlider_;

  juce::Label mixLabel_;
  juce::Slider mixSlider_;

  juce::Label outputDbLabel_;
  juce::Slider outputDbSlider_;

  using APVTS = juce::AudioProcessorValueTreeState;
  std::unique_ptr<APVTS::ButtonAttachment> syncAttachment_;
  std::unique_ptr<APVTS::ComboBoxAttachment> rateAttachment_;
  std::unique_ptr<APVTS::ComboBoxAttachment> stepsAttachment_;
  std::unique_ptr<APVTS::ComboBoxAttachment> grooveAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> rateHzAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> depthAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> floorDbAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> smoothMsAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> mixAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> outputDbAttachment_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
}  // namespace audio_plugin
