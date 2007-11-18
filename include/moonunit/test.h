#ifndef __MU_TEST_H__
#define __MU_TEST_H__

#define MU_TEST_PREFIX __mu_t_
#define MU_FUNC_PREFIX __mu_f_
#define MU_FS_PREFIX __mu_fs_
#define MU_FT_PREFIX __my_ft_

struct MoonHarness;

typedef struct MoonUnitTest
{
    // File name of library test is in
    // (filled in by harness)
    const char* library;
    // Test suite name
    const char* suite;
    // Test name
    const char* name;
    // Source file where test is located
    const char* file;
    // First line of test definition
    unsigned int line;
    // Test function
    void (*function) (void);
    // Harness responsible for this test
    // (filled in by harness)
    struct MoonHarness* harness;
} MoonUnitTest;



#define MU_TEST(suite, name)                                        \
    void __mu_f_##suite##_##name(MoonUnitTest*);                    \
    GibUnitTest __mu_t_##suite##_##name =                           \
    {                                                               \
        NULL,                                                       \
        #suite,                                                     \
        #name,                                                      \
        __FILE__,                                                   \
        __LINE__,                                                   \
        __mu_f_##suite##_##name                                     \
    };                                                              \
    static void __mu_f_##suite##_##name(MoonUnitTest* __mu_self__)  \
        
#define MU_LIBRARY_SETUP                        \
    void __mu_ls()                              \
        
#define MU_LIBRARY_TEARDOWN                     \
    void __mu_lt()                              \
        
#define MU_FIXTURE_SETUP(name)                  \
    void __mu_fs_##name()                       \
        
#define MU_FIXTURE_TEARDOWN(name)               \
    void __mu_ft_##name()                       \

#define MU_ASSERT(expr)                                         \
    __mu_assert(__mu_self__, expr, #expr, __FILE__, __LINE__)   \

#define MU_SUCCESS                              \
    __mu_success()
    
#define MU_FAILURE(...)                                                 \
    __mu_failure(__mu_self__, __FILE__, __LINE__, format, __VA_ARGS__)  \

#endif
