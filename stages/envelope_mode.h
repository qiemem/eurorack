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

// -----------------------------------------------------------------------------

// EnvelopeMode
// Manages modes with DAHDSR (Delay, Attack, Hold, Decay, Sustain, Release)
// envelopes.
//
// The DAHDSR envelope generator is a multi-channel, 6-stage (per channel) envelope
// generator. Each stage is independently configurable.

#ifndef STAGES_ENVELOPE_MODE_H
#define STAGES_ENVELOPE_MODE_H

#include "stmlib/stmlib.h"

#include "stages/envelope_manager.h"

namespace stages {

class Settings;
class Ui;

class EnvelopeMode {
 public:
  EnvelopeMode() { }
  ~EnvelopeMode() { }

  void Init(Settings* settings);
  void ReInit();
  void SetUI(Ui* ui);

  void ProcessEnvelopes(IOBuffer::Block* block, size_t size);

 private:
  EnvelopeManager envelope_manager_;
  Settings* settings_;
  Ui* ui_;

  // The index of the currently selected envelope
  size_t active_envelope_;

  // bootup delay before processing.
  int warm_time_;

  uint16_t switch_pressed_time_[kNumChannels];

  // Initial slider positions (set when switching channels). This allows us to
  // determine once the user has moved a slider sufficiently.
  // We don't use slider locking, since that feature does not support locking to
  // an arbitrary value.
  bool need_to_set_initial_slider_positions_;
  float initial_slider_positions_[kNumChannels];

  // For each envelope feature, whether we are currently using slider values
  // Slider positions are ignored for the active envelope until the user moves
  // them sufficiently to activate them.
  bool slider_enabled_[kNumChannels];

  void ProcessSixIndependentEgs(IOBuffer::Block* block, size_t size);
  void ProcessSixIdenticalEgs(IOBuffer::Block* block, size_t size);
  void ProcessEGs(IOBuffer::Block* block, uint32_t manual_gates, size_t size);

  DISALLOW_COPY_AND_ASSIGN(EnvelopeMode);
};

}  // namespace stages

#endif  // STAGES_ENVELOPE_MODE_H
