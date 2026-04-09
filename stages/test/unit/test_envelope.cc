#include "stages/envelope.h"
#include "stages/test/unit/unit_test.h"
#include <cstdlib>

using namespace stages;

UT_TEST(TestDefaultStateAfterInit) {
  Envelope env;
  env.Init();
  UT_ASSERT_EQ(env.CurrentStage(), IDLE);
  UT_ASSERT_FLOAT_EQ(env.Value(false), 0.0f, 0.001f);
  UT_ASSERT_TRUE(!env.IsLooping());
  return 0;
}

UT_TEST(TestOutputStaysInValidRange) {
  Envelope env;
  env.Init();

  env.SetDelayLength(0.05f);
  env.SetAttackLength(0.1f);
  env.SetHoldLength(0.05f);
  env.SetDecayLength(0.1f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.05f);
  env.SetReleaseLength(0.05f);

  UT_ASSERT_EQ(env.Value(true), 0.0f);
  UT_ASSERT_EQ(env.CurrentStage(), DELAY);

  while (env.CurrentStage() != IDLE) {
    float v = env.Value(env.CurrentStage()<SUSTAIN);
    UT_ASSERT_TRUE(v >= 0.0f && v <= 1.0f);
  }
  return 0;
}

UT_TEST(TestGateHighTriggersAttack) {
  Envelope env;
  env.Init();

  env.SetAttackLength(0.1f);

  UT_ASSERT_EQ(env.CurrentStage(), IDLE);

  env.Value(true);

  UT_ASSERT_EQ(env.CurrentStage(), ATTACK);
  return 0;
}

UT_TEST(TestLoopingWrapsAround) {
  Envelope env;
  env.Init();

  env.SetDelayLength(0.05f);
  env.SetAttackLength(0.05f);
  env.SetHoldLength(0.05f);
  env.SetDecayLength(0.05f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.05f);
  env.SetLooping(true);

  int transitions = 0;
  while (transitions < 2) {
    if (env.CurrentStage() == ATTACK) {
      transitions++;
    }
    env.Value(false);
  }

  UT_ASSERT_TRUE(transitions >= 2);
  return 0;
}

UT_TEST(TestCurveSettingsAffectInterpolation) {
  for (float curve1 = 0.0f; curve1 <= 0.9f; curve1 += 0.1f) {
    float curve2 = curve1 + 0.1f;
    Envelope env1, env2;
    env1.Init();
    env2.Init();
    env1.SetAttackCurve(curve1);
    env2.SetAttackCurve(curve2);
    env1.SetAttackLength(0.1f);
    env2.SetAttackLength(0.1f);

    while (env1.CurrentStage() == ATTACK && env2.CurrentStage() == ATTACK) {
      float v1 = env1.Value(true);
      float v2 = env2.Value(true);
      UT_ASSERT_TRUE(v1 >= 0.0f && v1 <= 1.0f);
      UT_ASSERT_TRUE(v2 >= 0.0f && v2 <= 1.0f);
      UT_ASSERT_TRUE(v2 > v1);
    }
  }
  return 0;
}

UT_TEST(TestStagesWithZeroLengthAreSkipped) {
  Envelope env;
  env.Init();

  env.SetAttackLength(0.0f);
  env.SetDecayLength(0.0f);
  env.SetSustainLevel(0.5f);
  env.SetReleaseLength(0.1f);

  UT_ASSERT_TRUE(!env.HasAttack());
  UT_ASSERT_TRUE(!env.HasDecay());

  env.SetAttackLength(0.1f);
  UT_ASSERT_TRUE(env.HasAttack());

  env.Value(true);
  while (env.CurrentStage() != IDLE) {
    env.Value(false);
  }

  UT_ASSERT_TRUE(env.CurrentStage() == IDLE);
  return 0;
}

UT_TEST(TestStageTransitions) {
  Envelope env;
  env.Init();

  env.SetDelayLength(0.05f);
  env.SetAttackLength(0.05f);
  env.SetHoldLength(0.05f);
  env.SetDecayLength(0.05f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.05f);
  env.SetReleaseLength(0.05f);

  UT_ASSERT_EQ(env.CurrentStage(), IDLE);
  while (env.CurrentStage() == IDLE) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), DELAY);

  while (env.CurrentStage() == DELAY) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), ATTACK);

  while (env.CurrentStage() == ATTACK) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), HOLD);

  while (env.CurrentStage() == HOLD) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), DECAY);

  while (env.CurrentStage() == DECAY) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), SUSTAIN);

  while (env.CurrentStage() == SUSTAIN) env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);

  while (env.CurrentStage() == RELEASE) env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), IDLE);

  return 0;
}

UT_TEST(TestAttackAndDecayTiming) {
  Envelope env;
  env.Init();

  env.SetAttackLength(0.1f);
  env.SetDecayLength(0.1f);
  UT_ASSERT_TRUE(env.HasAttack());
  UT_ASSERT_TRUE(env.HasDecay());

  int attack_samples = 0;
  env.Value(true);
  while (env.CurrentStage() == ATTACK) {
    attack_samples++;
    env.Value(true);
  }
  UT_ASSERT_TRUE(attack_samples > 100 && attack_samples < 300);

  int decay_samples = 0;
  while (env.CurrentStage() == DECAY) {
    decay_samples++;
    env.Value(true);
  }
  UT_ASSERT_TRUE(decay_samples > 100);
  return 0;
}

UT_TEST(TestSustainWithGateHighStaysInSustain) {
  Envelope env;
  env.Init();

  env.SetAttackLength(0.1f);
  env.SetDecayLength(0.1f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.1f);
  env.SetReleaseLength(0.1f);

  env.Value(true);

  while (env.CurrentStage() == ATTACK) env.Value(true);
  while (env.CurrentStage() == DECAY) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), SUSTAIN);

  // Without gate, should stay for sustain length
  int sustain_samples = 0;
  while (env.CurrentStage() == SUSTAIN) {
    sustain_samples++;
    env.Value(false);
  }
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);
  UT_ASSERT_TRUE(sustain_samples > 100 && sustain_samples < 300);

  // It should stay longer than that with the gate
  env.Value(true);
  while (env.CurrentStage() != SUSTAIN) env.Value(true);
  for (int i = 0; i < sustain_samples * 2; i++) env.Value(true);
  UT_ASSERT_EQ(env.CurrentStage(), SUSTAIN);
  env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);

  // Without sustain length, it release as soon as the gate drops
  env.SetSustainLength(0.0f);
  env.Value(true);
  while (env.CurrentStage() != SUSTAIN) env.Value(true);
  env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);

  return 0;
}

UT_TEST(TestGateLowExitsAllStages) {
  Envelope env;
  env.Init();
  env.SetDelayLength(0.05f);
  env.SetAttackLength(0.05f);
  env.SetHoldLength(0.05f);
  env.SetDecayLength(0.05f);
  env.SetReleaseLength(0.05);

  // From ATTACK → RELEASE (don't set delay - it keeps us stuck in DELAY!)
  while (env.CurrentStage() != ATTACK) env.Value(true);
  env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);

  // From HOLD → RELEASE
  while (env.CurrentStage() != HOLD) env.Value(true);
  env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);

  // From DECAY → RELEASE
  while (env.CurrentStage() != DECAY) env.Value(true);
  env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);

  // From SUSTAIN → RELEASE
  while (env.CurrentStage() != SUSTAIN) env.Value(true);
  env.Value(false);
  UT_ASSERT_EQ(env.CurrentStage(), RELEASE);
  return 0;
}

UT_TEST(TestLoopingUsesSustainLength) {
  Envelope env;
  env.Init();

  env.SetAttackLength(0.05f);
  env.SetDecayLength(0.05f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.1f);
  env.SetLooping(true);

  env.Value(true);

  while (env.CurrentStage() == ATTACK) {
    env.Value(true);
  }
  while (env.CurrentStage() == DECAY) {
    env.Value(true);
  }

  int sustain_samples = 0;
  while (env.CurrentStage() == SUSTAIN && sustain_samples < 10000) {
    env.Value(true);
    sustain_samples++;
  }

  UT_ASSERT_TRUE(sustain_samples > 100);
  return 0;
}

UT_TEST(TestLoopingUsesAllStages) {
  Envelope env;
  env.Init();

  env.SetDelayLength(0.05f);
  env.SetAttackLength(0.05f);
  env.SetHoldLength(0.05f);
  env.SetDecayLength(0.05f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.05f);
  env.SetReleaseLength(0.05f);
  env.SetLooping(true);


  int seen [static_cast<int>(RELEASE) + 1] = {0};
  stages::EnvelopeStage cur_stage = env.CurrentStage();

  while (seen[static_cast<int>(RELEASE)] < 5) {
    cur_stage = env.CurrentStage();
    seen[static_cast<int>(cur_stage)]++;
    while(env.CurrentStage() == cur_stage) env.Value(false);
  }

  for (int i = 1; i <= RELEASE; i++) {
    UT_ASSERT_EQ(seen[i], 5);
    seen[i] = 0;
  }

  // Should still loop with gate
  env.Value(true); // first one resets
  while (seen[static_cast<int>(RELEASE)] < 5) {
    cur_stage = env.CurrentStage();
    seen[static_cast<int>(cur_stage)]++;
    while(env.CurrentStage() == cur_stage) env.Value(true);
  }

  for (int i = 1; i <= RELEASE; i++) {
    UT_ASSERT_EQ(seen[i], 5);
    seen[i] = 0;
  }

  return 0;
}

UT_TEST(TestLoopingGateRestart) {
  Envelope env;
  env.Init();

  env.SetAttackLength(0.1f);
  env.SetDecayLength(0.1f);
  env.SetSustainLevel(0.5f);
  env.SetSustainLength(0.05f);
  env.SetLooping(true);

  env.Value(true);

  int in_attack = 0, in_decay = 0, in_sustain = 0;
  while (env.CurrentStage() == ATTACK) {
    in_attack++;
    env.Value(true);
  }
  while (env.CurrentStage() == DECAY) {
    in_decay++;
    env.Value(true);
  }
  while (env.CurrentStage() == SUSTAIN) {
    in_sustain++;
    env.Value(true);
  }

  UT_ASSERT_TRUE(in_attack > 0);
  UT_ASSERT_TRUE(in_decay > 0);
  UT_ASSERT_TRUE(in_sustain > 0);

  env.Value(false);

  while (env.CurrentStage() != IDLE) {
    env.Value(false);
  }

  UT_ASSERT_EQ(env.CurrentStage(), IDLE);

  env.Value(true);

  UT_ASSERT_TRUE(env.CurrentStage() == DELAY || env.CurrentStage() == ATTACK);
  return 0;
}

int main() {
  UT_RUN_TEST(TestDefaultStateAfterInit);
  UT_RUN_TEST(TestOutputStaysInValidRange);
  UT_RUN_TEST(TestGateHighTriggersAttack);
  UT_RUN_TEST(TestLoopingWrapsAround);
  UT_RUN_TEST(TestCurveSettingsAffectInterpolation);
  UT_RUN_TEST(TestStagesWithZeroLengthAreSkipped);
  UT_RUN_TEST(TestStageTransitions);
  UT_RUN_TEST(TestAttackAndDecayTiming);
  UT_RUN_TEST(TestSustainWithGateHighStaysInSustain);
  UT_RUN_TEST(TestGateLowExitsAllStages);
  UT_RUN_TEST(TestLoopingUsesSustainLength);
  UT_RUN_TEST(TestLoopingUsesAllStages);
  UT_RUN_TEST(TestLoopingGateRestart);

  return 0;
}
