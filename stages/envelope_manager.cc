// Copyright 2025 Luke Zulauf.
//
// Author: Luke Zulauf (lzulauf@gmail.com)
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


#include "stages/envelope_manager.h"
#include "stages/ui.h"
#include "stages/settings.h"

namespace stages {

  void EnvelopeManager::Init(Settings* settings) {
    settings_ = settings;
    ReInit();
  }

  void EnvelopeManager::ReInit() {
    for (size_t i = 0; i < kNumChannels; ++i) {
      get_envelope(i).Init();
    }
    if (settings_->state().multimode == MULTI_MODE_SIX_INDEPENDENT_EGS) {
      for (size_t i = 0; i < kNumChannels; ++i) {
        const uint8_t* eg_state = settings_->state().independent_eg_state[i];
        Envelope& envelope = get_envelope(i);
        envelope.SetDelayLength(Uint8ToPotOrSlider(eg_state[IEG_DELAY_LENGTH]));
        envelope.SetAttackLength(Uint8ToPotOrSlider(eg_state[IEG_ATTACK_LENGTH]));
        envelope.SetAttackCurve(Uint8ToPotOrSlider(eg_state[IEG_ATTACK_CURVE]));
        envelope.SetHoldLength(Uint8ToPotOrSlider(eg_state[IEG_HOLD_LENGTH]));
        envelope.SetDecayLength(Uint8ToPotOrSlider(eg_state[IEG_DECAY_LENGTH]));
        envelope.SetDecayCurve(Uint8ToPotOrSlider(eg_state[IEG_DECAY_CURVE]));
        envelope.SetSustainLevel(Uint8ToPotOrSlider(eg_state[IEG_SUSTAIN_LEVEL]));
        envelope.SetSustainLength(Uint8ToPotOrSlider(eg_state[IEG_SUSTAIN_LENGTH]));
        envelope.SetReleaseLength(Uint8ToPotOrSlider(eg_state[IEG_RELEASE_LENGTH]));
        envelope.SetReleaseCurve(Uint8ToPotOrSlider(eg_state[IEG_RELEASE_CURVE]));
        envelope.SetLooping((settings_->state().independent_eg_looping >> i) & 1);
      }
    }
  }

  void EnvelopeManager::SetAllDelayLength(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetDelayLength(value);
    }
  }

  void EnvelopeManager::SetAllAttackLength(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetAttackLength(value);
    }
  }

  void EnvelopeManager::SetAllAttackCurve(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetAttackCurve(value);
    }
  }

  void EnvelopeManager::SetAllHoldLength(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetHoldLength(value);
    }
  }

  void EnvelopeManager::SetAllDecayLength(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetDecayLength(value);
    }
  }

  void EnvelopeManager::SetAllDecayCurve(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetDecayCurve(value);
    }
  }

  void EnvelopeManager::SetAllSustainLevel(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetSustainLevel(value);
    }
  }

  void EnvelopeManager::SetAllSustainLength(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetSustainLength(value);
    }
  }

  void EnvelopeManager::SetAllReleaseLength(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetReleaseLength(value);
    }
  }

  void EnvelopeManager::SetAllReleaseCurve(float value) {
    for (uint8_t envelope = 0; envelope < kNumChannels; ++envelope) {
      get_envelope(envelope).SetReleaseCurve(value);
    }
  }

  bool EnvelopeManager::SetDelayLength(uint8_t channel, float value) {
    get_envelope(channel).SetDelayLength(value);
    return SetIndependentEGState(channel, IEG_DELAY_LENGTH, value);
  }

  bool EnvelopeManager::SetAttackLength(uint8_t channel, float value) {
    get_envelope(channel).SetAttackLength(value);
    return SetIndependentEGState(channel, IEG_ATTACK_LENGTH, value);
  }

  bool EnvelopeManager::SetAttackCurve(uint8_t channel, float value) {
    get_envelope(channel).SetAttackCurve(value);
    return SetIndependentEGState(channel, IEG_ATTACK_CURVE, value);
  }

  bool EnvelopeManager::SetHoldLength(uint8_t channel, float value) {
    get_envelope(channel).SetHoldLength(value);
    return SetIndependentEGState(channel, IEG_HOLD_LENGTH, value);
  }

  bool EnvelopeManager::SetDecayLength(uint8_t channel, float value) {
    get_envelope(channel).SetDecayLength(value);
    return SetIndependentEGState(channel, IEG_DECAY_LENGTH, value);
  }

  bool EnvelopeManager::SetDecayCurve(uint8_t channel, float value) {
    get_envelope(channel).SetDecayCurve(value);
    return SetIndependentEGState(channel, IEG_DECAY_CURVE, value);
  }

  bool EnvelopeManager::SetSustainLevel(uint8_t channel, float value) {
    get_envelope(channel).SetSustainLevel(value);
    return SetIndependentEGState(channel, IEG_SUSTAIN_LEVEL, value);
  }

  bool EnvelopeManager::SetSustainLength(uint8_t channel, float value) {
    get_envelope(channel).SetSustainLength(value);
    return SetIndependentEGState(channel, IEG_SUSTAIN_LENGTH, value);
  }

  bool EnvelopeManager::SetReleaseLength(uint8_t channel, float value) {
    get_envelope(channel).SetReleaseLength(value);
    return SetIndependentEGState(channel, IEG_RELEASE_LENGTH, value);
  }

  bool EnvelopeManager::SetReleaseCurve(uint8_t channel, float value) {
    get_envelope(channel).SetReleaseCurve(value);
    return SetIndependentEGState(channel, IEG_RELEASE_CURVE, value);
  }

  bool EnvelopeManager::SetLooping(uint8_t channel, bool looping) {
    get_envelope(channel).SetLooping(looping);
    uint8_t loop_settings = settings_->mutable_state()->independent_eg_looping;
    if (((loop_settings >> channel) & 1) != looping) {
      loop_settings ^= 1 << channel;
      settings_->mutable_state()->independent_eg_looping = loop_settings;
      return true;
    }
    return false;
  }

  bool EnvelopeManager::SetIndependentEGState(uint8_t channel, uint8_t state_offset, float value) {
    uint8_t* eg_state = settings_->mutable_state()->independent_eg_state[channel];
    uint8_t existing_value = eg_state[state_offset];
    uint8_t converted = PotOrSliderToUint8(value);
    if (abs(converted - existing_value) > kNoiseTolerance) {
        eg_state[state_offset] = converted;
        return true;
    }

    return false;
  }

} // namespace stages
