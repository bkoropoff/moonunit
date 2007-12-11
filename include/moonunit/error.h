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

// Moonunit error code facility
// Used internally and for plugins
// Patterned after GError in glib

typedef struct MuError
{
    const char* domain;
    int code;
    char* message;
} MuError;

void Mu_Error_Raise(MuError** err, const char* domain, int code, const char* format, ...);
void Mu_Error_Handle(MuError** err);
void Mu_Error_Reraise(MuError** err, MuError* src);
bool Mu_Error_Equal(MuError* err, const char* domain, int code);

#define MU_RAISE_RETURN_VOID(err, domain, code, ...)    \
    do                                                  \
    {                                                   \
        Mu_Error_Raise(err, domain, code, __VA_ARGS__); \
        return;                                         \
    } while (0)                                         \

#define MU_RAISE_RETURN(ret, err, domain, code, ...)    \
    do                                                  \
    {                                                   \
        Mu_Error_Raise(err, domain, code, __VA_ARGS__); \
        return (ret);                                   \
    } while (0)                                         \

#define MU_RAISE_GOTO(lab, err, domain, code, ...)      \
    do                                                  \
    {                                                   \
        Mu_Error_Raise(err, domain, code, __VA_ARGS__); \
        goto lab;                                       \
    } while (0)                                         \

#define MU_RERAISE_RETURN_VOID(err, src)        \
    do                                          \
    {                                           \
        Mu_Error_Reraise(err, src);             \
        return;                                 \
    } while (0)                                 \

#define MU_RERAISE_RETURN(ret, err, src)        \
    do                                          \
    {                                           \
        Mu_Error_Reraise(err, src);             \
        return (ret);                           \
    } while (0)                                 \

#define MU_RERAISE_GOTO(lab, err, src)          \
    do                                          \
    {                                           \
        Mu_Error_Reraise(err, src);             \
        goto lab;                               \
    } while (0)                                 \

#define MU_HANDLE_GOTO(lab, domain, code)       \
    do                                          \
    {                                           \
        if (Mu_Error_Equal(err, domain, code))  \
            goto lab;                           \
    } while (0)                                 \

#define MU_HANDLE(err, domain, code)            \
    if (Mu_Error_Equal(err, domain, code))      \
        
#define MU_HANDLE_ALL(err)                      \
    if (err)                                    \

extern char const Mu_ErrorDomain_General[];

typedef enum MuErrorGeneral
{
    MU_ERROR_GENERIC,
    MU_ERROR_NOMEM,
    MU_ERROR_ERRNO
} MuErrorGeneral;

#endif
