
#include "stages/envelope.h"
#include "stages/envelope_utils.h"

namespace stages {

  const float kMinStageLength = 0.001f;

  void Envelope::Init() {

    stage = IDLE;
    phase = 0.0f;
    stageStartValue = 0.0f;

    for (int i = 0; i < 7; i++) {
      phaseIncrement[i] = 1.0f;
    }

    sustainLevel = 0.0f;

    attackCurve = 0.5f;
    decayCurve = 0.5f;
    releaseCurve = 0.5f;

    gate = false;
    value = 0.0f;

    loop = false;
  }

  float Envelope::Value(bool gate_high) {
    phase += phaseIncrement[stage];

    // Compute stage transitions based on phase >= 1.0f. Cascades as SetStage
    // only resets phase if the target stage exists.
    if (!gate && gate_high) SetStage(DELAY);
    if (stage == IDLE && IsLooping()) SetStage(DELAY);
    if (stage == DELAY && phase >= 1.0f) SetStage(ATTACK);
    if (stage == ATTACK && phase >= 1.0f) SetStage(HOLD);
    if (stage == HOLD && phase >= 1.0f) SetStage(DECAY);
    if (stage == DECAY && phase >= 1.0f) SetStage(SUSTAIN);
    bool sustain_time_done = stage == SUSTAIN && phase >= 1.0f;
    bool exit_sustain = IsLooping()
      ? sustain_time_done
      : !gate_high && (!HasMinSustain() || sustain_time_done) && stage != IDLE
        && stage != RELEASE;
    if (exit_sustain) SetStage(stage != DELAY ? RELEASE : IDLE);
    if (stage == RELEASE && phase >= 1.0f) SetStage(IDLE);
    // We're okay spending one sample in IDLE when looping

    switch (stage) {

      case ATTACK:
        value = Interpolate(stageStartValue, 1.0f, phase, attackCurve);
        break;

      case HOLD:
        value = 1.0f;
        break;

      case DECAY:
        value = Interpolate(1.0f, sustainLevel, phase, decayCurve);
        break;

      case SUSTAIN:
        value = sustainLevel;
        break;

      case RELEASE:
        value = Interpolate(stageStartValue, 0.0f, phase, releaseCurve);
        break;

      default:
        value = 0.0f;
        break;
    }

    gate = gate_high;
    return value;
  }

  void Envelope::SetStage(EnvelopeStage s) {

    // Set stage (if different than current) and reset the phase, but only if
    // the stage exists.
    if (stage != s) {
      stage = s;
      stageStartValue = value;
      // If the stage exists, start at the beginning; if it doesn't, make sure
      // we're at the end.
      phase = HasStage(s) ? 0.0f : 1.0f;
    }
  }

  void Envelope::SetStageLength(float f, EnvelopeStage stage) {

    // If factor is above threshold, set the phase increment per sample, based
    // on RateToFrequency. For stages with 0 duration (very fast), use a high
    // increment to ensure immediate transition
    if (f >= kMinStageLength) {
      phaseIncrement[stage] = RateToFrequency(f);
    } else {
      // For zero duration stages, use an increment that ensures immediate
      // transition. We use this to determine if a stage should be skipped.
      phaseIncrement[stage] = 1.0f;
    }
  }

  void Envelope::SetStageCurve(float f, float* field) {

    // Set curve factor
    *field = f;
  }

  bool Envelope::HasStage(EnvelopeStage stage) {

    // Return if it has a phase increment less than 1.0f (non-zero duration),
    // used for skipping stages and for slider LEDs
    return phaseIncrement[stage] < 1.0f;
  }

  float Envelope::Interpolate(float from, float to, float phase, float curve) {

    // Interpolation is linear for curve = 0.5, ease-in for curve < 0.5,
    // ease-out for curve > 0.5.
    float t = WarpPhase(phase, curve);
    return from + (to - from) * t;
  }
}
