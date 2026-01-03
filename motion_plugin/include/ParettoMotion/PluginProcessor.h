#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <cstdint>

#include "common/DspContext.h"
#include "motion/MotionEngine.h"
#include "motion/MotionParameters.h"

namespace paretto_motion {

class ParettoMotionAudioProcessor : public juce::AudioProcessor {
public:
  ParettoMotionAudioProcessor();
  ~ParettoMotionAudioProcessor() override;

  juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  juce::AudioProcessorValueTreeState apvts_;

  paretto::motion::MotionEngine engine_{};
  paretto::motion::MotionParameters params_{};
  std::uint32_t seed_{0};
  paretto::dsp::ProcessContext ctx_{};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParettoMotionAudioProcessor)
};

}  // namespace paretto_motion
