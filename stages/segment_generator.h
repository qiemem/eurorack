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
// Multi-stage envelope.

#ifndef STAGES_SEGMENT_GENERATOR_H_
#define STAGES_SEGMENT_GENERATOR_H_

#include "stmlib/dsp/delay_line.h"
#include "stmlib/dsp/hysteresis_quantizer.h"
#include "stmlib/utils/gate_flags.h"

#include "tides2/ramp/ramp_extractor.h"
#include "stages/delay_line_16_bits.h"

#include "stages/modes.h"
#include "stmlib/utils/random.h"
#include "stages/quantizer.h"
#include "stages/oscillator.h"
#include "stages/variable_shape_oscillator.h"
#include "stages/modes.h"

namespace stages {

const float kSampleRate = 31250.0f;

// Each segment generator can handle up to 36 segments. That's a bit of a waste
// of RAM because the 6 generators running on a module will never have to deal
// with 36 segments each. But it was a bit too much to have a shared pool of
// pre-allocated Segments shared by all SegmentGenerators!
const int kMaxNumSegments = 36;
const int kMaxNumLocalSegments = 6;

const size_t kMaxDelay = 576;

#define DECLARE_PROCESS_FN(X) void Process ## X \
      (const stmlib::GateFlags* gate_flags, Output* out, size_t size);

namespace segment {

// High level descriptions / parameters.
enum Type {
  TYPE_RAMP = 0,
  TYPE_STEP = 1,
  TYPE_HOLD = 2,
  TYPE_TURING = 3,
};

enum FreqRange {
  RANGE_DEFAULT = 0,
  RANGE_SLOW = 1,
  RANGE_FAST = 2,
  RANGE_AUDIO = 3,
};

struct Configuration {
  Type type;
  bool loop;
  bool bipolar;
  FreqRange range; // For LFOs
  int quant_scale; // For TMs
  bool reset_on_gate; // For LFOs
};

struct Parameters {
  // Segment type  | Main  | Secondary
  //
  // RAMP          | Time  | Shape (or level if followed by RAMP)
  // HOLD          | Level | Time
  // STEP          | Level | Shape (portamento)
  // TURING        | Prob  | Sequence length
  float primary;
  float secondary;
};

struct LocalParameters {
  float cv;
  float slider;
};

}  // namespace segment

class SegmentGenerator {
 public:
  SegmentGenerator() { }
  ~SegmentGenerator() { }

  struct Output {
    float value;
    float phase;
    uint8_t segment;
    // bit representation of which channels have changed discrete state,
    // starting with the current channel (ie rightmost bit is current
    // channel). Will only be something besides 1 or 0 for groups.
    uint32_t changed_segments;
  };

  struct Segment {
    // Low level state.

    float* start;  // NULL if we should start from the current value.
    float* time;  // NULL if the segment has infinite duration.
    float* curve;
    float* portamento;
    float* end;
    float* phase;

    int8_t if_rising;
    int8_t if_falling;
    int8_t if_complete;
    bool bipolar;
    bool retrig;
    segment::FreqRange range;
    uint8_t quant_scale;

    bool advance_tm;
    uint16_t shift_register;
    float register_value;
    size_t tm_steps;
  };
  
  void Init() {
    Init(MULTI_MODE_STAGES, NULL);
  }
  
  void Init(MultiMode multimode, stmlib::HysteresisQuantizer2* step_quantizer);
  
  typedef void (SegmentGenerator::*ProcessFn)(
      const stmlib::GateFlags* gate_flags, Output* out, size_t size);

  bool Process(
      const stmlib::GateFlags* gate_flags, Output* out, size_t size) {
    (this->*process_fn_)(gate_flags, out, size);
    return active_segment_ == 0;
  }

  void SetMode(MultiMode multimode) {
    multimode_ = multimode;
  }

  void Configure(
      bool has_trigger,
      const segment::Configuration* segment_configuration,
      int num_segments);

  void ConfigureSequencer(
      const segment::Configuration* segment_configuration,
      int num_segments);

  inline void ConfigureSingleSegment(
      bool has_trigger,
      const segment::Configuration segment_configuration) {

    int i = has_trigger ? 2 : 0;
    i += segment_configuration.loop ? 1 : 0;
    int type = int(segment_configuration.type);
    i += type * 4;
    ProcessFn new_process_fn =
        (multimode_ == MULTI_MODE_STAGES_ADVANCED
        ? advanced_process_fn_table_ : process_fn_table_)[i];
    if (new_process_fn != process_fn_
        || segments_[0].range != segment_configuration.range) {
      ramp_extractor_.Reset();
    }
    process_fn_ = new_process_fn;
    segments_[0].range = segment_configuration.range;
    segments_[0].bipolar = segment_configuration.bipolar;
    segments_[0].retrig = (segment_configuration.type != segment::TYPE_RAMP)
        || !segment_configuration.bipolar;
    segments_[0].quant_scale = segment_configuration.quant_scale;
    reset_on_gate_ = segment_configuration.reset_on_gate;
    num_segments_ = 1;
    ResetPllCounter();
  }

  const int kPllCountdown = static_cast<int>(kSampleRate * 0.25f);
  inline void ResetPllCounter() {
    pll_counter_ = kPllCountdown;
  }

  inline void ConfigureSlave(int i) {
    monitored_segment_ = i;
    process_fn_ = &SegmentGenerator::ProcessSlave;
    num_segments_ = 0;
  }

  // -1.0f -> -octaves (in pitch) and 1.0f -> octaves (in pitch)
  float QuantizeLinear(int seg, const Scale& scale, float value, int octaves) {
    int num_steps = 2 * octaves * scale.num_notes + 1;
    if (step_quantizer_[seg].num_steps() != num_steps) {
      step_quantizer_[seg].Init(num_steps, 0.03f, false);
    }
    int ix = step_quantizer_[seg].Process((value + 1.0f) / 2.0f);
    int16_t pitch = scale.notes[ix % scale.num_notes] + (ix / scale.num_notes) * scale.span;
    pitch -= octaves * scale.span;
    return static_cast<float>(pitch) / eight_octaves;
  }

  void set_segment_parameters(int index, float primary, float secondary) {
    // assert (primary >= -1.0f && primary <= 2.0f)
    // assert (secondary >= 0.0f && secondary <= 1.0f)
    parameters_[index].primary = primary;
    parameters_[index].secondary = secondary;
  }

  void set_segment_parameters(int index,
          float primary, float secondary,
          float cv, float slider) {
    // assert (primary >= -1.0f && primary <= 2.0f)
    // assert (secondary >= 0.0f && secondary <= 1.0f)
    parameters_[index].primary = primary;
    parameters_[index].secondary = secondary;
    local_parameters_[index].slider = slider;
    local_parameters_[index].cv = cv;
  }

  inline int num_segments() {
    return num_segments_;
  }

  inline bool needs_attenuation() const {
    return process_fn_ == &SegmentGenerator::ProcessAttOff || process_fn_ == &SegmentGenerator::ProcessAttSampleAndHold;
  }

  inline bool needs_cv_preprocessing() const {
    return !(
      process_fn_ == &SegmentGenerator::ProcessFreeRunningLFO
      || process_fn_ == &SegmentGenerator::ProcessTapLFO
      || process_fn_ == &SegmentGenerator::ProcessTuring
    );
  }

 private:
  // Process function for the general case.
  DECLARE_PROCESS_FN(MultiSegment);
  DECLARE_PROCESS_FN(RiseAndFall);
  DECLARE_PROCESS_FN(Sequencer)
  DECLARE_PROCESS_FN(DecayEnvelope);
  DECLARE_PROCESS_FN(TimedPulseGenerator);
  DECLARE_PROCESS_FN(GateGenerator);
  DECLARE_PROCESS_FN(ProbabilisticGateGenerator);
  DECLARE_PROCESS_FN(SampleAndHold);
  DECLARE_PROCESS_FN(TrackAndHold);
  DECLARE_PROCESS_FN(TapLFO);
  DECLARE_PROCESS_FN(FreeRunningLFO);
  // DECLARE_PROCESS_FN(PLLOscillator);
  // DECLARE_PROCESS_FN(FreeRunningOscillator);
  DECLARE_PROCESS_FN(Delay);
  DECLARE_PROCESS_FN(AttOff);
  DECLARE_PROCESS_FN(AttSampleAndHold);
  DECLARE_PROCESS_FN(Portamento);
  DECLARE_PROCESS_FN(FreeRunningRandomLFO);
  DECLARE_PROCESS_FN(TapRandomLFO);
  DECLARE_PROCESS_FN(ThomasSymmetricAttractor);
  DECLARE_PROCESS_FN(DoubleScrollAttractor);
  DECLARE_PROCESS_FN(Turing);
  DECLARE_PROCESS_FN(Logistic);
  DECLARE_PROCESS_FN(Zero);
  DECLARE_PROCESS_FN(ClockedSampleAndHold);
  DECLARE_PROCESS_FN(Slave);

  void ProcessRandomFromPhase(float smoothness, Output* in_out, size_t size);

  void ProcessOscillator(const stmlib::GateFlags* gate_flags,
      Output* out,size_t size);

  static void ShapeLFO(float shape, const float *phase, Output *out,
                       size_t size, bool bipolar);
  template <bool bandlimit>
  static void
  ShapeSplineLFO(float shape, float frequencey, const float *input_phase,
                 SegmentGenerator::Output *out, size_t size, bool bipolar);
  float PortamentoRateToLPCoefficient(float rate) const;

  float phase_;
  float aux_;
  float previous_delay_sample_;

  float start_;
  float value_;
  float next_; // used for spline interpolation in smooth random
  float lp_;
  float primary_;

  float zero_;
  float half_;
  float one_;

  int previous_segment_;
  int active_segment_;
  int monitored_segment_;
  int retrig_delay_;

  int num_segments_;

  MultiMode multimode_;

  ProcessFn process_fn_;

  bool smooth_audio_rate_tracking_;
  int pll_counter_;
  tides::RampExtractor ramp_extractor_;
  stmlib::HysteresisQuantizer2 function_quantizer_;

  Segment segments_[kMaxNumSegments + 1];  // There's a sentinel!
  segment::Parameters parameters_[kMaxNumSegments];
  segment::LocalParameters local_parameters_[kMaxNumLocalSegments];

  DelayLine16Bits<kMaxDelay> delay_line_;
  stmlib::DelayLine<stmlib::GateFlags, 128> gate_delay_;

  static ProcessFn process_fn_table_[16];
  static ProcessFn advanced_process_fn_table_[16];

  enum Direction {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_UP_DOWN,
    DIRECTION_ALTERNATING,
    DIRECTION_RANDOM,
    DIRECTION_RANDOM_WITHOUT_REPEAT,
    DIRECTION_ADDRESSABLE,
    DIRECTION_LAST
  };

  int first_step_;
  int last_step_;
  bool quantized_output_;

  int up_down_counter_;
  bool reset_;

  bool accepted_gate_;
  bool hold_address_;
  int inhibit_clock_;
  stmlib::HysteresisQuantizer2 address_quantizer_;
  stmlib::HysteresisQuantizer2* step_quantizer_;

  float x_;
  float y_;
  float z_;

  bool reset_on_gate_;
  VariableShapeOscillator audio_osc_;

  DISALLOW_COPY_AND_ASSIGN(SegmentGenerator);
};

}  // namespace stages

#endif  // STAGES_SEGMENT_GENERATOR_H_
