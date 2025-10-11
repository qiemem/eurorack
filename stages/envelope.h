#ifndef STAGES_6EG_ENVELOPE_H_
#define STAGES_6EG_ENVELOPE_H_

namespace stages {

enum EnvelopeStage {
  IDLE,
  DELAY,
  ATTACK,
  HOLD,
  DECAY,
  SUSTAIN,
  RELEASE
};

class Envelope {
  
  public:
    
    Envelope() { }
    ~Envelope() { }
    
    void Init();
    
    inline void SetDelayLength  (float f) { SetStageLength(f, &delayLength  ); };
    inline void SetAttackLength (float f) { SetStageLength(f, &attackLength ); };
    inline void SetHoldLength   (float f) { SetStageLength(f, &holdLength   ); };
    inline void SetDecayLength  (float f) { SetStageLength(f, &decayLength  ); };
    inline void SetSustainLevel (float f) { sustainLevel = f - 0.001f;         };
    inline void SetReleaseLength(float f) { SetStageLength(f, &releaseLength); };
    
    inline void SetAttackCurve (float f) { SetStageCurve(f, &attackCurve);  };
    inline void SetDecayCurve  (float f) { SetStageCurve(f, &decayCurve);   };
    inline void SetReleaseCurve(float f) { SetStageCurve(f, &releaseCurve); };
    
    inline bool HasDelay  () { return HasStageLength(&delayLength  ); };
    inline bool HasAttack () { return HasStageLength(&attackLength ); };
    inline bool HasHold   () { return HasStageLength(&holdLength   ); };
    inline bool HasDecay  () { return HasStageLength(&decayLength  ); };
    inline bool HasSustain() { return sustainLevel > 0.001f;          };
    inline bool HasRelease() { return HasStageLength(&releaseLength); };
    
    inline EnvelopeStage CurrentStage() { return stage; }
    
    void Gate(bool high);
    
    float Value();
	
  private:
    
    EnvelopeStage stage;
    long stageTime;
    float stageStartValue;
    
    long delayLength;
    long attackLength;
    long holdLength;
    long decayLength;
    float sustainLevel;
    long releaseLength;
    
    float attackCurve;
    float decayCurve;
    float releaseCurve;
    
    bool gate;
    float value;
  
    void SetStage(EnvelopeStage stage);
    void SetStageLength(float f, long *field);
    void SetStageCurve(float f, float *field);
    bool HasStageLength(long *field);
    
    float Interpolate(float from, float to, long time, long length, float curve = 0.5f);
};

}

#endif  // STAGES_6EG_ENVELOPE_H_
