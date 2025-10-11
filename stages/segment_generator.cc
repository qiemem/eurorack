// Copyright 2017 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Multi-stage envelope

#include "stages/segment_generator.h"

#include "stages/oscillator.h"
#include "stages/quantizer_scales.h"
#include "stmlib/dsp/dsp.h"
#include "stmlib/dsp/parameter_interpolator.h"
#include "stmlib/dsp/units.h"
#include "stmlib/utils/random.h"

#include <cassert>
#include <cmath>
#include <algorithm>
// #include <cstdio>

#include "stages/resources.h"
#include "stmlib/stmlib.h"
#include "stmlib/utils/gate_flags.h"
#include "stmlib/utils/random.h"
#include "stages/envelope_utils.h"

namespace stages {

using namespace stmlib;
using namespace std;
using namespace segment;

// Duration of the "tooth" in the output when a trigger is received while the
// output is high.
const int kRetrigDelaySamples = 32;

// S&H delay (for all those sequencers whose CV and GATE outputs are out of
// sync).
const size_t kSampleAndHoldDelay = kSampleRate * 2 / 1000;

// Clock inhibition following a rising edge on the RESET input
const size_t kClockInhibitDelay = kSampleRate * 5 / 1000;

void SegmentGenerator::Init(MultiMode multimode, stmlib::HysteresisQuantizer2* step_quantizer) {
  process_fn_ = &SegmentGenerator::ProcessMultiSegment;

  multimode_ = multimode;

  phase_ = 0.0f;
  aux_ = 0.0f;

  zero_ = 0.0f;
  half_ = 0.5f;
  one_ = 1.0f;

  start_ = 0.0f;
  value_ = 0.0f;
  next_ = Random::GetFloat();
  lp_ = 0.0f;

  monitored_segment_ = 0;
  active_segment_ = 0;
  previous_segment_ = 0;
  retrig_delay_ = 0;
  primary_ = 0;

  Segment s;
  s.start = &zero_;
  s.end = &zero_;
  s.time = &zero_;
  s.curve = &half_;
  s.portamento = &zero_;
  s.phase = NULL;
  s.if_rising = 0;
  s.if_falling = 0;
  s.if_complete = 0;
  s.bipolar = false;
  s.retrig = true;
  s.range = RANGE_DEFAULT;
  s.quant_scale = 0;
  s.shift_register = Random::GetSample();
  s.register_value = Random::GetFloat();
  fill(&segments_[0], &segments_[kMaxNumSegments + 1], s);

  Parameters p;
  p.primary = 0.0f;
  p.secondary = 0.0f;
  fill(&parameters_[0], &parameters_[kMaxNumSegments], p);

  ramp_extractor_.Init(
      kSampleRate,
      1000.0f / kSampleRate);

  delay_line_.Init();
  gate_delay_.Init();

  function_quantizer_.Init(2, 0.025f, false);
  address_quantizer_.Init(2, 0.025f, false);

  num_segments_ = 0;

  first_step_ = 1;
  last_step_ = 1;

  x_ = Random::GetFloat();
  y_ = Random::GetFloat();
  z_ = Random::GetFloat();

  quantized_output_ = false;
  up_down_counter_ = inhibit_clock_ = 0;
  reset_ = false;
  accepted_gate_ = true;
  step_quantizer_ = step_quantizer;

  audio_osc_.Init();
}

inline float SegmentGenerator::PortamentoRateToLPCoefficient(float rate) const {
  int32_t i = static_cast<int32_t>(rate * 512.0f);
  return lut_portamento_coefficient[i];
}

static size_t tm_steps(const float param) {
  size_t steps = static_cast<size_t>(16 * param + 1);
  CONSTRAIN(steps, 1, 16);
  return steps;
}

static float tm_prob(const float param) {
  // Ensures registers lock at extremes
  return 1.02f * param - 0.01f;
}

static void advance_tm(
    size_t steps,
    float prob,
    uint16_t& shift_register,
    float& register_value,
    bool bipolar) {
  uint16_t sr = shift_register;
  uint16_t copied_bit = (sr << (steps - 1)) & (1 << 15);
  uint16_t mutated = copied_bit ^ ((Random::GetFloat() < prob) << 15);
  sr = (sr >> 1) | mutated;
  shift_register = sr;
  register_value = (float)(shift_register) / 65535.0f;
  if (bipolar) {
    register_value = (10.0f / 8.0f) * (register_value - 0.5f);
  }
}

// Seems popular enough :)
#define TRACK_PREVIOUS_SEGMENT

void SegmentGenerator::ProcessMultiSegment(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  float phase = phase_;
  float start = start_;
  float lp = lp_;
  float value = value_;

  while (size--) {
    const Segment& segment = segments_[active_segment_];

#ifdef TRACK_PREVIOUS_SEGMENT
    const Segment& previous = segments_[previous_segment_];
    // Having phase means segment is trackable
    // If previous.start == previous.end and segment.end = previous.start we
    // can end up with start and end tracking the same value, which would do
    // nothing.
    if (!segment.start && previous.phase && segment.end != previous.end) {
      // Just setting start to the previous segment's end would cause a jump
      // when, e.g., going from a slewed step to a ramp before the step
      // finishes. In the case where the current segment does not have a start
      // it's set to the last value of the previous segment. Thus, slewing
      // between that and the end tracks what that segment would have done.
      ONE_POLE(
          start,
          *previous.end,
          PortamentoRateToLPCoefficient(*previous.portamento));
    }
#endif  // TRACK_PREVIOUS_SEGMENT

    if (segment.time) {
      phase += RateToFrequency(*segment.time);
    }

    bool complete = phase >= 1.0f;
    if (complete) {
      phase = 1.0f;
    }
    value = Crossfade(
        start,
        *segment.end,
        WarpPhase(segment.phase ? *segment.phase : phase, *segment.curve));

    ONE_POLE(lp, value, PortamentoRateToLPCoefficient(*segment.portamento));

    // Decide what to do next.
    int go_to_segment = -1;
    // It would probably be better to do retrig with go_to_segments, but that
    // makes single decay segments harder.
    if ((*gate_flags & GATE_FLAG_RISING) && segment.retrig) {
      go_to_segment = segment.if_rising;
    } else if (*gate_flags & GATE_FLAG_FALLING) {
      go_to_segment = segment.if_falling;
    } else if (complete) {
      go_to_segment = segment.if_complete;
    }

    if (go_to_segment != -1) {
      if (previous.advance_tm) {
        const float steps_param = parameters_[previous_segment_].secondary;
        const float prob_param = parameters_[previous_segment_].primary;
        advance_tm(
            tm_steps(steps_param), tm_prob(prob_param),
            (&segments_[previous_segment_])->shift_register,
            (&segments_[previous_segment_])->register_value,
            previous.bipolar);
      }
      phase = 0.0f;
      const Segment& destination = segments_[go_to_segment];
      start = destination.start
          ? *destination.start
          : (go_to_segment == active_segment_ ? start : value);
      if (go_to_segment != active_segment_) {
        previous_segment_ = active_segment_;
      }
      active_segment_ = go_to_segment;
    }

    out->value = lp;
    out->phase = phase;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
  phase_ = phase;
  start_ = start;
  lp_ = lp;
  value_ = value;
}

void SegmentGenerator::ProcessDecayEnvelope(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float frequency = RateToFrequency(parameters_[0].primary);
  while (size--) {
    if ((*gate_flags & GATE_FLAG_RISING) && (active_segment_ != 0 || segments_[0].retrig)) {
      phase_ = 0.0f;
      active_segment_ = 0;
    }

    phase_ += frequency;
    if (phase_ >= 1.0f) {
      phase_ = 1.0f;
      active_segment_ = 1;
    }
    lp_ = value_ = 1.0f - WarpPhase(phase_, parameters_[0].secondary);
    out->value = lp_;
    out->phase = phase_;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ProcessRiseAndFall(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  float fall = PortamentoRateToLPCoefficient(local_parameters_[0].slider);
  float rise = PortamentoRateToLPCoefficient(parameters_[0].secondary);
  ParameterInterpolator primary(&primary_, local_parameters_[0].cv, size);
  switch (segments_[0].range) {
    // enum is frequency ranges, so flipped for time...
    case RANGE_SLOW:
      // This is as high as we can go without exceeding 1. Makes it about 3
      // times shorter
      fall *= 1.0f / lut_portamento_coefficient[0];
      rise *= 1.0f / lut_portamento_coefficient[0];
      break;
    case RANGE_FAST:
      // 16 times longer
      fall *= 0.0625f;
      rise *= 0.0625f;
      break;
    default:
      // fine where it is
      break;
  }

  while (size--) {
    value_ = segments_[0].bipolar ? primary.Next() : fabsf(primary.Next());
    if (value_ > lp_) {
      ONE_POLE(lp_, value_, rise);
      phase_ = 0;
    } else {
      ONE_POLE(lp_, value_, fall);
      phase_ = 1;
    }
    out->value = lp_;
    out->phase = phase_;
    out->segment = active_segment_ = fabsf(lp_) > 0.1f ? 0 : 1;
    out++;
  }
}

void SegmentGenerator::ProcessTimedPulseGenerator(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float frequency = RateToFrequency(parameters_[0].secondary);

  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);
  while (size--) {
    if ((*gate_flags & GATE_FLAG_RISING) && (active_segment_ != 0 || segments_[0].retrig)) {
      retrig_delay_ = active_segment_ == 0 ? kRetrigDelaySamples : 0;
      phase_ = 0.0f;
      active_segment_ = 0;
    }
    if (retrig_delay_) {
      --retrig_delay_;
    }
    phase_ += frequency;
    if (phase_ >= 1.0f) {
      phase_ = 1.0f;
      active_segment_ = 1;
    }

    const float p = primary.Next();
    lp_ = value_ = active_segment_ == 0 && !retrig_delay_ ? p : 0.0f;
    out->value = lp_;
    out->phase = phase_;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ProcessGateGenerator(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);
  while (size--) {
    if (*gate_flags & GATE_FLAG_RISING) {
      accepted_gate_ = Random::GetFloat() < parameters_[0].secondary * 1.01f;
    }
    active_segment_ = (*gate_flags & GATE_FLAG_HIGH) && accepted_gate_ ? 0 : 1;

    const float p = primary.Next();
    lp_ = value_ = active_segment_ == 0 ? p : 0.0f;
    out->value = lp_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ProcessProbabilisticGateGenerator(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);
  const float prob = 1.02f * parameters_[0].secondary - 0.01f;
  while (size--) {
    if (*gate_flags & GATE_FLAG_RISING) {
      active_segment_ = Random::GetFloat() < prob ? 0 : 1;
    }
    active_segment_ = (*gate_flags & GATE_FLAG_HIGH) && (active_segment_ == 0) ? 0 : 1;

    const float p = primary.Next();
    lp_ = value_ = active_segment_ == 0 ? p : 0.0f;
    out->value = lp_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ProcessSampleAndHold(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float coefficient = PortamentoRateToLPCoefficient(
      parameters_[0].secondary);

  // If quantizing, interpolation can cause holding the wrong value.
  if (segments_[0].quant_scale > 0) primary_ = parameters_[0].primary;
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);

  while (size--) {
    const float p = primary.Next();
    gate_delay_.Write(*gate_flags);
    if (gate_delay_.Read(kSampleAndHoldDelay) & GATE_FLAG_RISING) {
      value_ = p;
    }
    active_segment_ = *gate_flags & GATE_FLAG_HIGH ? 0 : 1;

    ONE_POLE(lp_, value_, coefficient);
    out->value = lp_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ProcessAttSampleAndHold(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {

  // If quantizing, interpolation can cause holding the wrong value.
  if (segments_[0].quant_scale > 0) primary_ = parameters_[0].primary;
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);

  while (size--) {
    const float p = primary.Next();
    gate_delay_.Write(*gate_flags);
    if (gate_delay_.Read(kSampleAndHoldDelay) & GATE_FLAG_RISING) {
      value_ = p;
    }
    active_segment_ = *gate_flags & GATE_FLAG_HIGH ? 0 : 1;

    out->value = lp_ = value_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}


void SegmentGenerator::ProcessTrackAndHold(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float coefficient = PortamentoRateToLPCoefficient(
      parameters_[0].secondary);
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);

  while (size--) {
    const float p = primary.Next();
    gate_delay_.Write(*gate_flags);
    if (gate_delay_.Read(kSampleAndHoldDelay) & GATE_FLAG_HIGH) {
      value_ = p;
    }
    active_segment_ = *gate_flags & GATE_FLAG_HIGH ? 0 : 1;

    ONE_POLE(lp_, value_, coefficient);
    out->value = lp_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ProcessClockedSampleAndHold(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float frequency = RateToFrequency(parameters_[0].secondary);
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);
  while (size--) {
    phase_ += frequency;
    if (phase_ >= 1.0f) {
      phase_ -= 1.0f;

      const float reset_time = phase_ / frequency;
      value_ = primary.subsample(1.0f - reset_time);
    }
    primary.Next();
    active_segment_ = phase_ < 0.5f ? 0 : 1;
    out->value = value_;
    out->phase = phase_;
    out->segment = active_segment_;
    ++out;
  }
}

inline tides::Ratio calc_ratio(int n, int d) {
  // I honestly don't know why the - 1e-06f is here. I just noticed that all
  // the original ratios were that much lower than expected.
  return (tides::Ratio) { float(n) / float(d) - 1e-06f, d };
}

const tides::Ratio divider_ratios[] = {
  calc_ratio(1, 32), // slow start: 0
  calc_ratio(1, 16),
  calc_ratio(1, 8),
  calc_ratio(1, 7),
  calc_ratio(1, 6),
  calc_ratio(1, 5),
  calc_ratio(1, 4), // default start: 6
  calc_ratio(1, 3),
  calc_ratio(1, 2),
  calc_ratio(1, 1), // fast start: 9
  calc_ratio(2, 1),
  calc_ratio(3, 1),
  calc_ratio(4, 1),
  calc_ratio(5, 1),
  calc_ratio(6, 1),
  calc_ratio(7, 1),
  calc_ratio(8, 1),
  calc_ratio(12, 1),
  calc_ratio(16, 1),
};

const uint8_t divider_ratios_start[] = {6, 0, 9, 6};
const uint8_t num_divider_ratios[] = {7, 10, 10, 7};

void SegmentGenerator::ProcessTapLFO(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  ProcessOscillator(gate_flags, out, size);
}

void SegmentGenerator::ProcessFreeRunningLFO(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  ProcessOscillator(NULL, out, size);
}

inline float Log2Fast(float x) {
  union {
    float f;
    int32_t w;
  } r;
  r.f = x;
  float log2f = (float)(((r.w >> 23) & 255) - 128);
  r.w  &= ~(255 << 23);
  r.w  += 127 << 23;
  log2f += ((-0.34484843f) * r.f + 2.02466578f) * r.f - 0.67487759f;
  return log2f;
}

const float default_root_note = 2.0439497f;
const float root_notes[] = {default_root_note, default_root_note / 16.0f,
                            default_root_note * 64.0f,
                            default_root_note * 128.0f};
const float audio_rate_threshold = 16.0f * default_root_note / kSampleRate;

void SegmentGenerator::ProcessOscillator(
    const GateFlags* gate_flags,
    SegmentGenerator::Output* out,
    size_t size) {

  float frequency = 0.0f;
  bool freq_is_ar = false;
  const float root_note = root_notes[segments_[0].range];
  float ramp[size];
  // true: Use triangle -> saw -> square -> pwm pulse fully bandlimited osc
  // false: Use saw -> triangle -> sine -> triangle -> square osc but still
  // partially bandlimit if frequency is high enough.
  FreqRange range = segments_[0].range;
  bool audio_rate = range == segment::RANGE_AUDIO;
  bool pll = audio_rate || smooth_audio_rate_tracking_;


  tides::Ratio r = { 1.0f, 1 };
  if (gate_flags && !reset_on_gate_) {
    r = function_quantizer_.Lookup(divider_ratios + divider_ratios_start[range],
                                   parameters_[0].primary * 1.03f);
    frequency = ramp_extractor_.Process(pll, false, r, gate_flags, ramp, size);
    // Check if the phase is actually changing. If its not, then frequency will
    // be positive even though we're missing expected gates. Without this, the
    // segment can flip to audio rate when the user unplugs a patch cable until
    // the module figures out the cable is gone.
    if (ramp[size - 2] == ramp[size - 1])
      frequency = 0.0f;

    freq_is_ar = frequency > audio_rate_threshold;
    if (smooth_audio_rate_tracking_ != freq_is_ar) {
      pll_counter_ -= size;
      if (pll_counter_ <= 0) {
        smooth_audio_rate_tracking_ = freq_is_ar;
      }
    } else {
      ResetPllCounter();
    }
  } else {
    float f = 96.0f * (parameters_[0].primary - 0.5f);
    CONSTRAIN(f, -128.0f, 127.0f);
    frequency = SemitonesToRatio(f) * root_note / kSampleRate;
    if (!audio_rate && multimode_ == MULTI_MODE_STAGES_SLOW_LFO) {
      frequency /= 8.0f;
    }
    freq_is_ar = frequency > audio_rate_threshold;
  }

  if (audio_rate) {
    audio_osc_.Render(frequency, parameters_[0].secondary, ramp, size);

    // This is really cool, but induces a pretty big performance hit.
    // // Blinking rate follows the distance to the nearest C.
    // float distance_to_c = frequency <= 0.0f
    //     ? 0.5f
    //     : Log2Fast(frequency / r.ratio * kSampleRate / root_note);

    // // Wrap to [-0.5, 0.5]
    // MAKE_INTEGRAL_FRACTIONAL(distance_to_c);
    // if (distance_to_c_fractional < -0.5f) {
    //   distance_to_c_fractional += 1.0f;
    // } else if (distance_to_c_fractional > 0.5f) {
    //   distance_to_c_fractional -= 1.0f;
    // }
    // float d = std::min(2.0f * fabsf(distance_to_c_fractional), 1.0f);

    // // Blink f between 0.125 Hz and 16 Hz depending on distance to C.
    // const float blink_frequency =
    //     float(size) * (16.0f * d * (2.0f - d) + 0.125f) / kSampleRate;
    // phase_ += blink_frequency;
    // if (phase_ >= 1.0f) {
    //   phase_ -= 1.0f;
    // }
    for (size_t i = 0; i < size; ++i) {
      out[i].phase = ramp[i] * 2.0f - 1.0f;
      out[i].value = ramp[i] * 5.0f / 8.0f;
      out[i].segment = phase_ < 0.5f ? 0 : 1;
    }
  } else {
    if (!gate_flags) {
      for (size_t i = 0; i < size; ++i) {
        phase_ += frequency;
        if (phase_ >= 1.0f) {
          phase_ -= 1.0f;
        }
        ramp[i] = phase_;
      }
    } else if (reset_on_gate_) {
      for (size_t i = 0; i < size; ++i) {
        if (*gate_flags & GATE_FLAG_RISING) {
          phase_ = 0.0f;
        } else {
          phase_ += frequency;
          if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
          }
        }
        ramp[i] = phase_;
        ++gate_flags;
      }
    }
    if (freq_is_ar)
      ShapeSplineLFO<true>(parameters_[0].secondary, frequency, ramp, out, size,
                           segments_[0].bipolar);
    else
      ShapeSplineLFO<false>(parameters_[0].secondary, frequency, ramp, out, size,
                           segments_[0].bipolar);
  }
  active_segment_ = out[size - 1].segment;
}

void SegmentGenerator::ProcessDelay(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float max_delay = static_cast<float>(kMaxDelay - 1);

  float delay_time = SemitonesToRatio(
      2.0f * (parameters_[0].secondary - 0.5f) * 36.0f) * 0.5f * kSampleRate;
  float clock_frequency = 1.0f;
  float delay_frequency = 1.0f / delay_time;

  if (delay_time >= max_delay) {
    clock_frequency = max_delay * delay_frequency;
    delay_time = max_delay;
  }
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);

  active_segment_ = 0;
  while (size--) {
    phase_ += clock_frequency;
    ONE_POLE(lp_, primary.Next(), clock_frequency);
    if (phase_ >= 1.0f) {
      phase_ -= 1.0f;
      delay_line_.Write(lp_);
    }

    aux_ += delay_frequency;
    if (aux_ >= 1.0f) {
      aux_ -= 1.0f;
    }
    active_segment_ = aux_ < 0.5f ? 0 : 1;

    ONE_POLE(
        value_,
        delay_line_.Read(delay_time - phase_),
        clock_frequency);
    out->value = value_;
    out->phase = aux_;
    out->segment = active_segment_;
    ++out;
  }
}

void SegmentGenerator::ProcessAttOff(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);
  active_segment_ = 0;
  while (size--) {
    out->value = lp_ = value_ = primary.Next();
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++out;
  }
}

void SegmentGenerator::ProcessPortamento(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float coefficient = PortamentoRateToLPCoefficient(
      parameters_[0].secondary);
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);

  active_segment_ = 0;
  while (size--) {
    value_ = primary.Next();
    ONE_POLE(lp_, value_, coefficient);
    out->value = lp_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++out;
  }
}

// y1 - value at t = 0
// k1 - slope at t = 0
// y2 - value at t = 1
// k2 - slope at t = 1
float spline(float y1, float k1, float y2, float k2, float t) {
  float r = 1.0f - t;
  float d = y2 - y1;
  return r * y1 + t * y2 + t * r * (r * (k1 - d) + t * (d - k2));
}

// Is it normal? Almost! This gives a shockingly good approximation of a normal
// distribution, doesn't use any exps, logs, cos, etc., and doesn't
// occasionally take a bunch of iterations. Output is 16bit.
float almost_normal() {
  // 37837.21 and 3.4641032 are 65536 and 2 divided by 0.57735 respectively
  // 0.57735 is the std of the sum of the four uniform distributions, giving us
  // a std of 1. This isn't really necessary, but is kinda convenient for
  // thinking about range.
  uint32_t a = Random::GetWord();
  uint32_t b = Random::GetWord();
  return static_cast<float>(
    (a >> 16) + (a & 0xffff) + (b >> 16) + (b & 0xffff)
    ) / 37837.21f - 3.4641032f;
}

// Brownian modulo accuracy of almost_normal. Also constrained to [0, 1].
// Constraint is imposed by "bouncing" off the edges.
inline float almost_brownian(float last, float std_dev, float min, float max) {
  float width = max - min;
  last += width * std_dev * almost_normal();
  if (last > max) {
    last = 2 * max - last;
  }
  if (last < min) {
    last = 2 * min - last;
  }
  CONSTRAIN(last, min, max);
  return last;
}

void SegmentGenerator::ProcessFreeRunningRandomLFO(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  float f = 96.0f * (parameters_[0].primary - 0.5f);
  CONSTRAIN(f, -128.0f, 127.0f);

  const float root_note = root_notes[segments_[0].range];
  float frequency = SemitonesToRatio(f) * root_note / kSampleRate;
  if (segments_[0].range == RANGE_FAST) {
    // This is so we can more smoothly transition into full noise
    frequency *= 4.0f;
  }

  active_segment_ = 0;

  if (multimode_ == MULTI_MODE_STAGES_SLOW_LFO) {
    frequency /= 8.0f;
  }

  if (frequency > 0.25f) {
    phase_ = 0.0f;
    float std_dev = 2.0f * (1.0f - parameters_[0].secondary);
    std_dev = 0.5f * std_dev * std_dev + 0.01f;
    float min = segments_[0].bipolar ? -5.0f / 8.0f : 0.0f;
    float max = segments_[0].bipolar ? 5.0f / 8.0f : 1.0f;
    if (parameters_[0].secondary < 0.5f) {
      while (size--) {
        out->value = value_ = next_;
        out->segment = 0;
        next_ = Random::GetFloat() * (max - min) + min;
        ++out;
      }
    } else {
      while (size--) {
        out->value = value_ = next_;
        out->segment = 0;
        next_ = almost_brownian(next_, std_dev, min, max);
        ++out;
      }
    }
  } else {
    // phase_ gets updated in ProcessRandomFromPhase
    float phase = phase_;
    if (reset_on_gate_) {
      for (size_t i = 0; i < size; ++i) {
        if (*gate_flags & GATE_FLAG_RISING) {
          phase = 0.0f;
        } else {
          phase += frequency;
          if (phase >= 1.0f) {
            phase -= 1.0f;
          }
        }
        out[i].phase = phase;
        ++gate_flags;
      }
    } else {
      for (size_t i = 0; i < size; ++i) {
        phase += frequency;
        if (phase >= 1.0f) {
          phase -= 1.0f;
        }
        out[i].phase = phase;
      }
    }
    ProcessRandomFromPhase(parameters_[0].secondary, out, size);
  }
}

void SegmentGenerator::ProcessTapRandomLFO(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  float ramp[12];
  uint8_t range = segments_[0].range;

  if (reset_on_gate_) {
    ProcessFreeRunningRandomLFO(gate_flags, out, size);
  } else {
    tides::Ratio r =
        function_quantizer_.Lookup(divider_ratios + divider_ratios_start[range],
                                   parameters_[0].primary * 1.03f);

    ramp_extractor_.Process( false, false, r, gate_flags, ramp, size);
    for (size_t i = 0; i < size; ++i) {
      out[i].phase = ramp[i];
    }
    ProcessRandomFromPhase(parameters_[0].secondary, out, size);
  }
}


void SegmentGenerator::ProcessRandomFromPhase(
    float smoothness,
    SegmentGenerator::Output* in_out,
    size_t size) {

  float k = (smoothness - 0.25f) / 0.25f;
  CONSTRAIN(k, 0.0f, 1.0f);
  float phase_mult = 1.0f;
  if (smoothness < 0.25f) {
    if (smoothness <= 0.001f) {
      phase_mult = float(kSampleRate);
    }
    phase_mult = 0.25f / smoothness;
  }

  while (size--) {
    float phase = in_out->phase;
    if (phase < phase_) {
      start_ = value_;
      value_ = next_;
      if (smoothness <= 0.5f) {
        next_ = Random::GetFloat();
        if (segments_[0].bipolar) {
          next_ = 10.0f / 8.0f * (next_ - 0.5f);
        }
      } else {
        float std_dev = 2.0f * (1.0f - smoothness);
        std_dev = 0.5f * std_dev * std_dev + 0.01f;
        next_ = segments_[0].bipolar
          ? almost_brownian(next_, std_dev, -5.0f / 8.0f, 5.0f / 8.0f)
          : almost_brownian(next_, std_dev, 0.0f, 1.0f);
      }
    }

    float p = phase * phase_mult;
    if (p >= 1.0f) {
      lp_ = value_;
    } else {
      float k1 = value_ - start_;
      float k2 = next_ - value_;
      lp_ = spline(start_, k * k1, value_, k * k2, p);
    }
    in_out->value = lp_;
    phase_ = phase;
    in_out->segment = active_segment_ = phase_ < 0.5f ? 0 : 1;;
    ++in_out;
  }
}


inline float tcsa(float  v, const float w, const float b) {
  v *= 0.159155f; // Convert radians to phase.
  // need to calc wrap here since InterpolateWrap can't handle negatives
  // Using floorf is too slow... The ternary is apparently faster
  v -= static_cast<float>(static_cast<int32_t>(v));
  v = v < 0.0f ? 1.0f - v : v;
  return Interpolate(lut_sine, v, 1024.0f) - b * w;
}


void SegmentGenerator::ProcessThomasSymmetricAttractor(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  float f = 96.0f * (parameters_[0].primary - 0.5f);
  CONSTRAIN(f, -128.0f, 127.0f);

  active_segment_ = 0;
  float frequency = SemitonesToRatio(f) * 2.0439497f / kSampleRate;
  switch (segments_[active_segment_].range) {
    case segment::RANGE_SLOW:
      frequency /= 16.0f;
      break;
    case segment::RANGE_FAST:
      frequency *= 64.0f;
      break;
    default:
      // It's good where it is
      break;
  }

  CONSTRAIN(frequency, 0.0f, kMaxFrequency);
  // Gives a similar feel to the LFO speeds here
  frequency *= 32.0f;

  const float max_b = 0.200f;
  const float min_b = 0.001f;
  float b = ((max_b - min_b) * parameters_[0].secondary + min_b);
  CONSTRAIN(b, min_b, max_b);
  const bool bipolar = segments_[0].bipolar;

  const float offset = bipolar ? 0.0f : 1.0f;
  const float amp = bipolar ? 10.0f / 16.0f : 0.5f;
  float x = x_;
  float y = y_;
  float z = z_;
  while (size--) {
    // Runge-Kutta version: too slow unfortunately
    /*
    const float dx1 = tcsa(y, x, b);
    const float dy1 = tcsa(z, y, b);
    const float dz1 = tcsa(x, z, b);

    const float x1 = x + dx1 * dt / 2.0f;
    const float y1 = y + dy1 * dt / 2.0f;
    const float z1 = z + dz1 * dt / 2.0f;

    const float dx2 = tcsa(y1, x1, b);
    const float dy2 = tcsa(z1, y1, b);
    const float dz2 = tcsa(x1, z1, b);

    const float x2 = x + dx2 * dt / 2.0f;
    const float y2 = y + dy2 * dt / 2.0f;
    const float z2 = z + dz2 * dt / 2.0f;

    const float dx3 = tcsa(y2, x2, b);
    const float dy3 = tcsa(z2, y2, b);
    const float dz3 = tcsa(x2, z2, b);

    const float x3 = x + dx3 * dt;
    const float y3 = y + dy3 * dt;
    const float z3 = z + dz3 * dt;

    const float dx4 = tcsa(y3, x3, b);
    const float dy4 = tcsa(z3, y3, b);
    const float dz4 = tcsa(x3, z3, b);

    x += dt * (dx1 + 2.0f * dx2 + 2.0f * dx3 + dx4) / 6.0f;
    y += dt * (dy1 + 2.0f * dy2 + 2.0f * dy3 + dy4) / 6.0f;
    z += dt * (dz1 + 2.0f * dz2 + 2.0f * dz3 + dz4) / 6.0f;
    */

    const float dx = tcsa(y, x, b);
    const float dy = tcsa(z, y, b);
    const float dz = tcsa(x, z, b);
    x += frequency * dx;
    y += frequency * dy;
    z += frequency * dz;

    float squashed = amp * (offset + x / (1.0f + fabsf(x)));

    out->value = value_ = lp_= squashed;
    out->segment = active_segment_ = 0;
    ++out;
  }
  x_ = x;
  y_ = y;
  z_ = z;
}

#define DS_DXDT(x,y,z) a * (y - x)
#define DS_DYDT(x,y,z) (c - a) * x - x * z + c * y
#define DS_DZDT(x,y,z) x * y - b * z

void SegmentGenerator::ProcessDoubleScrollAttractor(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  float f = 96.0f * (parameters_[0].primary - 0.5f);
  CONSTRAIN(f, -128.0f, 127.0f);

  active_segment_ = 0;
  // 1.4 gives a similar feel to the LFO speeds here
  float frequency = SemitonesToRatio(f) * 1.3f * 2.0439497f / kSampleRate;
  switch (segments_[active_segment_].range) {
    case segment::RANGE_SLOW:
      // Range from ~8s to ~30min
      frequency /= (16.0f * 16.0f);
      break;
    case segment::RANGE_FAST:
      // ~32hz to ~8s.
      break;
    default:
      // ~2hz to ~120s.
      frequency /= 16.0f;
      break;
  }
  // Could increase to 0.075 if we used runge-kutta
  CONSTRAIN(frequency, 0.0f, 0.01f);


  const float a = 42.0f;
  const float max_b = 6.0f;
  const float min_b = 1.0f;
  const float b = ((max_b - min_b) * parameters_[0].secondary + min_b);
  //CONSTRAIN(b, min_b, max_b);
  const float c = 28.0f;

  const bool bipolar = segments_[0].bipolar;

  const float offset = bipolar ? -0.5f : 0.0f;
  const float amp = bipolar ? 10.0f / 8.0f : 1.0f;
  float x = x_;
  float y = y_;
  float z = z_;
  while (size--) {
    // Right now, behavior changes a good bit with dt. Could try runge-kutta to fix
    const float dx = DS_DXDT(x, y, z);
    const float dy = DS_DYDT(x, y, z);
    const float dz = DS_DZDT(x, y, z);
    x += frequency * dx;
    y += frequency * dy;
    z += frequency * dz;

    float output = (x + 18.0f) / 36.0f;
    CONSTRAIN(output, 0.0f, 1.0f);

    out->value = value_ = lp_= amp * output + offset;
    out->segment = active_segment_ = output > 0.5f;
    ++out;
  }
  x_ = x;
  y_ = y;
  z_ = z;
}

void SegmentGenerator::ProcessTuring(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  size_t steps = tm_steps(parameters_[0].secondary);
  if (segments_[0].tm_steps != steps) {
    out->changed_segments |= 1;
    segments_[0].tm_steps = steps;
  }
  ParameterInterpolator primary(&primary_, parameters_[0].primary, size);

  Segment* seg = &segments_[0];
  while (size--) {
    float prob_param = primary.Next();
    if (*gate_flags & GATE_FLAG_RISING) {
      advance_tm(
          steps,
          tm_prob(prob_param),
          seg->shift_register,
          seg->register_value,
          seg->bipolar);
      value_ = seg->register_value;
    }
    active_segment_ = *gate_flags & GATE_FLAG_HIGH ? 0 : 1;
    out->value = segments_[0].quant_scale > 0
      ? QuantizeLinear(0, scales[segments_[0].quant_scale], value_, 2)
      : value_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++out;
    ++gate_flags;
  }
}

void SegmentGenerator::ProcessLogistic(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  const float coefficient = PortamentoRateToLPCoefficient(
      parameters_[0].secondary);
  float r = 0.5f * parameters_[0].primary + 3.5f;
  if (value_ <= 0.0f) {
    value_ = Random::GetFloat();
  }

  while (size--) {
    if(*gate_flags & GATE_FLAG_RISING) {
      value_ *= r * (1 - value_);
    }
    active_segment_ = *gate_flags & GATE_FLAG_HIGH ? 0 : 1;

    ONE_POLE(lp_, value_, coefficient);
    out->value = segments_[0].bipolar ? 10.0f / 8.0f * (lp_ - 0.5f) : lp_;
    out->phase = 0.5f;
    out->segment = active_segment_;
    ++out;
    ++gate_flags;
  }
}

void SegmentGenerator::ProcessZero(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  value_ = 0.0f;
  active_segment_ = 1;
  while (size--) {
    out->value = 0.0f;
    out->phase = 0.5f;
    out->segment = 1;
    ++out;
  }
}

void SegmentGenerator::ProcessSlave(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  while (size--) {
    active_segment_ = out->segment == monitored_segment_ ? 0 : 1;
    out->value = active_segment_ ? 0.0f : 1.0f - out->phase;
    ++out;
  }
}

/**
 * Polynomial approximation of the residular between a perfect step and a
 * bandlimited step. This allows you to approximately bandlimit a wave with a
 * value discontinuity. This works by approximating a bandlimited impulse (blit)
 * as a simple upside down triangle (t from -dt to dt, achieving peak of 1 at
 * t=0). That is then integrated to get an approximate bandlimited step (blep),
 * and then the difference between the blep and a perfect step is what we find
 * in this function. This can then be added to a naive step to bandlimit it.
 * This does effectively the same thing as ThisBlepSample(float) and
 * NextBlepSample(float), but is written such that we don't have to accumulate
 * anything across time, which makes it work well with the original LFO
 * functions (and I find a bit simpler to think about).
 * This page does a nice job of walking through the math:
 * https://www.metafunction.co.uk/post/all-about-digital-oscillators-part-2-blits-bleps
 * Not actually using this, but keeping it around in case I forget how polyblep
 * works.
*/
float polyblep(float t, float dt) {
  if (t < dt) {
    t = t / dt - 1.0f;
    return -t * t;
  } else if (t > 1.0f - dt) {
    t = (t - 1.0f) / dt + 1.0f;
    return t * t;
  }
  return 0.0f;
}

/**
 * Polynomial approximation of the residual with a bandlimited ramp. This allows
 * you to approximiately bandlimit a wave with a discontinuity in the first
 * derivative (e.g. a triangle). A bandlimited change in derivative is just an
 * integrated blep (see above). Thus, this is just the integral of polyblep.
 * It should be scaled by the change in slope times dt. Unfortunately, I cannot
 * now find the article that helped this click for me, but it does seem to work.
 * Similar to ThisIntegratedBlepSample(float) and
 * NextIntegratedBlepSample(float), but I don't understand why those are fourth
 * order while the blep sample ones are second order.
 * Not actually using this, but keeping it around in case I forget how polyblamp
 * works.
*/
float polyblamp(float t, float dt) {
  if (t < dt) {
    t = t / dt - 1.0f;
    return (-1.0f / 3.0f) * t * t * t;
  } else if (t > 1.0f - dt) {
    t = (t - 1.0f) / dt + 1.0f;
    return (1.0f / 3.0f) * t * t * t;
  }
  return 0.0f;
}

float spline_lfo(const float attack, const float attack_slope, const float pw,
                 const float release, const float release_slope,
                 const float up_slope, const float down_slope, float t) {
  if (t <= attack + pw) {
    if (t > attack) return 1.0f;
    return spline(-1.0f, up_slope, 1.0f, up_slope, t * attack_slope);
  } else {
    t -= attack + pw;
    if (t >= release) return -1.0f;
    return spline(1.0f, down_slope, -1.0f, down_slope, t * release_slope);
  }
}

  template <bool bandlimit>
  void SegmentGenerator::ShapeSplineLFO(
      float shape, float frequency, const float *input_phase,
      SegmentGenerator::Output *out, size_t size, bool bipolar) {
    const float ramp_boundary = 0.333f;
    const float trap_boundary = 0.667f;
    // The following settings reproduce the response curve of the original LFO
    // shape control, but seem to have a measurable performance impact in my
    // testing. The above settings felt okay in practice, so sticking with them
    // for performance and simplicity.
    // shape -= 0.5f;
    // shape = (2.0f + 9.999999f * shape / (1.0f + 3.0f * fabs(shape))) / 4.0f;
    // const float ramp_boundary = 0.25f;
    // const float trap_boundary = 0.75f;
    float attack, pw1, release, up_slope, down_slope;
    if (shape <= ramp_boundary) {
      attack = shape / (2.0f * ramp_boundary);
      pw1 = 0.0f;
      release = 1.0f - attack;
      up_slope = 2.0f;
    } else if (shape <= trap_boundary) {
      attack = 0.5f;
      pw1 = 0.0f;
      release = 0.5f;
      up_slope = 2.0f * fabs(shape - 0.5f) / (0.5f - ramp_boundary);
    } else {
      const float pw = (shape - trap_boundary) / (1.0f - trap_boundary);
      attack = (1.0f - pw) * 0.5f;
      pw1 = pw * 0.5f;
      release = attack;
      up_slope = 2.0f;
    }
    down_slope = -up_slope;
    if (bandlimit) {
      // This is not a traditional polyblep implementation. Instead, it uses the
      // spline function to create a 3rd order polynominal approximation of a
      // step that 2*dt wide... which is, if I understand, basically what
      // polyblep is trying to do (though polyblep often uses a 2nd order
      // approximation), but does so by applying a correction to a naive step
      // instead. AFAICT they sound pretty much the same.
      // The big advantage of this approach is that it allows a smooth
      // transition from trapezoid to square without losing performance since
      // this just takes advantage of what we were already doing. Special
      // purpose square wave and special purpose trapezoid implementations can
      // be faster, but smooth application of polyblep as you transition
      // destroys that, is a pain, and sounds worse.
      // Note that this code does *not* attempt to bandlimit discontinuities in
      // first derivative (ie what blamp methods do), but it could be extended
      // to do so be adding a 2*dt width splined transition from one slope to
      // the other. This would incur a performance hit though and, honestly,
      // when I tried adding polyblamp to other implementations of this, it only
      // sounded marginally better to me.
      const float dp2 = 2.0f * frequency;
      if (attack < dp2) {
        up_slope *= attack / dp2;
        if (pw1 > dp2 - attack) pw1 -= dp2 - attack;
        else release -= dp2 - attack;
        attack = dp2;
      }
      if (release < dp2) {
        down_slope *= release / dp2;
        release = dp2;
      }
    }
    const float attack_slope = attack == 0.0f ? 0.0f : 1.0f / attack;
    const float release_slope = release == 0.0f ? 0.0f : 1.0f / release;

    const float amplitude = bipolar ? 10.0f / 16.0f : 0.5f;
    const float offset = bipolar ? 0.0f : 0.5f;
    while (size--) {
      const float phase = *input_phase;
      out->phase = phase;
      out->value =
          amplitude * spline_lfo(attack, attack_slope, pw1, release,
                                 release_slope, up_slope, down_slope, phase) +
          offset;
      out->segment = phase < 0.5f ? 0 : 1;
      ++out;
      ++input_phase;
    }
  }

  void SegmentGenerator::ShapeLFO(float shape, const float *input_phase,
                                  SegmentGenerator::Output *out, size_t size,
                                  bool bipolar) {
    shape -= 0.5f;
    shape = 2.0f + 9.999999f * shape / (1.0f + 3.0f * fabs(shape));

    const float slope = min(shape * 0.5f, 0.5f);
    const float plateau_width = max(shape - 3.0f, 0.0f);
    const float sine_amount =
        max(shape < 2.0f ? shape - 1.0f : 3.0f - shape, 0.0f);

    const float slope_up = 1.0f / slope;
    const float slope_down = 1.0f / (1.0f - slope);
    const float plateau = 0.5f * (1.0f - plateau_width);
    const float normalization = 1.0f / plateau;
    const float phase_shift = plateau_width * 0.25f;

    const float amplitude = bipolar ? (10.0f / 16.0f) : 0.5f;
    const float offset = bipolar ? 0.0f : 0.5f;
    while (size--) {
      float phase = *input_phase + phase_shift;
      if (phase > 1.0f) {
        phase -= 1.0f;
      }
      float triangle = phase < slope ? slope_up * phase
                                     : 1.0f - (phase - slope) * slope_down;
      triangle -= 0.5f;
      CONSTRAIN(triangle, -plateau, plateau);
      triangle = triangle * normalization;
      // Using Interpolate instead of InterpolateWrap gives about about about a
      // 20ms speedup in benchmarks and allows an extra synced segment  float
      // sine = InterpolateWrap(lut_sine, phase + 0.75f, 1024.0f);
      float sine = Interpolate(
          lut_sine, phase < 0.25f ? phase + 0.75f : phase - 0.25f, 1024.0f);
      out->phase = *input_phase;
      out->value = amplitude * Crossfade(triangle, sine, sine_amount) + offset;
      out->segment = phase < 0.5f ? 0 : 1;
      ++out;
      ++input_phase;
    }
  }

inline bool is_step(Configuration config) {
  // Looping Turing types are holds
  return config.type == TYPE_STEP
    || (config.type == TYPE_TURING && !config.loop);
}

void SegmentGenerator::ProcessSequencer(
    const GateFlags* gate_flags, SegmentGenerator::Output* out, size_t size) {
  // Read the value of the small pot to determine the direction.
  Direction direction = Direction(function_quantizer_.Process(
      parameters_[0].secondary));

  int last_active = active_segment_;
  if (direction == DIRECTION_ADDRESSABLE) {
    reset_ = false;
    if (!hold_address_) {
      active_segment_ =
          address_quantizer_.Process(parameters_[0].primary) + first_step_;
    }
  } else {
    hold_address_ = false;
    // Detect a rising edge on the slider/CV to reset to the first step.
    if (parameters_[0].primary > 0.125f && !reset_) {
      reset_ = true;
      active_segment_ = direction == DIRECTION_DOWN ? last_step_ : first_step_;
      up_down_counter_ = 0;
      inhibit_clock_ = kClockInhibitDelay;
    }
    if (reset_ && parameters_[0].primary < 0.0625f) {
      reset_ = false;
    }
  }
  while (size--) {
    if (inhibit_clock_) {
      --inhibit_clock_;
    }

    bool clockable = !inhibit_clock_ && !reset_;

    // If a rising edge is detected on the gate input, advance to the next step.
    if ((*gate_flags & GATE_FLAG_RISING) && clockable) {
      switch (direction) {
        case DIRECTION_ADDRESSABLE:
          hold_address_ = true;
          active_segment_ =
              address_quantizer_.Process(parameters_[0].primary) + first_step_;
          break;
        case DIRECTION_UP:
          ++active_segment_;
          if (active_segment_ > last_step_) {
            active_segment_ = first_step_;
          }
          break;

        case DIRECTION_DOWN:
          --active_segment_;
          if (active_segment_ < first_step_) {
            active_segment_ = last_step_;
          }
          break;

        case DIRECTION_UP_DOWN:
          {
            int n = last_step_ - first_step_ + 1;
            if (n == 1) {
              active_segment_ = first_step_;
            } else {
              up_down_counter_ = (up_down_counter_ + 1) % (2 * (n - 1));
              active_segment_ = first_step_ + (up_down_counter_ < n
                  ? up_down_counter_
                  : 2 * (n - 1) - up_down_counter_);
            }
          }
          break;

        case DIRECTION_ALTERNATING:
          {
            int n = last_step_ - first_step_ + 1;
            if (n == 1) {
              active_segment_ = first_step_;
            } else if (n == 2) {
              up_down_counter_ = (up_down_counter_ + 1) % 2;
              active_segment_ = first_step_ + up_down_counter_;
            } else {
              up_down_counter_ = (up_down_counter_ + 1) % (4 * n - 8);
              int i = (up_down_counter_ - 1) / 2;
              active_segment_ = first_step_ + ((up_down_counter_ & 1)
                  ? 1 + ((i < (n - 1)) ? i : 2 * (n - 2) - i)
                  : 0);
            }
          }
          break;

        case DIRECTION_RANDOM:
          active_segment_ = first_step_ + static_cast<int>(
              Random::GetFloat() * static_cast<float>(
                  last_step_ - first_step_ + 1));
          break;

        case DIRECTION_RANDOM_WITHOUT_REPEAT:
          {
            int n = last_step_ - first_step_ + 1;
            int r = static_cast<int>(
                Random::GetFloat() * static_cast<float>(n - 1));
            active_segment_ = first_step_ + \
                ((active_segment_ - first_step_ + r + 1) % n);
          }
          break;

        case DIRECTION_LAST:
          break;
      }
    }

    value_ = segments_[active_segment_].advance_tm ?
      segments_[active_segment_].register_value
      : parameters_[active_segment_].primary;
    if (quantized_output_) {
      value_ = QuantizeLinear(active_segment_, scales[1], value_, 1);
    }
    if ((last_active != active_segment_) && segments_[last_active].advance_tm) {
      const float steps_param = parameters_[last_active].secondary;
      const float prob_param = parameters_[last_active].primary;
      advance_tm(
          steps_param, prob_param,
          (&segments_[last_active])->shift_register,
          (&segments_[last_active])->register_value,
          segments_[last_active].bipolar);
    }
    // TODO: Worth using segs.portamento_ instead of branches? If AR ever
    // suffers, worth checking out.
    const float port = segments_[active_segment_].advance_tm
      ? 0.0f : parameters_[active_segment_].secondary;

    ONE_POLE(
        lp_,
        value_,
        PortamentoRateToLPCoefficient(port));

    last_active = active_segment_;
    out->value = lp_;
    out->phase = 0.0f;
    out->segment = active_segment_;
    ++gate_flags;
    ++out;
  }
}

void SegmentGenerator::ConfigureSequencer(
    const Configuration* segment_configuration,
    int num_segments) {
  num_segments_ = num_segments;

  first_step_ = 0;
  for (int i = 1; i < num_segments; ++i) {
    if (segment_configuration[i].loop) {
      if (!first_step_) {
        first_step_ = last_step_ = i;
      } else {
        last_step_ = i;
      }
    }
    segments_[i].advance_tm =
      (segment_configuration[i].type == segment::TYPE_TURING);
  }
  if (!first_step_) {
    // No loop has been found, use the whole group.
    first_step_ = 1;
    last_step_ = num_segments - 1;
  }

  int num_steps = last_step_ - first_step_ + 1;
  address_quantizer_.Init(
      num_steps,
      0.02f / 8.0f * static_cast<float>(num_steps),
      false);

  inhibit_clock_ = up_down_counter_ = 0;
  quantized_output_ = (segment_configuration[0].type == TYPE_RAMP) && \
      step_quantizer_;
  reset_ = false;
  lp_ = value_ = 0.0f;
  active_segment_ = first_step_;
  process_fn_ = &SegmentGenerator::ProcessSequencer;
}

void SegmentGenerator::Configure(
    bool has_trigger,
    const Configuration* segment_configuration,
    int num_segments) {
  if (num_segments == 1) {
    function_quantizer_.Init(num_divider_ratios[segment_configuration[0].range],
                             0.025f, false);
    ConfigureSingleSegment(has_trigger, segment_configuration[0]);
    return;
  }

  bool sequencer_mode = segment_configuration[0].type != TYPE_STEP &&
                        segment_configuration[0].type != TYPE_TURING &&
                        !segment_configuration[0].loop && num_segments >= 3;
  for (int i = 1; i < num_segments; ++i) {
    sequencer_mode = sequencer_mode && is_step(segment_configuration[i]);
  }
  if (sequencer_mode) {
    function_quantizer_.Init(DIRECTION_LAST, 0.025f, false);
    ConfigureSequencer(segment_configuration, num_segments);
    return;
  }

  num_segments_ = num_segments;

  // assert(has_trigger);

  process_fn_ = &SegmentGenerator::ProcessMultiSegment;

  // A first pass to collect loop points, and check for STEP segments.
  int loop_start = -1;
  int loop_end = -1;
  bool has_step_segments = false;
  int last_segment = num_segments - 1;
  int first_ramp_segment = -1;

  for (int i = 0; i <= last_segment; ++i) {
    has_step_segments = has_step_segments || is_step(segment_configuration[i]);
    if (segment_configuration[i].loop) {
      if (loop_start == -1) {
        loop_start = i;
      }
      loop_end = i;
    }
    if (segment_configuration[i].type == TYPE_RAMP) {
      if (first_ramp_segment == -1) {
        first_ramp_segment = i;
      }
    }
  }

  // Check if there are step segments inside the loop.
  bool has_step_segments_inside_loop = false;
  if (loop_start != -1) {
    for (int i = loop_start; i <= loop_end; ++i) {
      if (is_step(segment_configuration[i])) {
        has_step_segments_inside_loop = true;
        break;
      }
    }
  }

  for (int i = 0; i <= last_segment; ++i) {
    Segment* s = &segments_[i];
    s->bipolar = segment_configuration[i].bipolar;
    s->retrig = true;
    s->advance_tm = false;
    if (segment_configuration[i].type == TYPE_RAMP) {
      s->retrig = !s->bipolar; // For ramp, bipolar means don't retrig.
      s->start = (num_segments == 1) ? &one_ : NULL;
      s->time = &parameters_[i].primary;
      s->curve = &parameters_[i].secondary;
      s->portamento = &zero_;
      s->phase = NULL;

      if (i == last_segment) {
        s->end = &zero_;
      } else if (segment_configuration[i + 1].type == TYPE_TURING) {
        s->end = &segments_[i+1].register_value;
      } else if (segment_configuration[i + 1].type != TYPE_RAMP) {
        s->end = &parameters_[i + 1].primary;
      } else if (i == first_ramp_segment) {
        s->end = &one_;
      } else {
        s->end = &parameters_[i].secondary;
        // The whole "reuse the curve from other segment" thing
        // is a bit too complicated...
        //
        // for (int j = i + 1; j <= last_segment; ++j) {
        //   if (segment_configuration[j].type == TYPE_RAMP) {
        //     if (j == last_segment ||
        //         segment_configuration[j + 1].type != TYPE_RAMP) {
        //       s->curve = &parameters_[j].secondary;
        //       break;
        //     }
        //   }
        // }
        s->curve = &half_;
      }
    } else {
      s->start = s->end = &parameters_[i].primary;
      s->curve = &half_;
      if (segment_configuration[i].type == TYPE_STEP) {
        s->portamento = &parameters_[i].secondary;
        s->time = NULL;
        // Sample if there is a loop of length 1 on this segment. Otherwise
        // track.
        s->phase = i == loop_start && i == loop_end ? &zero_ : &one_;
      } else if (segment_configuration[i].type == TYPE_TURING) {
        s->start = s->end = &s->register_value;
        s->advance_tm = true;
        s->portamento = &zero_;
        s->time = NULL;
        s->phase = &zero_;
      } else {
        s->portamento = &zero_;
        // Hold if there's a loop of length 1 of this segment. Otherwise, use
        // the programmed time.
        s->time = i == loop_start && i == loop_end
            ? NULL : &parameters_[i].secondary;
        s->phase = &one_;  // Track the changes on the slider.
      }
    }

    s->if_complete = i == loop_end ? loop_start : i + 1;
    s->if_falling = loop_end == -1 || loop_end == last_segment || has_step_segments ? -1 : loop_end + 1;
    s->if_rising = 0;

    if (has_step_segments) {
      if (!has_step_segments_inside_loop && i >= loop_start && i <= loop_end) {
        s->if_rising = (loop_end + 1) % num_segments;
      } else {
        // Just go to the next stage.
        // s->if_rising = (i == loop_end) ? loop_start : (i + 1) % num_segments;

        // Find the next STEP segment.
        bool follow_loop = loop_end != -1;
        int next_step = i;
        while (!is_step(segment_configuration[next_step])) {
          ++next_step;
          if (follow_loop && next_step == loop_end + 1) {
            next_step = loop_start;
            follow_loop = false;
          }
          if (next_step >= num_segments) {
            next_step = num_segments - 1;
            break;
          }
        }
        s->if_rising = next_step == loop_end
            ? loop_start
            : (next_step + 1) % num_segments;
      }
    }
  }

  Segment* sentinel = &segments_[num_segments];
  sentinel->end = sentinel->start = segments_[num_segments - 1].end;
  sentinel->time = &zero_;
  sentinel->curve = &half_;
  sentinel->portamento = &zero_;
  sentinel->if_rising = 0;
  sentinel->if_falling = -1;
  sentinel->if_complete = loop_end == last_segment ? 0 : -1;
  sentinel->retrig = true;

  // After changing the state of the module, we go to the sentinel.
  previous_segment_ = active_segment_ = num_segments;
}

/* static */
SegmentGenerator::ProcessFn SegmentGenerator::process_fn_table_[16] = {
  // RAMP
  &SegmentGenerator::ProcessZero,
  &SegmentGenerator::ProcessFreeRunningLFO,
  &SegmentGenerator::ProcessDecayEnvelope,
  &SegmentGenerator::ProcessTapLFO,

  // STEP
  &SegmentGenerator::ProcessPortamento,
  &SegmentGenerator::ProcessPortamento,
  &SegmentGenerator::ProcessSampleAndHold,
  &SegmentGenerator::ProcessSampleAndHold,

  // HOLD
  &SegmentGenerator::ProcessDelay,
  &SegmentGenerator::ProcessDelay,
  // &SegmentGenerator::ProcessClockedSampleAndHold,
  &SegmentGenerator::ProcessTimedPulseGenerator,
  &SegmentGenerator::ProcessGateGenerator,

  // These types can't normally be accessed, but are what random segments default
  // to in basic mode.
  &SegmentGenerator::ProcessZero,
  &SegmentGenerator::ProcessZero,
  &SegmentGenerator::ProcessZero,
  &SegmentGenerator::ProcessZero,
};

// Seems really silly to have to separate tables with just a single difference but meh
SegmentGenerator::ProcessFn SegmentGenerator::advanced_process_fn_table_[16] = {
  // RAMP
  &SegmentGenerator::ProcessRiseAndFall,
  &SegmentGenerator::ProcessFreeRunningLFO,
  &SegmentGenerator::ProcessDecayEnvelope,
  &SegmentGenerator::ProcessTapLFO,

  // STEP
  &SegmentGenerator::ProcessPortamento,
  &SegmentGenerator::ProcessAttOff,
  &SegmentGenerator::ProcessSampleAndHold,
  &SegmentGenerator::ProcessAttSampleAndHold,

  // HOLD
  &SegmentGenerator::ProcessDelay,
  &SegmentGenerator::ProcessDelay,
  // &SegmentGenerator::ProcessClockedSampleAndHold,
  &SegmentGenerator::ProcessTimedPulseGenerator,
  &SegmentGenerator::ProcessProbabilisticGateGenerator,

  // TURING
  &SegmentGenerator::ProcessDoubleScrollAttractor,
  &SegmentGenerator::ProcessFreeRunningRandomLFO,
  //&SegmentGenerator::ProcessThomasSymmetricAttractor,
  &SegmentGenerator::ProcessTuring,
  //&SegmentGenerator::ProcessLogistic,
  &SegmentGenerator::ProcessTapRandomLFO,
};


}  // namespace stages
