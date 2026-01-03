#pragma once

#include <memory>

#include "PluginProcessor.h"

namespace paretto_motion {

class ParettoMotionAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit ParettoMotionAudioProcessorEditor(ParettoMotionAudioProcessor&);
  ~ParettoMotionAudioProcessorEditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

private:
  ParettoMotionAudioProcessor& processorRef;

  juce::ToggleButton syncToggle_;
  juce::ComboBox modeChoice_;
  juce::ComboBox lfoRateChoice_;

  juce::Label motionLabel_;
  juce::Slider motionSlider_;

  juce::Label chaosLabel_;
  juce::Slider chaosSlider_;

  juce::Label smoothMsLabel_;
  juce::Slider smoothMsSlider_;

  juce::Label toGainLabel_;
  juce::Slider toGainSlider_;

  juce::Label toToneLabel_;
  juce::Slider toToneSlider_;

  juce::Label toGritLabel_;
  juce::Slider toGritSlider_;

  juce::Label toSpaceLabel_;
  juce::Slider toSpaceSlider_;

  juce::Label lfoShapeLabel_;
  juce::Slider lfoShapeSlider_;

  juce::Label envSenseLabel_;
  juce::Slider envSenseSlider_;

  juce::Label rndRateLabel_;
  juce::Slider rndRateSlider_;

  juce::Label mixLabel_;
  juce::Slider mixSlider_;

  juce::Label outputDbLabel_;
  juce::Slider outputDbSlider_;

  using APVTS = juce::AudioProcessorValueTreeState;
  std::unique_ptr<APVTS::ButtonAttachment> syncAttachment_;
  std::unique_ptr<APVTS::ComboBoxAttachment> modeAttachment_;
  std::unique_ptr<APVTS::ComboBoxAttachment> lfoRateAttachment_;

  std::unique_ptr<APVTS::SliderAttachment> motionAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> chaosAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> smoothMsAttachment_;

  std::unique_ptr<APVTS::SliderAttachment> toGainAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> toToneAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> toGritAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> toSpaceAttachment_;

  std::unique_ptr<APVTS::SliderAttachment> lfoShapeAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> envSenseAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> rndRateAttachment_;

  std::unique_ptr<APVTS::SliderAttachment> mixAttachment_;
  std::unique_ptr<APVTS::SliderAttachment> outputDbAttachment_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParettoMotionAudioProcessorEditor)
};

}  // namespace paretto_motion
