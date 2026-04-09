#ifndef STAGES_TEST_UNIT_TEST_H_
#define STAGES_TEST_UNIT_TEST_H_

#include <cmath>
#include <cstdio>

#define UT_TEST(name) int test_##name()
#define UT_ASSERT_TRUE(x) \
  if (!(x)) { \
    printf("FAIL: %s\n", #x); \
    return 1; \
  }
#define UT_ASSERT_EQ(a, b) \
  if ((a) != (b)) { \
    printf("FAIL: %s != %s (%ld != %ld)\n", #a, #b, (long)(a), (long)(b)); \
    return 1; \
  }
#define UT_ASSERT_FLOAT_EQ(a, b, eps) \
  if (fabsf((a) - (b)) > (eps)) { \
    printf("FAIL: %s ~= %s (%f != %f)\n", #a, #b, (float)(a), (float)(b)); \
    return 1; \
  }
#define UT_RUN_TEST(name) \
  do { \
    printf("Running %s...\n", #name); \
    if (test_##name()) { \
      printf("FAIL: %s\n", #name); \
    } else { \
      printf("PASSED\n"); \
    } \
  } while (0)

#endif
