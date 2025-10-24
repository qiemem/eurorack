#ifndef STAGES_6EG_ENVELOPE_H_
#define STAGES_6EG_ENVELOPE_H_

#include <inttypes.h>

namespace stages {

enum EnvelopeStage {
  IDLE = 0,
  DELAY = 1,
  ATTACK = 2,
  HOLD = 3,
  DECAY = 4,
  SUSTAIN = 5,
  RELEASE = 6
};

class Envelope {
  
  public:
    
    Envelope() { }
    ~Envelope() { }
    
    void Init();
    
    inline void SetDelayLength  (float f) { SetStageLength(f, DELAY  ); };
    inline void SetAttackLength (float f) { SetStageLength(f, ATTACK ); };
    inline void SetHoldLength   (float f) { SetStageLength(f, HOLD   ); };
    inline void SetDecayLength  (float f) { SetStageLength(f, DECAY  ); };
    inline void SetSustainLevel (float f) { sustainLevel = f - 0.001f;  };
    // Sets minimum sustain time
    inline void SetSustainLength(float f) { SetStageLength(f, SUSTAIN); };
    inline void SetReleaseLength(float f) { SetStageLength(f, RELEASE); };
    
    inline void SetAttackCurve (float f) { SetStageCurve(f, &attackCurve);  };
    inline void SetDecayCurve  (float f) { SetStageCurve(f, &decayCurve);   };
    inline void SetReleaseCurve(float f) { SetStageCurve(f, &releaseCurve); };

    inline void SetLooping(bool l) { loop = l; }

    inline bool HasDelay  () { return HasStage(DELAY  ); };
    inline bool HasAttack () { return HasStage(ATTACK ); };
    inline bool HasHold   () { return HasStage(HOLD   ); };
    inline bool HasDecay  () { return HasStage(DECAY  ); };
    inline bool HasSustain() { return sustainLevel > 0.001f; };
    inline bool HasMinSustain() { return HasStage(SUSTAIN); };
    inline bool HasRelease() { return HasStage(RELEASE); };
    inline bool IsLooping() { return loop; }
    
    inline EnvelopeStage CurrentStage() { return stage; }
    
    float Value(bool gate_high);

  private:
    
    EnvelopeStage stage;
    float phase;
    float stageStartValue;
    
    float phaseIncrement[7];  // indexed by EnvelopeStage enum: IDLE, DELAY, ATTACK, HOLD, DECAY, SUSTAIN, RELEASE
    float sustainLevel;
    
    float attackCurve;
    float decayCurve;
    float releaseCurve;
    
    bool gate;
    bool loop;
    float value;
  
    void SetStage(EnvelopeStage stage);
    void SetStageLength(float f, EnvelopeStage stage);
    void SetStageCurve(float f, float *field);
    bool HasStage(EnvelopeStage stage);
    
    float Interpolate(float from, float to, float phase, float curve = 0.5f);
};

}

#endif  // STAGES_6EG_ENVELOPE_H_
