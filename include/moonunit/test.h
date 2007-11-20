#ifndef __MU_TEST_H__
#define __MU_TEST_H__

#define MU_TEST_PREFIX "__mu_t_"
#define MU_FUNC_PREFIX "__mu_f_"
#define MU_FS_PREFIX "__mu_fs_"
#define MU_FT_PREFIX "__mu_ft_"

struct MoonUnitHarness;
struct MoonUnitLoader;
struct MoonUnitLibrary;

typedef struct MoonUnitTest
{
    // Test suite name
    const char* suite;
    // Test name
    const char* name;
    // Source file where test is located
    const char* file;
    // First line of test definition
    unsigned int line;
    // Test function
    void (*function) (struct MoonUnitTest*);
    // Loader that loaded this test and library
    // (filled in by loader)
    struct MoonUnitLoader* loader;
    struct MoonUnitLibrary* library;
    // Harness running this test
    // (filled in by harness)
    struct MoonUnitHarness* harness;
    void* data;
} MoonUnitTest;



#define MU_TEST(suite, name)                                           \
    __attribute__((used)) void __mu_f_##suite##_##name(MoonUnitTest*); \
    MoonUnitTest __mu_t_##suite##_##name =                             \
    {                                                                  \
        #suite,                                                        \
        #name,                                                         \
        __FILE__,                                                      \
        __LINE__,                                                      \
        __mu_f_##suite##_##name,                                       \
        0, 0, 0                                                        \
    };                                                                 \
    void __mu_f_##suite##_##name(MoonUnitTest* __mu_self__)            \
        
#define MU_LIBRARY_SETUP                        \
    void __mu_ls()                              \
        
#define MU_LIBRARY_TEARDOWN                     \
    void __mu_lt()                              \
        
#define MU_FIXTURE_SETUP(name)                     \
    void __mu_fs_##name(MoonUnitTest* __mu_self__) \
        
#define MU_FIXTURE_TEARDOWN(name)                  \
    void __mu_ft_##name(MoonUnitTest* __mu_self__) \

#define MU_ASSERT(expr)                                         \
    __mu_assert(__mu_self__, expr, #expr, __FILE__, __LINE__)   \

#define MU_ASSERT_EQUAL(type, expr, expected) \
	__mu_assert_equal(__mu_self__, #expr, #expected, __FILE__, __LINE__, type, (expr), (expected))

#define MU_INTEGER "%i"
#define MU_STRING "%s"
#define MU_FLOAT "%f"

#define MU_SUCCESS                              \
    __mu_success(__mu_self__)
    
#define MU_FAILURE(...)                                                 \
    __mu_failure(__mu_self__, __FILE__, __LINE__, format, __VA_ARGS__)  \

#define MU_CURRENT_TEST (__mu_self__)

void __mu_assert(MoonUnitTest* test, int result, const char* expr, const char* file, unsigned int line);
void __mu_assert_equal(MoonUnitTest* test, const char* expr, const char* expected, const char* file, unsigned int line, const char* type, ...);
void __mu_success(MoonUnitTest* test);
void __mu_failure(MoonUnitTest* test, const char* file, unsigned int line, const char* message, ...);

#endif
