#ifndef __MU_TEST_H__
#define __MU_TEST_H__

#define MU_TEST_PREFIX "__mu_t_"
#define MU_FUNC_PREFIX "__mu_f_"
#define MU_FS_PREFIX "__mu_fs_"
#define MU_FT_PREFIX "__mu_ft_"

struct MoonUnitHarness;
struct MoonUnitLoader;
struct MoonUnitLibrary;
struct MoonUnitTestMethods;

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
    
    // Members filled in before running a test
    
    // Loader that loaded this test and library
    // (filled in by loader)
    struct MoonUnitLoader* loader;
    struct MoonUnitLibrary* library;
    
    // Harness running this test
    // (filled in by harness)
    struct MoonUnitHarness* harness;
    
    // Methods (assertions, etc.)
    // (filled in by loader)
    struct MoonUnitTestMethods* methods;
    
    // Custom data
    void* data;
} MoonUnitTest;

typedef struct MoonUnitTestMethods
{
	void (*assert)(MoonUnitTest* test, 
				   int result, const char* expr, const char* file, unsigned int line);
	void (*assert_equal)(MoonUnitTest* test, 
				   		 const char* expr, const char* expected, 
				 		 const char* file, unsigned int line, unsigned int type, ...);
	void (*success)(MoonUnitTest* test);
	void (*failure)(MoonUnitTest* test, 
				    const char* file, unsigned int line, const char* message, ...);
} MoonUnitTestMethods;

#ifdef __GNUC__
#    define __MU_USED__ __attribute__((used))
#    define __MU_SECTION__(name) __attribute__((section(name)))
#    define __MU_HIDDEN__ __attribute__((visibility("hidden")))
#else
#    define __MU_USED__
#    define __MU_SECTION__(name)
#    define __MU_HIDDEN__
#endif
#define __MU_SECTION_TEXT__ __MU_SECTION__(".moonunit_text")
#define __MU_SECTION_DATA__ __MU_SECTION__(".moonunit_data")

#ifdef MU_HIDE_TESTS
#    define __MU_HIDDEN_TEST__ __MU_HIDDEN__
#else
#    define __MU_HIDDEN_TEST__
#endif

#define MU_TEST(suite, name)                                           \
    __MU_SECTION_TEXT__                                                \
    __MU_HIDDEN__                                                      \
    void __mu_f_##suite##_##name(MoonUnitTest*);                       \
    __MU_SECTION_DATA__                                                \
    __MU_HIDDEN_TEST__                                                 \
    __MU_USED__                                                        \
    MoonUnitTest __mu_t_##suite##_##name =                             \
    {                                                                  \
        #suite,                                                        \
        #name,                                                         \
        __FILE__,                                                      \
        __LINE__,                                                      \
        __mu_f_##suite##_##name,                                       \
        0, 0, 0, 0                                                     \
    };                                                                 \
    void __mu_f_##suite##_##name(MoonUnitTest* __mu_self__)            \
        
#define MU_LIBRARY_SETUP                        \
    __MU_USED__                                 \
	__MU_HIDDEN_TEST__                          \
    void __mu_ls()                              \
        
#define MU_LIBRARY_TEARDOWN                     \
    __MU_USED__                                 \
    __MU_HIDDEN_TEST__                          \
    void __mu_lt()                              \
        
#define MU_FIXTURE_SETUP(name)                     \
    __MU_USED__                                    \
    __MU_HIDDEN_TEST__                             \
    void __mu_fs_##name(MoonUnitTest* __mu_self__) \
        
#define MU_FIXTURE_TEARDOWN(name)                  \
    __MU_USED__                                    \
    __MU_HIDDEN_TEST__                             \
    void __mu_ft_##name(MoonUnitTest* __mu_self__) \

#define MU_ASSERT(expr)                                                          \
    __mu_self__->methods->assert(__mu_self__, expr, #expr, __FILE__, __LINE__)   \

#define MU_ASSERT_EQUAL(type, expr, expected) \
	__mu_self__->methods->assert_equal(__mu_self__, #expr, #expected, __FILE__, __LINE__, type, (expr), (expected))

#define MU_INTEGER 0
#define MU_STRING  1
#define MU_FLOAT   2

#define MU_SUCCESS                              \
    __mu_self__->methods->success(__mu_self__)
    
#define MU_FAILURE(...) \
    __mu_self__->methods->failure(__mu_self__, __FILE__, __LINE__, __VA_ARGS__)

#define MU_CURRENT_TEST (__mu_self__)

extern MoonUnitTestMethods Mu_TestMethods;

#endif
