#include "harness/unity.h"

/* Raise this when you start a new milestone: 0 for G0, 1 for G1, and so on. The
   smoke tests for later milestones can't pass yet, so they stay off until then. */
#define MILESTONE 0

void run_smoke_tests(int milestone);

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();
  run_smoke_tests(MILESTONE);
  // Call your own run_*_tests() functions here.
  return UNITY_END();
}
