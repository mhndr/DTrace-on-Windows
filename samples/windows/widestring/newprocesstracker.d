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

    newprocesstracker.d

Abstract:

    This script tracks and reports all new process created in the system. 

Requirements: 

    This script has no special requirements.

Usage: 

     dtrace.exe -s newprocesstracker.d

--*/

struct ustr{uint16_t buffer[256];};

syscall::NtCreateUserProcess:entry 
{
	this->ProcessParameters = (nt`_RTL_USER_PROCESS_PARAMETERS *) copyin(arg8, sizeof(nt`_RTL_USER_PROCESS_PARAMETERS));

	this->fname = (uint16_t*)
                copyin((uintptr_t) this->ProcessParameters->ImagePathName.Buffer,
                this->ProcessParameters->ImagePathName.Length);

	printf("Process %s PID %d created %*ws \n", execname,pid, 
				this->ProcessParameters->ImagePathName.Length / 2, 
				((struct ustr*)this->fname)->buffer);
	
}