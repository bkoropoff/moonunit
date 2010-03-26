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

#ifndef __MU_ERROR_H__
#define __MU_ERROR_H__

#include <stdbool.h>

#include <moonunit/internal/boilerplate.h>

/* Moonunit error code facility
 * Used internally and for plugins
 * Patterned after GError in glib
 */

C_BEGIN_DECLS

typedef enum MuStatusCode
{
    /** Success */
    MU_ERROR_SUCCESS = 0,
    /** Generic error */
    MU_ERROR_GENERAL = 1,
    /** Out of memory */
    MU_ERROR_MEMORY = 2,
    /** Unexpected system error */
    MU_ERROR_SYSTEM = 3,
    /** Library loading failed */
    MU_ERROR_LOAD_LIBRARY = 4,
    /** Library could not be constructed */
    MU_ERROR_CONSTRUCT_LIBRARY = 5,
    /** Library could not be destroyed */
    MU_ERROR_DESTRUCT_LIBRARY = 6
} MuStatusCode;

typedef struct MuError
{
    int code;
    char* message;
} MuError;

void mu_error_raise(MuError** err, MuStatusCode code, const char* format, ...);
void mu_error_handle(MuError** err);
void mu_error_reraise(MuError** err, MuError* src);
bool mu_error_equal(MuError* err, int code);

#define MU_RAISE_RETURN_VOID(err, code, ...)            \
    do                                                  \
    {                                                   \
        mu_error_raise(err,  code, __VA_ARGS__);        \
        return;                                         \
    } while (0)                                         \
        
#define MU_RAISE_RETURN(ret, err, code, ...)            \
    do                                                  \
    {                                                   \
        mu_error_raise(err,  code, __VA_ARGS__);        \
        return (ret);                                   \
    } while (0)                                         \

#define MU_RAISE_GOTO(lab, err, code, ...)              \
    do                                                  \
    {                                                   \
        mu_error_raise(err, code, __VA_ARGS__);         \
        goto lab;                                       \
    } while (0)                                         \

#define MU_RERAISE_RETURN_VOID(err, src)        \
    do                                          \
    {                                           \
        mu_error_reraise(err, src);             \
        return;                                 \
    } while (0)                                 \

#define MU_RERAISE_RETURN(ret, err, src)        \
    do                                          \
    {                                           \
        mu_error_reraise(err, src);             \
        return (ret);                           \
    } while (0)                                 \

#define MU_RERAISE_GOTO(lab, err, src)          \
    do                                          \
    {                                           \
        mu_error_reraise(err, src);             \
        goto lab;                               \
    } while (0)                                 \

#define MU_CATCH_GOTO(lab, code)                \
    do                                          \
    {                                           \
        if (mu_error_equal(err, code))          \
            goto lab;                           \
    } while (0)                                 \
        
#define MU_CATCH(err, code)             \
    if (mu_error_equal(err, code))      \
        
#define MU_CATCH_ALL(err)                       \
    if (err)                                    \

#define MU_HANDLE(err)                          \
    mu_error_handle(err)

C_END_DECLS

#endif
