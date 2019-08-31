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

/*++

Copyright (c) Microsoft Corporation

Module Name:

    dtracep.h

Abstract:

    This file contains definitions local to the DTrace driver.

--*/

//
// Environment.
//

extern int dtrace_cpusup_init(void);
extern void dtrace_cpusup_cleanup(void);
extern void dtrace_module_unloaded(const char* modname);

//
// Driver
//

extern int dtrace_load(void);
extern int dtrace_unload(void);

//
// Device
//

struct dtrace_state;
extern int dtrace_open(PDEVICE_OBJECT dev, struct dtrace_state ** data);
extern void dtrace_close(struct dtrace_state *data);
extern int dtrace_ioctl(struct dtrace_state *state, unsigned long cmd, void* addr);

//
// Utility
//

#if !defined(DTRACE_STANDALONE)
#define dtrace_tsc_frequency_hv TraceGetTscFrequency
#define dtrace_safememcpy       TraceAccessMemory
#define dtrace_userstackwalk    TraceWalkUserStack
#define dtrace_threadprivate    TraceGetCurrentThreadTracePrivate
#define dtrace_priv_filter      TraceFilterAccess
#define dtrace_lkd              TraceActionLiveKernelDump
#define dtrace_tfreg            TraceGetTrapFrameRegister
#define dtrace_tf               TraceGetTrapFrame
#endif

extern ULONGLONG dtrace_tsc_frequency_hv(void);
extern int dtrace_safememcpy(void* sys, uintptr_t untr, size_t bytesize, size_t chunksize, int isread);
extern ULONG dtrace_userstackwalk(ULONG limit, PULONGLONG stack);
extern PULONG_PTR dtrace_threadprivate(ULONG Index);
extern void dtrace_priv_filter(KPROCESSOR_MODE PreviousMode, PBOOLEAN Kernel, PBOOLEAN User);
extern ULONG_PTR dtrace_tfreg(/*PKTRAP_FRAME*/PVOID TrapFrame, ULONG RegisterIndex);
extern /*PKTRAP_FRAME*/PVOID dtrace_tf(void);
extern void dtrace_lkd(LPCWSTR ComponentName,
                        ULONG BugCheckCode,
                        ULONG_PTR P1,
                        ULONG_PTR P2,
                        ULONG_PTR P3,
                        ULONG_PTR P4,
                        ULONG Flags);


