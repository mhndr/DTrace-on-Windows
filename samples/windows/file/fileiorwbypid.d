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

    fileiorwbypid.d

Abstract:

    This script tracks all file read and write I/O operations. Data is reported every 5 seconds by PID.

Requirements: 

    This script has no special requirements.

Usage: 

     dtrace.exe -s fileiorwbypid.d

--*/


#pragma D option quiet 
#pragma D option destructive
#pragma D option aggrate=1000us


BEGIN { 
    ts = timestamp; 
} 

syscall::NtReadFile:entry
{
	@read[execname, pid] = sum((unsigned long) arg6);
	@write[execname, pid] = sum(0);
}

syscall::NtWriteFile:entry
{
	@write[execname, pid] = sum((unsigned long) arg6);
	@read[execname, pid] = sum(0);
}

tick-3s { 
	system("cls");
	trunc(@read, 10);
	trunc(@write, 10);
	fact = 1024 * (timestamp - ts) / 1000000000; 
	normalize(@read, fact); 
	normalize(@write, fact); 

	printf("\n %-16s %16s %10s %10s\n", "Executable", "PID", "read[kb/s]", "write[kb/s]");
	printa(" %-16s %16d %@10d %@10d \n",@read, @write);

	clear(@read); 
	clear(@write);

	ts = timestamp; 
} 
