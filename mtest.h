#ifndef MTEST_H
#define MTEST_H

#ifdef __cplusplus
#include <cstdarg>
#else
#include <stdarg.h>
#endif

typedef struct {
    void (*testfun)(void* self);
    const char* tname;
    char** failures;
    int num_failures;
} _mtest_t;

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define TEST(name) \
    void _test_##name(void* s); \
    static _mtest_t _test_s##name = { \
        .testfun = _test_##name, \
        .tname = #name, \
        .failures = (char**) 0, \
        .num_failures = 0 \
    }; \
    static int _test_add_ ##name = _mtest_add(&_test_s##name); \
    void _test_##name(void* __self)

#define EXPECT(cond) \
    { if (!(cond)) { \
        _mtest_fail(__self, "[" __FILE__ ":" STRINGIFY(__LINE__) "] expected \"" #cond "\", to be true, found false"); \
    }} \

#define ASSERT(cond) \
    { if (!(cond)) { \
        _mtest_fail(__self, "[" __FILE__ ":" STRINGIFY(__LINE__) "] asserted \"" #cond "\", to be true, found false"); \
        return; \
    }} \

int mtest_main(int argc, char** argv);

int _mtest_add(_mtest_t* tstruct);
void _mtest_fail(void* self, const char* fmt, ...);

#endif
