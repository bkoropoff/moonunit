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
 * @brief MoonUnit C/C++ testing API
 */

/**
 * @defgroup test Unit Tests
 * @brief Macros, structures, and constants to define and
 * inspect unit tests
 *
 * This module contains the essential ingredients to add unit tests
 * to your code which Moonunit can discover and run.
 *
 * @warning As unit tests are not intended for installation on end
 * user systems, the APIs defined in these modules are not guaranteed to
 * remain ABI compatible across releases.  Source compatibility will
 * be maintained within the same major version of MoonUnit.
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
 * This macro defines a unit test; it must be
 * followed by the test body enclosed in curly braces.
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
#define MU_TEST(suite_name, test_name)                                  \
    void __mu_f_test_##suite_name##_##test_name(void);                  \
    C_DECL MuEntryInfo __mu_e_test_##suite_name##_##test_name;          \
    MuEntryInfo __mu_e_test_##suite_name##_##test_name =                \
    {                                                                   \
        FIELD(type, MU_ENTRY_TEST),                                     \
        FIELD(name, #test_name),                                        \
        FIELD(container, #suite_name),                                  \
        FIELD(file, __FILE__),                                          \
        FIELD(line, __LINE__),                                          \
        FIELD(run, __mu_f_test_##suite_name##_##test_name)              \
    };                                                                  \
    void __mu_f_test_##suite_name##_##test_name(void)

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
 * @warning A routine defined in this manner will
 * <i>not</i> be executed in a separate process by
 * the default harness plugin.  Care must be taken
 * to avoid crashing or otherwise interfering with
 * the operation of the harness.
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
#define MU_LIBRARY_SETUP                                                \
    void __mu_f_library_setup();                                        \
    C_DECL MuEntryInfo __mu_e_library_setup;                            \
    MuEntryInfo __mu_e_library_setup =                                  \
    {                                                                   \
        FIELD(type, MU_ENTRY_LIBRARY_SETUP),                            \
        FIELD(name, NULL),                                              \
        FIELD(container, NULL),                                         \
        FIELD(file, __FILE__),                                          \
        FIELD(line, __LINE__),                                          \
        FIELD(run, __mu_f_library_setup)                                \
    };                                                                  \
    void __mu_f_library_setup()

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
 * @warning A routine defined in this manner will
 * <i>not</i> be executed in a separate process by
 * the default harness plugin.  Care must be taken
 * to avoid crashing or otherwise interfering with
 * the operation of the harness.
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
#define MU_LIBRARY_TEARDOWN                                             \
    void __mu_f_library_teardown();                                     \
    C_DECL MuEntryInfo __mu_e_library_teardown;                         \
    MuEntryInfo __mu_e_library_teardown =                               \
    {                                                                   \
        FIELD(type, MU_ENTRY_LIBRARY_TEARDOWN),                         \
        FIELD(name, NULL),                                              \
        FIELD(container, NULL),                                         \
        FIELD(file, __FILE__),                                          \
        FIELD(line, __LINE__),                                          \
        FIELD(run, __mu_f_library_teardown)                             \
    };                                                                  \
    void __mu_f_library_teardown()

/**
 * @brief Define test fixture setup routine
 * 
 * Defines the setup routine for a test fixture --
 * an environment common to all tests in a particular
 * suite.  This routine will be run immediately before
 * each test in the given suite. The setup routine may
 * signal success or failure before the test itself
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
#define MU_FIXTURE_SETUP(suite_name)                                    \
    void __mu_f_fixture_setup_##suite_name(void);                       \
    C_DECL MuEntryInfo __mu_e_fixture_setup_##suite_name;               \
    MuEntryInfo __mu_e_fixture_setup_##suite_name =                     \
    {                                                                   \
        FIELD(type, MU_ENTRY_FIXTURE_SETUP),                            \
        FIELD(name, NULL),                                              \
        FIELD(container, #suite_name),                                  \
        FIELD(file, __FILE__),                                          \
        FIELD(line, __LINE__),                                          \
        FIELD(run, __mu_f_fixture_setup_##suite_name)                   \
    };                                                                  \
    void __mu_f_fixture_setup_##suite_name(void)                        \

/**
 * @brief Define test fixture teardown routine
 * 
 * Defines the teardown routine for a test fixture --
 * an environment common to all tests in a particular
 * suite.  This routine will be run immediately after
 * each test in the given suite.  A teardown routine may
 * signal failure, causing the test to be reported as 
 * failing even if the test itself did not.  This
 * may be used to enforce a common postcondition for
 * a suite of tests, free resources acquired in the
 * setup routine, etc.
 *
 * This macro should be followed by the body of the
 * setup routine enclosed in curly braces.
 *
 * @note If tests are run in a separate process,
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
#define MU_FIXTURE_TEARDOWN(suite_name)                                 \
    void __mu_f_fixture_teardown_##suite_name(void);                    \
    C_DECL MuEntryInfo __mu_e_fixture_teardown_##suite_name;            \
    MuEntryInfo __mu_e_fixture_teardown_##suite_name =                  \
    {                                                                   \
        FIELD(type, MU_ENTRY_FIXTURE_TEARDOWN),                         \
        FIELD(name, NULL),                                              \
        FIELD(container, #suite_name),                                  \
        FIELD(file, __FILE__),                                          \
        FIELD(line, __LINE__),                                          \
        FIELD(run, __mu_f_fixture_teardown_##suite_name)                \
    };                                                                  \
    void __mu_f_fixture_teardown_##suite_name(void)                     \
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
#define MU_ASSERT(expr)                                     \
    (Mu_Interface_Assert(__FILE__, __LINE__, #expr, 1, (expr)))

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
 * @param expr1 the first expression
 * @param expr2 the second expression
 * @hideinitializer
 */
#define MU_ASSERT_EQUAL(type, expr1, expr2)                     \
    (Mu_Interface_AssertEqual(__FILE__, __LINE__,               \
                              #expr1, #expr2, 1,                \
                              type, (expr1), (expr2)))          \
    
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
 * @param expr1 the first expression
 * @param expr2 the second expression
 * @hideinitializer
 */
#define MU_ASSERT_NOT_EQUAL(type, expr1, expr2)                 \
    (Mu_Interface_AssertEqual(__FILE__, __LINE__,               \
                         #expr1, #expr2, 0,                     \
                         type, (expr1), (expr2)))               \
    
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
#define MU_SUCCESS                              \
    (Mu_Interface_Result(__FILE__, __LINE__, MU_STATUS_SUCCESS, NULL))

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
#  define MU_FAILURE(...)                                               \
    (Mu_Interface_Result(__FILE__, __LINE__, MU_STATUS_FAILURE, __VA_ARGS__))
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
#    define MU_SKIP(...)                                                \
    (Mu_Interface_Result(__FILE__, __LINE__, MU_STATUS_SKIPPED, __VA_ARGS__))
#endif

/**
 * @brief Specify expected result
 *
 * Use of this macro indicates the expected result
 * of the current test.  By default, all tests are
 * expected to succeed (MU_STATUS_SUCCESS).  However,
 * some tests are most naturally written in a way such
 * that they normally fail, such as by throwing
 * an uncaught exception. In these cases, MU_EXPECT may
 * be used to indicate the expected test status before 
 * proceeding.  If the indicated status is not
 * MU_STATUS_SUCCESS, test results will be classified
 * as follows:
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
    (Mu_Interface_Expect((result)))

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
    (Mu_Interface_Timeout((ms)))

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
#define MU_LOG(level, ...)                                              \
    (Mu_Interface_Event(__FILE__, __LINE__, (level), __VA_ARGS__))

/**
 * @brief Log a warning
 *
 * Equivalent to MU_LOG(MU_LEVEL_WARNING, ...)
 * @hideinitializer
 */
#define MU_WARNING(...)                         \
    (MU_LOG(MU_LEVEL_WARNING, __VA_ARGS__))
/**
 * @brief Log an informational message
 *
 * Equivalent to MU_LOG(MU_LEVEL_INFO, ...)
 * @hideinitializer
 */
#define MU_INFO(...)                            \
    (MU_LOG(MU_LEVEL_INFO, __VA_ARGS__))
/**
 * @brief Log a verbose message
 *
 * Equivalent to MU_LOG(MU_LEVEL_VERBOSE, ...)
 * @hideinitializer
 */
#define MU_VERBOSE(...)                         \
    (MU_LOG(MU_LEVEL_VERBOSE, __VA_ARGS__))
/**
 * @brief Log a debug message
 *
 * Equivalent to MU_LOG(MU_LEVEL_DEBUG, ...)
 * @hideinitializer
 */
#define MU_DEBUG(...)                         \
    (MU_LOG(MU_LEVEL_DEBUG, __VA_ARGS__))

/**
 * @brief Log a trace message
 *
 * Equivalent to MU_LOG(MU_LEVEL_TRACE, ...)
 * @hideinitializer
 */
#define MU_TRACE(...)                           \
    (MU_LOG(MU_LEVEL_TRACE, __VA_ARGS__))

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
 * @brief Access current test
 *
 * This macro expands to a pointer to the MuTest
 * structure for the currently running test.  This
 * allows for reflective inspection of the current
 * test's properties: name, suite name, etc.
 *
 * <b>Example:</b>
 * @code
 * MU_FIXTURE_SETUP(SuiteName)
 * {
 *     // Access and print the name of the current test
 *     MU_TRACE("Entering test '%s'\n", MU_CURRENT_TEST->test->name);
 * }
 * @endcode
 * @hideinitializer
 */
#define MU_CURRENT_TEST (Mu_Interface_CurrentTest())

/*@}*/

#ifndef DOXYGEN
#define MU_TEST_PREFIX "__mu_t_"
#define MU_FUNC_PREFIX "__mu_f_"
#define MU_FS_PREFIX "__mu_fs_"
#define MU_FT_PREFIX "__mu_ft_"

void Mu_Interface_Expect(MuTestStatus status);
void Mu_Interface_Timeout(long ms);
void Mu_Interface_Event(const char* file, unsigned int line, MuLogLevel level, const char* fmt, ...);
void Mu_Interface_Assert(const char* file, unsigned int line, const char* expr, int sense, int result);
void Mu_Interface_AssertEqual(const char* file, unsigned int line, const char* expr1, const char* expr2, int sense, MuType type, ...);
void Mu_Interface_Result(const char* file, unsigned int line, MuTestStatus result, const char* message, ...);
MuTest* Mu_Interface_CurrentTest(void);

typedef enum MuEntryType
{
    MU_ENTRY_TEST,
    MU_ENTRY_LIBRARY_SETUP,
    MU_ENTRY_LIBRARY_TEARDOWN,
    MU_ENTRY_FIXTURE_SETUP,
    MU_ENTRY_FIXTURE_TEARDOWN,
} MuEntryType;

typedef struct MuEntryInfo
{
    MuEntryType type;
    const char* name;
    const char* container;
    const char* file;
    unsigned int line;
    void (*run)(void);
} MuEntryInfo;
#endif

C_END_DECLS

#endif
