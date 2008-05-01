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
 * @file interface.h
 * @brief MoonUnit unit test API
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

/*@}*/

#ifndef __MU_INTERFACE_H__
#define __MU_INTERFACE_H__

#include <moonunit/boilerplate.h>
#include <moonunit/type.h>
#include <moonunit/test.h>

#include <stdlib.h>

C_BEGIN_DECLS

#ifndef DOXYGEN

#ifdef __GNUC__
#    if defined(__APPLE__) || (defined(__hpux__) && defined(__hppa__))
#        define __MU_SECTION__(name)
#    else
#        define __MU_SECTION__(name) __attribute__((section(name)))
#    endif
#    define __MU_HIDDEN__ __attribute__((visibility("hidden")))
#    define __MU_WEAK__ __attribute__((weak))
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

#define MU_LINK_NONE 0
#define MU_LINK_WEAK 1
#define MU_LINK_UNDEFINED 2
#define MU_LINK_STRONG 3

#ifndef MU_LINK_STYLE
#define MU_LINK_STYLE MU_LINK_UNDEFINED
#endif

#if MU_LINK_STYLE == MU_LINK_WEAK
#    define __MU_LINK__(proto) proto __MU_WEAK__
#elif MU_LINK_STYLE == MU_LINK_UNDEFINED || MU_LINK_STYLE == MU_LINK_STRONG
#    define __MU_LINK__(proto) proto
#else
#    define __MU_LINK__(proto)
#endif

__MU_LINK__(MuTestToken* __mu_current_token(void));

#endif

/**
 * @defgroup test_def Definition
 * @ingroup test
 * @brief Macros to define unit tests
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
 * @param suite_name the (unquoted) name of the test suite which
 * this test should be part of
 * @param test_name the (unquoted) name of this test
 * @hideinitializer
 */
#define MU_TEST(suite_name, test_name)					\
    __MU_SECTION_TEXT__                                                 \
    __MU_HIDDEN_TEST__                                                  \
    void __mu_f_##suite_name##_##test_name(MuTestToken*);		\
    C_DECL MuTest __mu_t_##suite_name##_##test_name;			\
    __MU_SECTION_DATA__                                                 \
    __MU_HIDDEN_TEST__                                                  \
    MuTest __mu_t_##suite_name##_##test_name =				\
    {                                                                   \
        FIELD(suite, #suite_name),					\
        FIELD(name, #test_name),					\
        FIELD(file, __FILE__),                                          \
        FIELD(line, __LINE__),                                          \
        FIELD(loader, NULL),                                            \
        FIELD(library, NULL),                                           \
        FIELD(run, __mu_f_##suite_name##_##test_name)			\
    };                                                                  \
    void __mu_f_##suite_name##_##test_name(MuTestToken* __mu_token__)

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
 * @hideinitializer
 */
#define MU_LIBRARY_SETUP			    \
    __MU_HIDDEN_TEST__				    \
    void __mu_f_ls();				    \
    __MU_HIDDEN_TEST__                      \
    MuLibrarySetup __mu_ls =			    \
    {                                       \
        .file = __FILE__,                   \
        .line = __LINE__,                   \
        .run = __mu_f_ls                    \
    };                                      \
    void __mu_f_ls()

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
 * @hideinitializer
 */
#define MU_LIBRARY_TEARDOWN                 \
    __MU_HIDDEN_TEST__                      \
    void __mu_f_lt();                       \
    __MU_HIDDEN_TEST__                      \
    MuLibraryTeardown __mu_lt =			    \
    {                                       \
        .file = __FILE__,                   \
        .line = __LINE__,                   \
        .run = __mu_f_lt                    \
    };                                      \
    void __mu_f_lt()

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
 * @param suite_name the (unquoted) name of the test suite for
 * which the setup routine is being defined
 * @hideinitializer
 */
#define MU_FIXTURE_SETUP(suite_name)				    \
    __MU_HIDDEN_TEST__						    \
    void __mu_f_fs_##suite_name(MuTestToken* __mu_token__);	    \
    __MU_HIDDEN_TEST__						    \
    MuFixtureSetup __mu_fs_##suite_name =			    \
    {								    \
        .name = #suite_name,					    \
        .file = __FILE__,					    \
        .line = __LINE__,					    \
        .run = __mu_f_fs_##suite_name				    \
    };								    \
    void __mu_f_fs_##suite_name(MuTestToken* __mu_token__)

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
 * @param suite_name the (unquoted) name of the test suite for
 * which the setup routine is being defined
 * @hideinitializer
 */
#define MU_FIXTURE_TEARDOWN(suite_name)				\
    __MU_HIDDEN_TEST__						\
    void __mu_f_ft_##suite_name(MuTestToken* __mu__token__);	\
    __MU_HIDDEN_TEST__						\
    MuFixtureTeardown __mu_ft_##suite_name =			\
    {								\
        .name = #suite_name,					\
        .file = __FILE__,					\
        .line = __LINE__,					\
        .run = __mu_f_ft_##suite_name				\
    };								\
    void __mu_f_ft_##suite_name(MuTestToken* __mu_token__)

/*@}*/

/**
 * @defgroup test_result Testing and Logging
 * @ingroup test
 * @brief Macros to test assertions, flag failures, and log events.
 *
 * This module contains macros for use in the body of unit tests
 * to make assertions, raise errors, report unexpected results,
 * and log arbitrary messages.
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
 * <b>Example:</b>
 * @code
 * MU_ASSERT(foo != NULL && bar > baz);
 * @endcode
 *
 * @param expr the expression to test
 * @hideinitializer
 */
#define MU_ASSERT(expr)							\
    MU_TOKEN->method.assert(MU_TOKEN, expr, 1, #expr, __FILE__, __LINE__)

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
 * <li>MU_TYPE_INTEGER</li>
 * <li>MU_TYPE_FLOAT</li>
 * <li>MU_TYPE_STRING</li>
 * <li>MU_TYPE_POINTER</li>
 * <li>MU_TYPE_BOOLEAN</li>
 * </ul>
 *
 * <b>Example:</b>
 * @code
 * MU_ASSERT_EQUAL(MU_TYPE_INTEGER, 2 * 2, 2 + 2);
 * @endcode
 *
 * @param type the type of the two expressions
 * @param expr the first expression
 * @param expected the second expression
 * @hideinitializer
 */
#define MU_ASSERT_EQUAL(type, expr, expected)			\
    MU_TOKEN->method.assert_equal(MU_TOKEN,			\
                                  #expr, #expected, 1,		\
                                  __FILE__, __LINE__,		\
                                  type, (expr), (expected))	\
    
/**
 * @brief Confirm inequality of values or fail
 *
 * This macro asserts that the values of two expressions
 * are not equal.  If the assertion succeeds, execution of the
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
 * <li>MU_TYPE_INTEGER</li>
 * <li>MU_TYPE_FLOAT</li>
 * <li>MU_TYPE_STRING</li>
 * <li>MU_TYPE_POINTER</li>
 * <li>MU_TYPE_BOOLEAN</li>
 * </ul>
 *
 * <b>Example:</b>
 * @code
 * MU_ASSERT_NOT_EQUAL(MU_TYPE_INTEGER, 2 + 2, 5);
 * @endcode
 *
 * @param type the type of the two expressions
 * @param expr the first expression
 * @param expected the second expression
 * @hideinitializer
 */
#define MU_ASSERT_NOT_EQUAL(type, expr, expected)		\
    MU_TOKEN->method.assert_equal(MU_TOKEN,			\
                                  #expr, #expected, 0,		\
                                  __FILE__, __LINE__,		\
                                  type, (expr), (expected))	\

/**
 * @brief Succeed immediately
 *
 * Use of this macro will cause the current test to
 * terminate and succeed immediately.
 *
 * <b>Example:</b>
 * @code
 * MU_SUCCESS;
 * @endcode
 * @hideinitializer
 */
#define MU_SUCCESS				\
    MU_TOKEN->method.success(MU_TOKEN)

/**
 * @brief Fail immediately
 *
 * Use of this macro will cause the current test to
 * terminate and fail immediately.  This macro takes
 * a printf format string and an arbitrary number
 * of trailing arguments; the expansion of this string
 * will become the explanation reported for the test
 * failing.
 *
 * <b>Example:</b>
 * @code
 * MU_FAILURE("String '%s' does not contain an equal number of a's and b's",
 *            the_string);
 * @endcode
 *
 * @param format a printf format string for the failure
 * message
 * @hideinitializer
 */
#ifdef DOXYGEN
#  define MU_FAILURE(format, ...)
#else
#  define MU_FAILURE(...) \
    (MU_TOKEN->method.failure(MU_TOKEN, __FILE__, __LINE__, __VA_ARGS__))
#endif

/**
 * @brief Skip test
 *
 * Use of this macro will cause the current test to
 * terminate immediately and be reported as skipped.
 * Skipped tests are not counted as failures but are
 * reported differently from successful tests.
 * This macro takes a printf format string
 * and an arbitrary number of trailing arguments;
 * the expansion of this string will serve as an
 * explanation for the test being skipped.
 *
 * <b>Example:</b>
 * @code
 * MU_SKIP("This test is not applicable to the current system");
 * @endcode
 *
 * @param format a printf format string for the explanation
 * message
 * @hideinitializer
 */
#ifdef DOXYGEN
#    define MU_SKIP(format, ...)
#else
#    define MU_SKIP(...)						\
    (MU_TOKEN->method.skip(MU_TOKEN, __FILE__, __LINE__, __VA_ARGS__))
#endif

/**
 * @brief Specify expected result
 *
 * Use of this macro indicates the expected result
 * of the current test.  By default, all tests are
 * expected to succeed (MU_STATUS_SUCCESS).  However,
 * some tests are most naturally written in a way such
 * that they fail, e.g. by throwing an uncaught exception.
 * In these cases, MU_EXPECT may be used to indicate
 * the expected test status before proceeding.  If the
 * indicated status is not MU_STATUS_SUCCESS, test
 * results will be classified as follows:
 * <ul>
 * <li>If the test result is the same as that given to
 * MU_EXPECT, it will be classified as an expected failure</li>
 * <li>If the test result is different from that given to
 * MU_EXPECT but is not MU_STATUS_SUCCESS, it will be classified
 * as a failure</li>
 * <li>If the test result is MU_STATUS_SUCCESS, it will be
 * classified as an unexpected success</li>
 * </ul>
 *
 * <b>Example:</b>
 * @code
 * MU_EXPECT(MU_STATUS_EXCEPTION);
 * @endcode
 *
 * @param result the expected MuTestStatus value
 * @hideinitializer
 */
#define MU_EXPECT(result) \
    (MU_TOKEN->method.expect(MU_TOKEN, result))

/**
 * @brief Set or reset time allowance
 *
 * Use of this macro sets the time allowance
 * for the current test.  If the time allowance is
 * exceeded before the test completes, the test will
 * be forcefully terminated and reported as timing out.
 * Time is counted down starting from the last use of MU_TIMEOUT.
 *
 * <b>Example:</b>
 * @code
 * // This test should complete in at most 5 seconds
 * MU_TIMEOUT(5000);
 * @endcode
 *
 * @param ms the time allowance in milliseconds
 * @hideinitializer
 */
#define MU_TIMEOUT(ms) \
    (MU_TOKEN->method.timeout(MU_TOKEN, ms))

/**
 * @brief Log non-fatal message
 *
 * It is sometimes desirable for a unit test to be able to
 * report information which is orthogonal to the actual
 * test result.  This macro will log a message in the test
 * results without causing the current test to succeed or fail.
 *
 * MU_LOG supports 4 different logging levels of decreasing
 * severity and increasing verbosity:
 * <ul>
 * <li>MU_LEVEL_WARNING - indicates a worrisome but non-fatal condition</li>
 * <li>MU_LEVEL_INFO - indicates an important informational message</li>
 * <li>MU_LEVEL_VERBOSE - indicates information that is usually extraneous but sometimes relevant</li>
 * <li>MU_LEVEL_TRACE - indicates a message designed to help trace the execution of a test</li>
 * </ul>
 *
 * <b>Example:</b>
 * @code
 * MU_LOG(MU_LEVEL_TRACE, "About to call foo()");
 * MU_ASSERT(foo() != NULL);
 * @endcode
 * @param level the logging level
 * @param ... a printf-style format string and trailing arguments for the message to log
 * @hideinitializer
 */
#define MU_LOG(level, ...) (MU_TOKEN->method.event(MU_TOKEN, (level), __FILE__, __LINE__, __VA_ARGS__))

/**
 * @brief Log a warning
 *
 * Equivalent to MU_LOG(MU_LEVEL_WARNING, ...)
 * @hideinitializer
 */
#define MU_WARNING(...) (MU_LOG(MU_LEVEL_WARNING, __VA_ARGS__))
/**
 * @brief Log an informational message
 *
 * Equivalent to MU_LOG(MU_LEVEL_INFO, ...)
 * @hideinitializer
 */
#define MU_INFO(...) (MU_LOG(MU_LEVEL_INFO, __VA_ARGS__))
/**
 * @brief Log a verbose message
 *
 * Equivalent to MU_LOG(MU_LEVEL_VERBOSE, ...)
 * @hideinitializer
 */
#define MU_VERBOSE(...) (MU_LOG(MU_LEVEL_VERBOSE, __VA_ARGS__))
/**
 * @brief Log a trace message
 *
 * Equivalent to MU_LOG(MU_LEVEL_TRACE, ...)
 * @hideinitializer
 */
#define MU_TRACE(...) (MU_LOG(MU_LEVEL_TRACE, __VA_ARGS__))

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
 * @brief Access current unit test
 *
 * This macro expands to a pointer to the MuTest
 * structure for the currently running test.  Modification
 * of this structure is strongly discouraged, but access
 * to information contained therein may be useful in
 * some applications.
 *
 * <b>Example:</b>
 * @code
 * MU_FIXTURE_SETUP(SuiteName)
 * {
 *     CustomLogMessage("Entering test '%s'\n", MU_CURRENT_TEST->name);
 * }
 * @endcode
 * @hideinitializer
 */
#if MU_LINK_STYLE == MU_LINK_NONE
#    define MU_TOKEN (__mu_token__)
#else
#    define MU_TOKEN (__mu_current_token())
#endif

/*@}*/

#ifndef DOXYGEN
#define MU_TEST_PREFIX "__mu_t_"
#define MU_FUNC_PREFIX "__mu_f_"
#define MU_FS_PREFIX "__mu_fs_"
#define MU_FT_PREFIX "__mu_ft_"

void Mu_Interface_SetCurrentToken(MuTestToken* token);

C_END_DECLS

#endif

#endif
