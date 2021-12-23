#include "mtest.h"

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

  EXPECT(100);
  EXPECT(200);
  EXPECT(0);
  ASSERT(0);  // test stops here
  EXPECT(20); // unreachable
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

// To run your tests, call mtest_main() with the command-line arguments.
// The value returned from mtest_main() should be used as the exit code.
int main(int argc, char **argv) { return mtest_main(argc, argv); }
