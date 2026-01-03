#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "common/DspContext.h"
#include "common/EnvFollower.h"
#include "common/Lfo.h"
#include "common/SmoothRandom.h"
#include "common/Svf.h"
#include "common/StepSequencer.h"
#include "gate/GateEngine.h"
#include "motion/MotionEngine.h"

namespace {

using Clock = std::chrono::steady_clock;

struct BenchResult {
  double seconds{0.0};
  std::int64_t samples{0};

  double nsPerSample() const {
    if (samples <= 0) {
      return 0.0;
    }
    return (seconds * 1.0e9) / static_cast<double>(samples);
  }

  double megaSamplesPerSec() const {
    if (seconds <= 0.0) {
      return 0.0;
    }
    return (static_cast<double>(samples) / seconds) / 1.0e6;
  }
};

struct Options {
  bool smoke{false};
  std::string writeBaselinePath;
  std::string compareBaselinePath;
  double tolerancePct{10.0};
};

static void useValue(volatile float v) {
  (void)v;
}

static void useValue(volatile std::uint64_t v) {
  (void)v;
}

static Options parseOptions(int argc, char** argv) {
  Options o;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--smoke") {
      o.smoke = true;
      continue;
    }

    if (a == "--write-baseline" && i + 1 < argc) {
      o.writeBaselinePath = argv[++i];
      continue;
    }

    if (a == "--compare-baseline" && i + 1 < argc) {
      o.compareBaselinePath = argv[++i];
      continue;
    }

    if (a == "--tolerance" && i + 1 < argc) {
      o.tolerancePct = std::max(0.0, std::stod(argv[++i]));
      continue;
    }

    if (a == "--help" || a == "-h") {
      std::cout << "ParettoBench\n";
      std::cout << "  --smoke\n";
      std::cout << "  --write-baseline <path>\n";
      std::cout << "  --compare-baseline <path>\n";
      std::cout << "  --tolerance <pct> (default 10)\n";
      std::exit(0);
    }
  }

  return o;
}

static std::map<std::string, double> readBaseline(const std::string& path) {
  std::map<std::string, double> out;
  std::ifstream f(path);
  if (!f.is_open()) {
    return out;
  }

  for (;;) {
    std::string name;
    double ns = 0.0;
    f >> name >> ns;
    if (!f.good()) {
      break;
    }
    out[name] = ns;
  }

  return out;
}

static bool writeBaseline(const std::string& path, const std::map<std::string, BenchResult>& results) {
  std::ofstream f(path, std::ios::trunc);
  if (!f.is_open()) {
    return false;
  }

  for (const auto& kv : results) {
    f << kv.first << " " << std::setprecision(17) << kv.second.nsPerSample() << "\n";
  }

  return true;
}

template <typename Fn>
static BenchResult runBench(const char* name, std::int64_t samples, Fn&& fn) {
  for (int i = 0; i < 2; ++i) {
    fn(samples / 10);
  }

  const auto t0 = Clock::now();
  fn(samples);
  const auto t1 = Clock::now();

  const std::chrono::duration<double> dt = t1 - t0;
  const double sec = dt.count();

  std::cout << std::left << std::setw(18) << name;
  std::cout << "  " << std::right << std::setw(10) << std::fixed << std::setprecision(2) << (sec * 1000.0) << " ms";
  std::cout << "  " << std::setw(10) << std::fixed << std::setprecision(2) << (sec > 0.0 ? (static_cast<double>(samples) / sec) / 1.0e6 : 0.0)
            << " MS/s";
  std::cout << "  " << std::setw(10) << std::fixed << std::setprecision(2) << (sec > 0.0 ? (sec * 1.0e9) / static_cast<double>(samples) : 0.0) << " ns/smp";
  std::cout << "\n";

  return BenchResult{.seconds = sec, .samples = samples};
}

static int compareToBaseline(const std::map<std::string, BenchResult>& results, const std::map<std::string, double>& baseline,
                             double tolerancePct) {
  if (baseline.empty()) {
    std::cerr << "Baseline file missing or empty.\n";
    return 3;
  }

  const double tol = tolerancePct / 100.0;
  bool ok = true;

  for (const auto& kv : results) {
    const auto it = baseline.find(kv.first);
    if (it == baseline.end()) {
      continue;
    }

    const double base = it->second;
    const double now = kv.second.nsPerSample();
    if (base <= 0.0) {
      continue;
    }

    const double ratio = now / base;
    const double pct = (ratio - 1.0) * 100.0;
    if (ratio > (1.0 + tol)) {
      ok = false;
      std::cerr << "Regression: " << kv.first << " " << std::fixed << std::setprecision(2) << pct << "% slower" << " (base " << base
                << " ns, now " << now << " ns)\n";
    }
  }

  return ok ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) {
  const Options opt = parseOptions(argc, argv);

  const double sampleRate = 48000.0;
  const int blockSize = 256;

  const std::int64_t samples = opt.smoke ? (2LL * 1024 * 1024) : (64LL * 1024 * 1024);

  std::cout << "ParettoBench (" << (opt.smoke ? "smoke" : "full") << ")\n";

  std::map<std::string, BenchResult> results;

  results["Lfo"] = runBench("Lfo", samples, [&](std::int64_t n) {
    paretto::dsp::Lfo lfo;
    lfo.prepare(sampleRate);
    lfo.reset(0.0f);
    lfo.setRateHz(2.5f);
    lfo.setShape(paretto::dsp::LfoShape::Sine);

    float acc = 0.0f;
    for (std::int64_t i = 0; i < n; ++i) {
      acc += lfo.processSample();
    }
    useValue(acc);
  });

  results["EnvFollower"] = runBench("EnvFollower", samples, [&](std::int64_t n) {
    paretto::dsp::EnvFollower env;
    env.prepare(sampleRate);
    env.reset();
    env.setAttackReleaseMs(3.0f, 80.0f);

    float acc = 0.0f;
    for (std::int64_t i = 0; i < n; ++i) {
      const float x = std::sin(2.0f * 3.14159265358979323846f * static_cast<float>(i) * (220.0f / static_cast<float>(sampleRate)));
      acc += env.processSample(x);
    }
    useValue(acc);
  });

  results["SmoothRandom"] = runBench("SmoothRandom", samples, [&](std::int64_t n) {
    paretto::dsp::SmoothRandom rnd;
    rnd.prepare(sampleRate);
    rnd.seed(0x12345678u);
    rnd.reset(0.0f);
    rnd.setRateHz(0.6f);
    rnd.setSmoothMs(15.0f);

    float acc = 0.0f;
    for (std::int64_t i = 0; i < n; ++i) {
      acc += rnd.processSample(sampleRate);
    }
    useValue(acc);
  });

  results["SvfLowpass"] = runBench("SvfLowpass", samples, [&](std::int64_t n) {
    paretto::dsp::SvfLowpass svf;
    svf.prepare(sampleRate);
    svf.reset();
    svf.setResonance(0.2f);
    svf.setCutoffHz(1200.0f);

    float acc = 0.0f;
    float x = 0.0f;
    for (std::int64_t i = 0; i < n; ++i) {
      x = 0.9997f * x + 0.0003f;
      acc += svf.processSample(x);
    }
    useValue(acc);
  });

  results["StepSequencer"] = runBench("StepSequencer", samples, [&](std::int64_t n) {
    paretto::dsp::StepSequencer seq;
    seq.prepare(sampleRate);
    seq.setTiming(true, paretto::dsp::SyncRate::R1_16, 2.0f, 16, 0.2f, 0.0f);
    seq.setBpm(128.0);
    seq.reset();

    std::uint64_t acc = 0;
    std::int64_t pos = 0;
    for (std::int64_t i = 0; i < n; ++i) {
      const auto p = seq.getPositionAt(pos);
      acc += static_cast<std::uint64_t>(p.stepIndex);
      ++pos;
    }
    useValue(acc);
  });

  results["GateEngine"] = runBench("GateEngine", samples, [&](std::int64_t n) {
    paretto::gate::GateEngine eng;
    paretto::gate::GateParameters p;

    for (int i = 0; i < paretto::dsp::MaxSteps; ++i) {
      p.patternA.steps[static_cast<std::size_t>(i)].level = (i % 2 == 0) ? 1.0f : 0.0f;
      p.patternB.steps[static_cast<std::size_t>(i)].level = 1.0f;
    }
    p.mix = 1.0f;
    p.depth = 1.0f;
    p.syncEnabled = true;
    p.rate = paretto::dsp::SyncRate::R1_16;
    p.steps = paretto::gate::Steps::S16;

    eng.prepare(sampleRate, 0xCAFEBABEu);
    eng.setParameters(p);

    paretto::dsp::ProcessContext ctx;
    ctx.sampleRate = sampleRate;
    ctx.transport.valid = true;
    ctx.transport.playing = true;
    ctx.transport.bpm = 128.0;
    ctx.transport.ppqPosition = 0.0;
    ctx.transport.timeInSamples = 0;

    std::vector<float> l(static_cast<std::size_t>(blockSize), 0.0f);
    std::vector<float> r(static_cast<std::size_t>(blockSize), 0.0f);
    float* ch[2] = {l.data(), r.data()};

    float acc = 0.0f;
    std::int64_t remaining = n;
    while (remaining > 0) {
      const int ns = static_cast<int>(std::min<std::int64_t>(remaining, blockSize));
      for (int i = 0; i < ns; ++i) {
        const float x = std::sin(2.0f * 3.14159265358979323846f * static_cast<float>(i) * (220.0f / static_cast<float>(sampleRate)));
        l[static_cast<std::size_t>(i)] = x;
        r[static_cast<std::size_t>(i)] = x;
      }

      eng.process(ch, 2, ns, ctx);

      acc += l[0] + r[0];
      ctx.transport.timeInSamples += ns;
      remaining -= ns;
    }

    useValue(acc);
  });

  results["MotionEngine"] = runBench("MotionEngine", samples, [&](std::int64_t n) {
    paretto::motion::MotionEngine eng;
    paretto::motion::MotionParameters p;

    p.motion = 0.6f;
    p.chaos = 0.3f;
    p.mix = 1.0f;

    eng.prepare(sampleRate, blockSize);
    eng.setSeed(0xDEADBEEFu);
    eng.setParameters(p);

    paretto::dsp::ProcessContext ctx;
    ctx.sampleRate = sampleRate;
    ctx.transport.valid = true;
    ctx.transport.playing = true;
    ctx.transport.bpm = 128.0;
    ctx.transport.ppqPosition = 0.0;
    ctx.transport.timeInSamples = 0;

    std::vector<float> l(static_cast<std::size_t>(blockSize), 0.0f);
    std::vector<float> r(static_cast<std::size_t>(blockSize), 0.0f);
    float* ch[2] = {l.data(), r.data()};

    float acc = 0.0f;
    std::int64_t remaining = n;
    while (remaining > 0) {
      const int ns = static_cast<int>(std::min<std::int64_t>(remaining, blockSize));
      for (int i = 0; i < ns; ++i) {
        const float x = std::sin(2.0f * 3.14159265358979323846f * static_cast<float>(i) * (220.0f / static_cast<float>(sampleRate)));
        l[static_cast<std::size_t>(i)] = x;
        r[static_cast<std::size_t>(i)] = x;
      }

      eng.process(ch, 2, ns, ctx);

      acc += l[0] + r[0];
      ctx.transport.timeInSamples += ns;
      remaining -= ns;
    }

    useValue(acc);
  });

  if (!opt.writeBaselinePath.empty()) {
    if (!writeBaseline(opt.writeBaselinePath, results)) {
      std::cerr << "Failed to write baseline: " << opt.writeBaselinePath << "\n";
      return 4;
    }
  }

  if (!opt.compareBaselinePath.empty()) {
    const auto baseline = readBaseline(opt.compareBaselinePath);
    return compareToBaseline(results, baseline, opt.tolerancePct);
  }

  return 0;
}
