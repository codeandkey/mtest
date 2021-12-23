#include "mtest.h"

#include <cmath>

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

TEST(IsPrimeTest) {
  EXPECT(is_prime(3));
  EXPECT(is_prime(5));
  EXPECT(is_prime(7));
  EXPECT(is_prime(4392));
  EXPECT(is_prime(4002679));
  EXPECT(is_prime(40000003));
}

TEST(BasicTest) {
  EXPECT(100);
  EXPECT(200);
  EXPECT(0);
  ASSERT(0);  // test stops here
  EXPECT(20); // unreachable
}

TEST(OkTest) {
  EXPECT(1);
}

int main(int argc, char **argv) { return mtest_main(argc, argv); }
