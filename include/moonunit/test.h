/*
 * Copyright (c) 2007, Brian Koropoff
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Moonunit project nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file test.h
 * @brief Moonunit test structures, constants, and macros
 */

/**
 * @defgroup test Unit Tests
 * @brief Macros, structures, and constants to define and
 * inspect unit tests
 *
 * This module contains the essential ingredients to add unit tests
 * to your code which Moonunit can discover and run.
 */
/*@{*/

#ifndef __MU_TEST_H__
#define __MU_TEST_H__

#ifndef DOXYGEN

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

#endif

/**
 * @defgroup test_def Definition
 * @brief Macros to define unit tests
 * @ingroup test
 *
 * This module contains macros to define unit tests as well as
 * library and fixture setup and teardown routines.
 */
/*@{*/

/**
 * @brief Defines a unit test
 *
 * This macro expands to the definition of several global
 * structures and functions which can be detected and
 * extracted by the Moonunit test loader; it must be
 * followed by the test body enclosed in curly braces.
 * It should only be used at the top level of a C file.
 * In order to preserve expected symbol names, C++ code
 * must wrap all unit test definitions with extern "C" { ... }
 *
 * <b>Example:</b>
 * @code
 * MU_TEST(Arithmetic, add)
 * {
 *     MU_ASSERT(1 + 1 == 2);
 * }
 * @endcode
 *
 * @param suite the (unquoted) name of the test suite which
 * this test should be part of
 * @param name the (unquoted) name of this test
 */
#ifdef DOXYGEN
#define MU_TEST(suite, name)
#else
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
    void __mu_f_##suite##_##name(MoonUnitTest* __mu_self__)
#endif

/**
 * @brief Define library setup routine
 * 
 * Defines an (optional) setup routine which will be
 * executed exactly once by the test harness when
 * this library is loaded.  It should be followed
 * by a curly brace-enclosed code block which performs
 * any necessary global initialization.  Only one
 * instance of this macro should appear in a given
 * library.
 *
 * <b>Example:</b>
 * @code
 * MU_LIBRARY_SETUP
 * {
 *     Library_Init();
 * }
 * @endcode
 */
#ifdef DOXYGEN
#define MU_LIBRARY_SETUP
#else
#define MU_LIBRARY_SETUP                        \
    __MU_USED__                                 \
	__MU_HIDDEN_TEST__                          \
    void __mu_ls()
#endif
        
/**
 * @brief Define library teardown routine
 *
 * Defines an (optional) teardown routine which will be
 * executed exactly once by the test harness when
 * this library is unloaded.  It should be followed
 * by a curly brace-enclosed code block which performs
 * any necessary cleanup of allocated memory or other
 * resources.  Only one instance of this macro should
 * appear in a given library.
 *
 * <b>Example:</b>
 * @code
 * MU_LIBRARY_TEARDOWN
 * {
 *     Library_Free_Resources();
 * }
 * @endcode
 */
#ifdef DOXYGEN
#define MU_LIBRARY_TEARDOWN
#else
#define MU_LIBRARY_TEARDOWN                     \
    __MU_USED__                                 \
    __MU_HIDDEN_TEST__                          \
    void __mu_lt()
#endif

/**
 * @brief Define test fixture setup routine
 * 
 * Defines the setup routine for a test fixture --
 * an environment common to all tests in a particular
 * suite.  This routine will be run immediately before
 * each test in the given suite.  All macros available
 * during the body of a test are also available during
 * the setup routine.  The setup routine may signal
 * success or failure before the test itself
 * is executed; in this case, the test itself will
 * not be run.
 *
 * This macro should be followed by the body of the
 * setup routine enclosed in curly braces.
 *
 * <b>Example:</b>
 * @code
 * static int x;
 * static int y;
 *
 * MU_FIXTURE_SETUP(Arithmetic)
 * {
 *     x = 2;
 *     y = 3;
 * }
 * @endcode
 *
 * @param name the (unquoted) name of the test suite for
 * which the setup routine is being defined
 */
#ifdef DOXYGEN
#define MU_FIXTURE_SETUP(name)
#else
#define MU_FIXTURE_SETUP(name)                     \
    __MU_USED__                                    \
    __MU_HIDDEN_TEST__                             \
    void __mu_fs_##name(MoonUnitTest* __mu_self__)
#endif

/**
 * @brief Define test fixture teardown routine
 * 
 * Defines the teardown routine for a test fixture --
 * an environment common to all tests in a particular
 * suite.  This routine will be run immediately after
 * each test in the given suite.  All macros available
 * during the body of a test are also available during
 * the teardown routine.  A teardown routine may
 * signal failure, causing the test to reported as 
 * failing even if the test itself did not.  This
 * may be used to enforce a common postcondition for
 * a suite of tests.  However, the primary purpose is
 * to free any resources allocated by the matching
 * fixture setup routine.
 *
 * This macro should be followed by the body of the
 * setup routine enclosed in curly braces.
 *
 * <i>Note</i>: If tests are run in a separate process,
 * explicit deallocation of resources is not necessary.
 *
 * <b>Example:</b>
 * @code
 *
 * static void* data;
 *
 * MU_FIXTURE_TEARDOWN(SomeSuite)
 * {
 *     MU_ASSERT(data != NULL);
 *     free(data);
 * }
 * @endcode
 *
 * @param name the (unquoted) name of the test suite for
 * which the setup routine is being defined
 */
#ifdef DOXYGEN
#define MU_FIXTURE_TEARDOWN(name)
#else
#define MU_FIXTURE_TEARDOWN(name)                  \
    __MU_USED__                                    \
    __MU_HIDDEN_TEST__                             \
    void __mu_ft_##name(MoonUnitTest* __mu_self__)
#endif

/*@}*/

/**
 * @defgroup test_result Testing
 * @ingroup test
 * @brief Macros to test assertions and flag failures
 *
 * This module contains macros for use in the body of unit tests
 * to make assertions, raise errors, and report unexpected results.
 */
/*@{*/

/**
 * @brief Confirm truth of predicate or fail
 *
 * This macro asserts that an arbitrary boolean expression
 * is true.  If the assertion succeeds, execution of the
 * current test will continue as usual.  If it fails,
 * the test will immediately fail and be terminated; the
 * expression, filename, and line number will be reported
 * as part of the cause of failure.
 *
 * This macro may only be used in the body of a test,
 * fixture setup, or fixture teardown routine.
 *
 * <b>Example:</b>
 * @code
 * MU_ASSERT(foo != NULL && bar > baz);
 * @endcode
 *
 * @param expr the expression to test
 */
#ifdef DOXYGEN
#define MU_ASSERT(expr)
#else
#define MU_ASSERT(expr)                                                          \
    __mu_self__->methods->assert(__mu_self__, expr, #expr, __FILE__, __LINE__)
#endif

/**
 * @brief Confirm equality of values or fail
 *
 * This macro asserts that the values of two expressions
 * are equal.  If the assertion succeeds, execution of the
 * current test will continue as usual.  If it fails,
 * the test will immediately fail and be terminated; the
 * expressions, their values, and the source filename and 
 * line number will be reported as part of the cause of
 * failure.
 *
 * The type of the two expressions must be specified as
 * the first argument of this macro.  The expressions
 * must have the same type. The following are legal
 * values of the type argument:
 * <ul>
 * <li>MU_INTEGER<li>
 * <li>MU_FLOAT</li>
 * <li>MU_STRING</li>
 * </ul>
 *
 * This macro may only be used in the body of a test,
 * fixture setup, or fixture teardown routine.
 *
 * <b>Example:</b>
 * @code
 * MU_ASSERT_EQUAL(MU_INTEGER, 2 * 2, 2 + 2);
 * @endcode
 *
 * @param type the type of the two expressions
 * @param expr the first expression
 * @param expected the second expression
 */
#ifdef DOXYGEN
#define MU_ASSERT_EQUAL(type, expr, expected)
#else
#define MU_ASSERT_EQUAL(type, expr, expected) \
	__mu_self__->methods->assert_equal(__mu_self__, #expr, #expected, __FILE__, __LINE__, type, (expr), (expected))
#endif

/**
 * @brief Integer type token
 *
 * Specifies that the arguments of an equality assertion are integers
 */
#define MU_INTEGER 0
/**
 * @brief String type token
 *
 * Specifies that the arguments of an equality assertion are strings
 */
#define MU_STRING  1
/**
 * @brief Float type token
 *
 * Specifies that the arguments of an equality assertion are floats
 */
#define MU_FLOAT   2

/**
 * @brief Causes immediate success
 *
 * Use of this macro will cause the current test to
 * terminate and succeed immediately.
 *
 * This macro may only be used in the body of a test,
 * fixture setup, or fixture teardown routine.
 *
 * <b>Example:</b>
 * @code
 * MU_SUCCESS;
 * @endcode
 */
#ifdef DOXYGEN
#define MU_SUCCESS
#else
#define MU_SUCCESS                              \
    __mu_self__->methods->success(__mu_self__)
#endif

/**
 * @brief Causes immediate failure
 *
 * Use of this macro will cause the current test to
 * terminate and fail immediately.  This macro takes
 * a printf format string and an arbitrary number
 * of trailing arguments; the expansion of this string
 * will become the explanation reported for the test
 * failing.
 *
 * This macro may only be used in the body of a test,
 * fixture setup, or fixture teardown routine.
 *
 * <b>Example:</b>
 * @code
 * MU_FAILURE("String '%s' does not contain an equal number of a's and b's",
 *            the_string);
 * @endcode
 *
 * @param format a printf format string for the failure
 * message
 */
#ifdef DOXYGEN
#define MU_FAILURE(format, ...)
#else
#define MU_FAILURE(...) \
    __mu_self__->methods->failure(__mu_self__, __FILE__, __LINE__, __VA_ARGS__)
#endif

/*@}*/

/**
 * @defgroup test_reflect Reflection
 * @ingroup test
 * @brief Macros and structures to inspect live unit tests
 *
 * This modules contains macros and structures that allow running
 * unit tests to inspect their own attributes and environment
 */
/*@{*/

/**
 * @brief Unit test structure
 *
 * Contains all information describing a particular
 * unit test
 */
typedef struct MoonUnitTest
{
    /*@{*/
    /** Test suite name */
    const char* suite;
    /** Test name */
    const char* name;
    /** Source file where test is located */
    const char* file;
    /** First line of test definition */
    unsigned int line;
    /** Test function */
    void (*function) (struct MoonUnitTest*);
    /*@}*/

#ifndef DOXYGEN
	/** Loader which loaded this test */
    struct MoonUnitLoader* loader;
    /** Library which contains this test */
    struct MoonUnitLibrary* library;
	/** Harness which is running this test */
    struct MoonUnitHarness* harness;  
    struct MoonUnitTestMethods* methods;
#endif    

    /** Custom user data */
    void* data;
} MoonUnitTest;

/**
 * @brief Access current unit test
 *
 * This macro expands to a pointer to the MoonUnitTest
 * structure for the currently running test.  Modification
 * of this structure is strongly discouraged, but access
 * to information contained therein may be useful in
 * some applications
 *
 * This macro may only be used in the body of a test,
 * fixture setup, or fixture teardown routine.
 *
 * <b>Example:</b>
 * @code
 * MU_FIXTURE_SETUP(SuiteName)
 * {
 *     CustomLogMessage("Entering test '%s'\n", MU_CURRENT_TEST->name);
 * }
 * @endcode
 *
 */
#ifdef DOXYGEN
#define MU_CURRENT_TEST
#else
#define MU_CURRENT_TEST (__mu_self__)
#endif

/*@}*/

/*@}*/

#ifndef DOXYGEN
#define MU_TEST_PREFIX "__mu_t_"
#define MU_FUNC_PREFIX "__mu_f_"
#define MU_FS_PREFIX "__mu_fs_"
#define MU_FT_PREFIX "__mu_ft_"

struct MoonUnitHarness;
struct MoonUnitLoader;
struct MoonUnitLibrary;
struct MoonUnitTestMethods;

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



extern MoonUnitTestMethods Mu_TestMethods;
#endif

#endif
