
#include "stages/envelope.h"
#include "stages/segment_generator.h"

#include <algorithm>

namespace stages {

const float kMinStageLength = 0.001f;
const float timeScale = kSampleRate * 10;

  
  void Envelope::Init() {
    
    stage = IDLE;
    stageTime = 0L;
    stageStartValue = 0.0f;
    
    delayLength = 0L;
    attackLength = 0L;
    holdLength = 0L;
    decayLength = 0L;
    sustainLevel = 0.0f;
    releaseLength = 0L;
    
    attackCurve = 0.5f;
    decayCurve = 0.5f;
    releaseCurve = 0.5f;
    
    gate = false;
    value = 0.0f;
    
  }
  
  void Envelope::Gate(bool high) {
    
    // Rising
    if (!gate && high) {
      SetStage(HasDelay() ? DELAY : ATTACK);
    }
    
    // Falling
    if (gate && !high) {
      switch (stage) {
        
        // Didn't start yet, back to idle
        case IDLE:
        case DELAY:
          SetStage(IDLE);
          break;
          
        // Else, skip to release stage, or go idle if current value is already zero
        default:
          if (value > 0.001f) {
            SetStage(RELEASE);
          } else {
            SetStage(IDLE);
          }
          break;
          
      }
    }
    
    // Update
    gate = high;
    
  }
  
  float Envelope::Value() {
    
    // Compute stage transitions (cascading)
    if (stage == DELAY   && stageTime >= delayLength  ) SetStage(ATTACK );
    if (stage == ATTACK  && stageTime >= attackLength ) SetStage(HOLD   );
    if (stage == HOLD    && stageTime >= holdLength   ) SetStage(DECAY  );
    if (stage == DECAY   && stageTime >= decayLength  ) SetStage(SUSTAIN);
    if (stage == RELEASE && stageTime >= releaseLength) SetStage(IDLE   );
    
    // Increase elapsed time
    if (stage != IDLE) stageTime++;
    
    // Compute new value
    switch (stage) {
      
      case ATTACK:
        value = Interpolate(stageStartValue, 1.0f, stageTime, attackLength, attackCurve);
        break;
      
      case HOLD:
        value = 1.0f;
        break;
      
      case DECAY:
        value = Interpolate(1.0f, sustainLevel, stageTime, decayLength, decayCurve);
        break;
      
      case SUSTAIN:
        value = sustainLevel;
        break;
      
      case RELEASE:
        value = Interpolate(stageStartValue, 0.0f, stageTime, releaseLength, releaseCurve);
        break;
        
      default:
        value = 0.0f;
        break;
      
    }
    
    return value;
    
  }
  
  void Envelope::SetStage(EnvelopeStage s) {
    
    // Set stage (if different than current) and restart the stage timer
    if (stage != s) {
      stage = s;
      stageTime = 0L;
      stageStartValue = value;
    }
    
  }
  
  void Envelope::SetStageLength(float f, long *field) {
    
    // If factor is above threshold, set the length in time units, according to time scale.
    // Use a curve so smaller values can be dialed in more precisely, despite big time scales.
    if (f >= kMinStageLength) {
      float ff = WarpPhase(f - kMinStageLength, 0.25f);
      *field = std::max(0L, (long)(ff * timeScale));
    } else {
      *field = 0L;
    }
    
  }
  
  void Envelope::SetStageCurve(float f, float *field) {
    
    // Set curve factor
    *field = f;
    
  }
  
  bool Envelope::HasStageLength(long *field) {
    
    // Return if it has a length bigger than zero, used for skipping stages and for slider LEDs
    return *field > 0L;
    
  }
  
  float Envelope::Interpolate(float from, float to, long time, long length, float curve) {
    
    // Interpolate values depending on the amount of time elapsed in respoet to total length.
    // Interpolation is linear for curve = 0.5, ease-in for curve < 0.5, ease-out for curve > 0.5.
    float t = WarpPhase((float)time / length, curve);
    return from + (to - from) * t;
    
  }
  
  float Envelope::WarpPhase(float t, float curve) {
    
    // Curve generator for interpolation, copied from SegmentGenerator::WarpPhase
    curve -= 0.5f;
    const bool flip = curve < 0.0f;
    if (flip) t = 1.0f - t;
    const float a = 128.0f * curve * curve;
    t = (1.0f + a) * t / (1.0f + a * t);
    if (flip) t = 1.0f - t;
    return t;
    
  }
  
}
