#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <atomic>
#include <array>
#include <cstdint>

namespace paretto_motion {
namespace {
std::atomic<std::uint32_t> gInstanceCounter{1u};

constexpr const char* kStateTreeId = "ParettoMotionParams";

namespace ParamId {
constexpr const char* motion = "motion";
constexpr const char* chaos = "chaos";
constexpr const char* smoothMs = "smoothMs";
constexpr const char* mode = "mode";
constexpr const char* mix = "mix";
constexpr const char* outputDb = "outputDb";

constexpr const char* toGain = "toGain";
constexpr const char* toTone = "toTone";
constexpr const char* toGrit = "toGrit";
constexpr const char* toSpace = "toSpace";

constexpr const char* syncEnabled = "syncEnabled";
constexpr const char* lfoRate = "lfoRate";
constexpr const char* lfoShape = "lfoShape";
constexpr const char* envSense = "envSense";
constexpr const char* rndRateHz = "rndRateHz";
}  // namespace ParamId

inline float loadParam(const juce::AudioProcessorValueTreeState& apvts, const char* id) {
  if (auto* v = apvts.getRawParameterValue(id)) {
    return v->load();
  }
  return 0.0f;
}
}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout ParettoMotionAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::motion, 1}, "Motion",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::chaos, 1}, "Chaos",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::smoothMs, 1}, "Smooth ms",
                                                         juce::NormalisableRange<float>(0.0f, 200.0f, 0.0f, 0.5f), 20.0f));

  layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ParamId::mode, 1}, "Mode",
                                                          juce::StringArray{"Pulse", "Drift", "Pump", "Wobble", "Scatter"}, 1));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::mix, 1}, "Mix",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 1.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::outputDb, 1}, "Output dB",
                                                         juce::NormalisableRange<float>(-24.0f, 12.0f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::toGain, 1}, "To Gain",
                                                         juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0f, 1.0f), 0.0f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::toTone, 1}, "To Tone",
                                                         juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0f, 1.0f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::toGrit, 1}, "To Grit",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::toSpace, 1}, "To Space",
                                                         juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0f, 1.0f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ParamId::syncEnabled, 1}, "Sync", true));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      juce::ParameterID{ParamId::lfoRate, 1}, "LFO Rate",
      juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/1D", "1/2D", "1/4D", "1/8D", "1/16D",
                        "1/32D", "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"},
      2));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::lfoShape, 1}, "LFO Shape",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.2f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::envSense, 1}, "Env Sense",
                                                         juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.5f));

  layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{ParamId::rndRateHz, 1}, "Rnd Rate Hz",
                                                         juce::NormalisableRange<float>(0.05f, 10.0f, 0.0f, 0.5f), 0.3f));

  return layout;
}

ParettoMotionAudioProcessor::ParettoMotionAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         )
    , apvts_(*this, nullptr, kStateTreeId, createParameterLayout()) {
}

ParettoMotionAudioProcessor::~ParettoMotionAudioProcessor() {}

const juce::String ParettoMotionAudioProcessor::getName() const { return JucePlugin_Name; }

bool ParettoMotionAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool ParettoMotionAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool ParettoMotionAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double ParettoMotionAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int ParettoMotionAudioProcessor::getNumPrograms() { return 1; }

int ParettoMotionAudioProcessor::getCurrentProgram() { return 0; }

void ParettoMotionAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }

const juce::String ParettoMotionAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void ParettoMotionAudioProcessor::changeProgramName(int index, const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void ParettoMotionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);

  ctx_.sampleRate = sampleRate;

  const auto counter = gInstanceCounter.fetch_add(1u, std::memory_order_relaxed);
  const auto ptrHash = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(this));
  seed_ = static_cast<std::uint32_t>(static_cast<std::uint32_t>(sampleRate) ^ ptrHash ^ counter);

  engine_.prepare(sampleRate, samplesPerBlock);
  engine_.setSeed(seed_);
  engine_.reset();
}

void ParettoMotionAudioProcessor::releaseResources() {}

bool ParettoMotionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
    return false;
  }

#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) {
    return false;
  }
#endif

  return true;
#endif
}

void ParettoMotionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  ctx_.transport.valid = false;
  if (auto* ph = getPlayHead()) {
    if (auto posInfo = ph->getPosition()) {
      ctx_.transport.valid = true;
      ctx_.transport.playing = posInfo->getIsPlaying();
      ctx_.transport.bpm = posInfo->getBpm().orFallback(120.0);
      ctx_.transport.ppqPosition = posInfo->getPpqPosition().orFallback(0.0);
      ctx_.transport.timeInSamples = posInfo->getTimeInSamples().orFallback(0);
    }
  }

  float* channelPtrs[2] = {nullptr, nullptr};
  const int numCh = std::min(2, totalNumInputChannels);
  for (int ch = 0; ch < numCh; ++ch) {
    channelPtrs[ch] = buffer.getWritePointer(ch);
  }

  params_.motion = loadParam(apvts_, ParamId::motion);
  params_.chaos = loadParam(apvts_, ParamId::chaos);
  params_.smoothMs = loadParam(apvts_, ParamId::smoothMs);

  {
    const int idx = static_cast<int>(loadParam(apvts_, ParamId::mode));
    const int safe = std::max(0, std::min(4, idx));
    params_.mode = static_cast<paretto::motion::MotionMode>(safe);
  }

  params_.mix = loadParam(apvts_, ParamId::mix);
  params_.outputDb = loadParam(apvts_, ParamId::outputDb);

  params_.toGain = loadParam(apvts_, ParamId::toGain);
  params_.toTone = loadParam(apvts_, ParamId::toTone);
  params_.toGrit = loadParam(apvts_, ParamId::toGrit);
  params_.toSpace = loadParam(apvts_, ParamId::toSpace);

  params_.syncEnabled = loadParam(apvts_, ParamId::syncEnabled) >= 0.5f;

  {
    constexpr std::array<paretto::dsp::SyncRate, 18> kRates = {
        paretto::dsp::SyncRate::R1_1,
        paretto::dsp::SyncRate::R1_2,
        paretto::dsp::SyncRate::R1_4,
        paretto::dsp::SyncRate::R1_8,
        paretto::dsp::SyncRate::R1_16,
        paretto::dsp::SyncRate::R1_32,
        paretto::dsp::SyncRate::R1_1_Dotted,
        paretto::dsp::SyncRate::R1_2_Dotted,
        paretto::dsp::SyncRate::R1_4_Dotted,
        paretto::dsp::SyncRate::R1_8_Dotted,
        paretto::dsp::SyncRate::R1_16_Dotted,
        paretto::dsp::SyncRate::R1_32_Dotted,
        paretto::dsp::SyncRate::R1_1_Triplet,
        paretto::dsp::SyncRate::R1_2_Triplet,
        paretto::dsp::SyncRate::R1_4_Triplet,
        paretto::dsp::SyncRate::R1_8_Triplet,
        paretto::dsp::SyncRate::R1_16_Triplet,
        paretto::dsp::SyncRate::R1_32_Triplet,
    };

    const int idx = static_cast<int>(loadParam(apvts_, ParamId::lfoRate));
    const int safe = std::max(0, std::min(static_cast<int>(kRates.size()) - 1, idx));
    params_.lfoRate = kRates[static_cast<std::size_t>(safe)];
  }

  params_.lfoShape = loadParam(apvts_, ParamId::lfoShape);
  params_.envSense = loadParam(apvts_, ParamId::envSense);
  params_.rndRateHz = loadParam(apvts_, ParamId::rndRateHz);

  engine_.setParameters(params_);
  engine_.process(channelPtrs, numCh, buffer.getNumSamples(), ctx_);
}

bool ParettoMotionAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ParettoMotionAudioProcessor::createEditor() {
  return new ParettoMotionAudioProcessorEditor(*this);
}

void ParettoMotionAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto state = apvts_.copyState();
  if (auto xml = state.createXml()) {
    copyXmlToBinary(*xml, destData);
  }
}

void ParettoMotionAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
    apvts_.replaceState(juce::ValueTree::fromXml(*xml));
  }
}

}  // namespace paretto_motion

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new paretto_motion::ParettoMotionAudioProcessor();
}
