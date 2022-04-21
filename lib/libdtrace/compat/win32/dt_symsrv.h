/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

// Copyright (c) Microsoft Corporation

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern struct dt_symsrv* dt_symsrv_start(void);
extern void dt_symsrv_stop(struct dt_symsrv*);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C++" {
#include <string_view>
#include <string>
#include <vector>

struct dt_symsrv_param_types_t {
    std::vector<std::string> ParamTypes;
    bool VaArgs;
};

extern dt_symsrv_param_types_t
dt_symsrv_load_paramtypes(HANDLE Process, std::string_view ModuleName,
                          ULONGLONG Base, ULONG TypeIndex);

}
#endif // __cplusplus
