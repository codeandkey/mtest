#include "../../mtest.h"

#include <math.h>

/**
 * Primality tester - we will test this function.
 */
int is_prime(int n) {
  if (n < 2)
    return 0;
  if (n < 4)
    return 1;
  if (!(n & 1))
    return 0;

  for (int i = 3; i <= sqrt(n); i += 2) {
    if (!(n % i))
      return 0;
  }

  return 1;
}

// Use TEST(MyTestName) to define tests, just like functions.
// Tests do not have a return value, however you can still 'return'
// to abort a test.

TEST(IsPrimeTest) {
  // EXPECT() allows us to test a condition which _should_ be true;
  // if it is not, we note the failure but continue on in the test.

  EXPECT(is_prime(3));
  EXPECT(is_prime(5));
  EXPECT(is_prime(7));
  EXPECT(is_prime(4392));     // this number is not prime, but
  EXPECT(is_prime(4002679));  // <- these checks
  EXPECT(is_prime(40000003)); // <- are still executed
}

TEST(BasicTest) {
  // ASSERT() allows us to test a condition, but if the assertion
  // fails the test is immediately aborted.

  EXPECT_EQ(100, 200);
  EXPECT(200);
  EXPECT(0);
  ASSERT_NE(0, 10 - 10);  // test stops here
  EXPECT(20); // unreachable
}

TEST(ComparisonTest) {
    EXPECT_EQ(5, 52);  // equality
    EXPECT_NE(10, 10); // non-equality
    EXPECT_LT(5, 5);   // less than
    EXPECT_LE(6, 5);   // less than (or equal)
    EXPECT_GT(0, 10);  // greater than
    EXPECT_GE(10, 50); // greater than (or equal)

    EXPECT_OP(5, != , 5);
    EXPECT_OP(5, <, 5);
}

// Test which always passes
TEST(OkTest) { EXPECT(1); }

// Some tests which take longer
TEST(LongTest1) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest2) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest3) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest4) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest5) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest6) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest7) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}

TEST(LongTest8) {
  for (int j = 0; j < 100000000; ++j) {
    j += 1;
    j -= 1;
  }
}
