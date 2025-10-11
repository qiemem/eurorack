// Copyright 2025 Bryan Head.
//
// Author: Bryan Head (bryan.head@gmail.com)
// Code adapted from Emilie Gillet (emilie.o.gillet@gmail.com)
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
// Utility functions shared between envelope and segment_generator modules.

#ifndef STAGES_ENVELOPE_UTILS_H_
#define STAGES_ENVELOPE_UTILS_H_

#include "stages/resources.h"

namespace stages {

// Warps phase for smooth interpolation with adjustable curve.
// curve = 0.5 gives linear interpolation
// curve < 0.5 gives ease-in (slow start)
// curve > 0.5 gives ease-out (slow end)
inline float WarpPhase(float t, float curve)  {
  curve -= 0.5f;
  const bool flip = curve < 0.0f;
  if (flip) t = 1.0f - t;
  const float a = 128.0f * curve * curve;
  t = (1.0f + a) * t / (1.0f + a * t);
  if (flip) t = 1.0f - t;
  return t;
}

// Converts a rate parameter to frequency using the env_frequency lookup table.
// At 31.25kHz sample rate, the duration to advance phase from 0 to 1 maps as:
// rate = <=0.0: fastest, ~1.0ms duration (index 0 of table, constrained)
// rate = 0.5: medium, ~505ms duration (index 1024 of table)
// rate = 1.0: slow, ~16s duration (index 2048 of table)
// rate = 2.0: slowest, ~19 minutes duration (index 4095 of table, constrained)
// Rate parameter is scaled by 2048 to index into the lookup table.
// Lower rates give faster transitions/shorter durations (until clamped at index 0), 
// higher rates give slower transitions/longer durations (until clamped at index 4095).
inline float RateToFrequency(float rate) {
  int32_t i = static_cast<int32_t>(rate * 2048.0f);
  CONSTRAIN(i, 0, LUT_ENV_FREQUENCY_SIZE - 1);
  return lut_env_frequency[i];
}

}  // namespace stages

#endif  // STAGES_ENVELOPE_UTILS_H_
