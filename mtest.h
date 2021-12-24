// SPDX-License-Identifier: MIT
// vim: set ts=2 sw=2:

#ifndef MTEST_H
#define MTEST_H

#include <stdarg.h>

typedef struct {
  void (*testfun)(void *self);
  const char *tname;
  char **failures;
  int num_failures;
} _mtest_t;

#define MT_STRINGIFY2(x) #x
#define MT_STRINGIFY(x) MT_STRINGIFY2(x)

/**
 * Defines a test. Tests should be defined as functions, for example
 *
 * TEST(MyTestName) {
 *   // Test logic here.
 *   // EXPECT() and ASSERT() to express test conditions.
 * }
 *
 * Tests should be registered once in source files (NOT headers). Test names
 * must be unique -- otherwise a 'multiple definition' error will occur during
 * compilation.
 *
 * @param name Test name token.
 */
#define TEST(name)                                                             \
  void _test_##name(void *s);                                                  \
  static _mtest_t _test_s##name = {.testfun = _test_##name,                    \
                                   .tname = #name,                             \
                                   .failures = (char **)0,                     \
                                   .num_failures = 0};                         \
  static int _test_add_##name = _mtest_add(&_test_s##name);                    \
  void _test_##name(void *__self)

/**
 * Tests that a condition is true. If the condition does not evaluate to a
 * nonzero value, the test is considered failed and this macro is reported.
 * The test will continue on regardless if this condition passes or fails.
 * 
 * @param cond Condition to test.
 */
#define EXPECT(cond)                                                           \
  {                                                                            \
    if (!(cond)) {                                                             \
      _mtest_fail(__self, "[" __FILE__ ":" MT_STRINGIFY(                       \
                              __LINE__) "] failed expectation \"" #cond "\""); \
    }                                                                          \
  }

/**
 * Tests that a condition is true. If the condition does not evaluate to a
 * nonzero value, the test is considered failed and this macro is reported.
 * The test will terminate immediately if this condition fails.
 * 
 * @param cond Condition to test.
 */
#define ASSERT(cond)                                                           \
  {                                                                            \
    if (!(cond)) {                                                             \
      _mtest_fail(__self,                                                      \
                  "[" __FILE__                                                 \
                  ":" MT_STRINGIFY(__LINE__) "] failed assertion \"" #cond     \
                                          "\", aborting test");                \
      return;                                                                  \
    }                                                                          \
  }

/**
 * Runs all tests registered with TEST(). Returns 0 if all tests pass,
 * or -1 if one or more tests failed.
 *
 * @param argc Argc from main().
 * @param argv Argv from main().
 * @return Exit code.
 */
int mtest_main(int argc, char **argv);

int _mtest_add(_mtest_t *tstruct);
void _mtest_fail(void *self, const char *fmt, ...);

#endif
