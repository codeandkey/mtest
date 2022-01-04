// SPDX-License-Identifier: MIT
// vim: set ts=2 sw=2:

/*
 * =================================================================
 * This file is a part of mtest - a minimal C++ testing framework.
 * The project is available at https://github.com/codeandkey/mtest
 * =================================================================
 */

#ifndef MTEST_H
#define MTEST_H

#include <iostream>

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
  static void _test_##name(void *s);                                           \
  static int _test_add_##name = _mtest_push(#name, &_test_##name);             \
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
      _mtest_fail(__self)                                                      \
        << "[" << __FILE__ << ":" << __LINE__ << "] "                          \
        << "failed expectation \"" << #cond << "\"";                           \
    }                                                                          \
  }

/**
 * Tests that a binary operator between two values returns true.
 * The test will continue on regardless if this condition passes or fails.
 * 
 * @param lhs Left-hand value.
 * @param op  Operator.
 * @param rhs Right-hand value.
 */
#define EXPECT_OP(lhs, op, rhs)                                                \
  {                                                                            \
    if (!((lhs) op (rhs))) {                                                   \
      _mtest_fail(__self)                                                      \
        << __FILE__ << ":" << __LINE__ << " | "                                \
        << "failed expectation \"" << #lhs << " " << #op << " " << #rhs        \
        << "\": \"" << lhs << "\" !" #op << " \"" << rhs << "\"";              \
    }                                                                          \
  }

#define EXPECT_EQ(lhs, rhs) EXPECT_OP(lhs, ==, rhs)
#define EXPECT_NE(lhs, rhs) EXPECT_OP(lhs, !=, rhs)
#define EXPECT_LT(lhs, rhs) EXPECT_OP(lhs, <, rhs)
#define EXPECT_LE(lhs, rhs) EXPECT_OP(lhs, <=, rhs)
#define EXPECT_GT(lhs, rhs) EXPECT_OP(lhs, >, rhs)
#define EXPECT_GE(lhs, rhs) EXPECT_OP(lhs, >=, rhs)

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
      _mtest_fail(__self)                                                      \
        << "[" << __FILE__ << ":" << __LINE__ << "] "                          \
        << "failed assertion \"" << #cond << "\", aborting test";              \
      return;                                                                  \
    }                                                                          \
  }

/**
 * Tests that a binary operator between two values returns true.
 * The test will terminate immediately if this condition fails.
 * 
 * @param lhs Left-hand value.
 * @param op  Operator.
 * @param rhs Right-hand value.
 */
#define ASSERT_OP(lhs, op, rhs)                                                \
  {                                                                            \
    if (!((lhs) op (rhs))) {                                                   \
      _mtest_fail(__self)                                                      \
        << "[" << __FILE__ << ":" << __LINE__ << "] "                          \
        << "failed assertion \"" << #lhs << " " << #op << " " << #rhs          \
        << "\": \"" << lhs << "\" !" #op << " \"" << rhs << "\", "             \
        << "aborting test";                                                    \
      return;                                                                  \
    }                                                                          \
  }

#define ASSERT_EQ(lhs, rhs) ASSERT_OP(lhs, ==, rhs)
#define ASSERT_NE(lhs, rhs) ASSERT_OP(lhs, !=, rhs)
#define ASSERT_LT(lhs, rhs) ASSERT_OP(lhs, <, rhs)
#define ASSERT_LE(lhs, rhs) ASSERT_OP(lhs, <=, rhs)
#define ASSERT_GT(lhs, rhs) ASSERT_OP(lhs, >, rhs)
#define ASSERT_GE(lhs, rhs) ASSERT_OP(lhs, >=, rhs)

/**
 * Runs all tests registered with TEST(). Returns 0 if all tests pass,
 * or -1 if one or more tests failed.
 *
 * @param argc Argc from main().
 * @param argv Argv from main().
 * @return Exit code.
 */
int mtest_main(int argc, char **argv);

int _mtest_push(const char* name, void(*tfun)(void*));
std::ostream& _mtest_fail(void *self);

#endif
