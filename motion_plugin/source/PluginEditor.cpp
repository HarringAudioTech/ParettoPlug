#include "PluginEditor.h"

namespace paretto_motion {
namespace {
namespace ParamId {
constexpr const char* syncEnabled = "syncEnabled";
constexpr const char* mode = "mode";
constexpr const char* lfoRate = "lfoRate";

constexpr const char* motion = "motion";
constexpr const char* chaos = "chaos";
constexpr const char* smoothMs = "smoothMs";

constexpr const char* toGain = "toGain";
constexpr const char* toTone = "toTone";
constexpr const char* toGrit = "toGrit";
constexpr const char* toSpace = "toSpace";

constexpr const char* lfoShape = "lfoShape";
constexpr const char* envSense = "envSense";
constexpr const char* rndRateHz = "rndRateHz";

constexpr const char* mix = "mix";
constexpr const char* outputDb = "outputDb";
}  // namespace ParamId

static void initSlider(juce::Slider& s) {
  s.setSliderStyle(juce::Slider::LinearHorizontal);
  s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
}

static void initLabel(juce::Label& l, const juce::String& text, juce::Component& attachTo) {
  l.setText(text, juce::dontSendNotification);
  l.attachToComponent(&attachTo, true);
  l.setJustificationType(juce::Justification::centredLeft);
}
}  // namespace

ParettoMotionAudioProcessorEditor::ParettoMotionAudioProcessorEditor(ParettoMotionAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p) {
  auto& apvts = processorRef.getAPVTS();

  syncToggle_.setButtonText("Sync");
  addAndMakeVisible(syncToggle_);

  modeChoice_.addItemList(juce::StringArray{"Pulse", "Drift", "Pump", "Wobble", "Scatter"}, 1);
  modeChoice_.setTextWhenNothingSelected("Mode");
  addAndMakeVisible(modeChoice_);

  lfoRateChoice_.addItemList(
      juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/1D", "1/2D", "1/4D", "1/8D", "1/16D",
                        "1/32D", "1/1T", "1/2T", "1/4T", "1/8T", "1/16T", "1/32T"},
      1);
  lfoRateChoice_.setTextWhenNothingSelected("LFO Rate");
  addAndMakeVisible(lfoRateChoice_);

  initSlider(motionSlider_);
  addAndMakeVisible(motionSlider_);
  initLabel(motionLabel_, "Motion", motionSlider_);

  initSlider(chaosSlider_);
  addAndMakeVisible(chaosSlider_);
  initLabel(chaosLabel_, "Chaos", chaosSlider_);

  initSlider(smoothMsSlider_);
  addAndMakeVisible(smoothMsSlider_);
  initLabel(smoothMsLabel_, "Smooth ms", smoothMsSlider_);

  initSlider(toGainSlider_);
  addAndMakeVisible(toGainSlider_);
  initLabel(toGainLabel_, "To Gain", toGainSlider_);

  initSlider(toToneSlider_);
  addAndMakeVisible(toToneSlider_);
  initLabel(toToneLabel_, "To Tone", toToneSlider_);

  initSlider(toGritSlider_);
  addAndMakeVisible(toGritSlider_);
  initLabel(toGritLabel_, "To Grit", toGritSlider_);

  initSlider(toSpaceSlider_);
  addAndMakeVisible(toSpaceSlider_);
  initLabel(toSpaceLabel_, "To Space", toSpaceSlider_);

  initSlider(lfoShapeSlider_);
  addAndMakeVisible(lfoShapeSlider_);
  initLabel(lfoShapeLabel_, "LFO Shape", lfoShapeSlider_);

  initSlider(envSenseSlider_);
  addAndMakeVisible(envSenseSlider_);
  initLabel(envSenseLabel_, "Env Sense", envSenseSlider_);

  initSlider(rndRateSlider_);
  addAndMakeVisible(rndRateSlider_);
  initLabel(rndRateLabel_, "Rnd Hz", rndRateSlider_);

  initSlider(mixSlider_);
  addAndMakeVisible(mixSlider_);
  initLabel(mixLabel_, "Mix", mixSlider_);

  initSlider(outputDbSlider_);
  addAndMakeVisible(outputDbSlider_);
  initLabel(outputDbLabel_, "Output dB", outputDbSlider_);

  syncAttachment_ = std::make_unique<APVTS::ButtonAttachment>(apvts, ParamId::syncEnabled, syncToggle_);
  modeAttachment_ = std::make_unique<APVTS::ComboBoxAttachment>(apvts, ParamId::mode, modeChoice_);
  lfoRateAttachment_ = std::make_unique<APVTS::ComboBoxAttachment>(apvts, ParamId::lfoRate, lfoRateChoice_);

  motionAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::motion, motionSlider_);
  chaosAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::chaos, chaosSlider_);
  smoothMsAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::smoothMs, smoothMsSlider_);

  toGainAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::toGain, toGainSlider_);
  toToneAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::toTone, toToneSlider_);
  toGritAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::toGrit, toGritSlider_);
  toSpaceAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::toSpace, toSpaceSlider_);

  lfoShapeAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::lfoShape, lfoShapeSlider_);
  envSenseAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::envSense, envSenseSlider_);
  rndRateAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::rndRateHz, rndRateSlider_);

  mixAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::mix, mixSlider_);
  outputDbAttachment_ = std::make_unique<APVTS::SliderAttachment>(apvts, ParamId::outputDb, outputDbSlider_);

  setSize(620, 420);
}

ParettoMotionAudioProcessorEditor::~ParettoMotionAudioProcessorEditor() {}

void ParettoMotionAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(15.0f);
  auto header = getLocalBounds().removeFromTop(28);
  g.drawFittedText("ParettoMotion", header, juce::Justification::centred, 1);
}

void ParettoMotionAudioProcessorEditor::resized() {
  auto r = getLocalBounds().reduced(12);
  r.removeFromTop(28);
  r.removeFromTop(8);

  auto topRow = r.removeFromTop(28);
  syncToggle_.setBounds(topRow.removeFromLeft(90));
  topRow.removeFromLeft(10);
  modeChoice_.setBounds(topRow.removeFromLeft(160));
  topRow.removeFromLeft(10);
  lfoRateChoice_.setBounds(topRow.removeFromLeft(200));

  r.removeFromTop(10);

  constexpr int labelW = 90;
  const int rowH = 26;
  const int rowGap = 6;

  auto placeRow = [&](juce::Slider& s) {
    auto row = r.removeFromTop(rowH);
    row.removeFromLeft(labelW);
    s.setBounds(row);
    r.removeFromTop(rowGap);
  };

  placeRow(motionSlider_);
  placeRow(chaosSlider_);
  placeRow(smoothMsSlider_);
  placeRow(toGainSlider_);
  placeRow(toToneSlider_);
  placeRow(toGritSlider_);
  placeRow(toSpaceSlider_);
  placeRow(lfoShapeSlider_);
  placeRow(envSenseSlider_);
  placeRow(rndRateSlider_);
  placeRow(mixSlider_);
  placeRow(outputDbSlider_);
}

}  // namespace paretto_motion
