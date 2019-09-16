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

    workingsetinfobyexecname.d

Abstract:

    This script provides the working set for a given execname. The execname is case sensitive.

Requirements: 

    This script needs symbols to be configured.

Usage: 

     dtrace -s workingsetinfobyexecname.d <execname>

--*/


#pragma D option quiet
#pragma D option destructive


intptr_t curptr;
struct nt`_EPROCESS *eprocess_ptr;
int found;

BEGIN
{
	curptr = (intptr_t) ((struct nt`_LIST_ENTRY *) (void*)&nt`PsActiveProcessHead)->Flink;	
	found = 0;
}

tick-1ms
{
/* use this for pointer parsing */

	if (found == 0)
	{
		eprocess_ptr = (struct nt`_EPROCESS *)(curptr - offsetof(nt`_EPROCESS, ActiveProcessLinks));
		processid = (string) eprocess_ptr->ImageFileName;

		if ($1 == processid)
		{
			found = 1;
		}
		else 
		{
			curptr = (intptr_t) ((struct nt`_LIST_ENTRY *) (void*)curptr)->Flink;
		}
	}		
	
}


tick-1s
{
	system ("cls");
	if (found == 1)
	{
		/* printf("Match found flink %x eprocess %x process name: %s\n", curptr, (intptr_t) eprocess_ptr, eprocess_ptr->ImageFileName);*/
		printf("Process name: %s\n", eprocess_ptr->ImageFileName);
		printf("Hard fault count %lu (Mb)\n", (eprocess_ptr->Vm.Instance.HardFaultCount)*4096/(1024*1024));
		printf("Page fault count %lu (Mb)\n", eprocess_ptr->Vm.Instance.PageFaultCount*4096/(1024*1024)); 
		printf("Working Set Size %llu (Kb) \n", eprocess_ptr->Vm.Instance.WorkingSetSize*4*1024/1024 ); 
		printf("Working Set Size (Private) %llu (Kb)\n", eprocess_ptr->Vm.Instance.WorkingSetLeafPrivateSize*4*1024/1024  ); 
		printf("Minimum Working Set Size %llu (Kb) \n", eprocess_ptr->Vm.Instance.MinimumWorkingSetSize*4*1024/1024 );
		printf("Maximum Working Set Size %llu (Kb) \n", eprocess_ptr->Vm.Instance.MaximumWorkingSetSize*4*1024/1024  );
		printf("Peak Set Size %llu (Kb)\n", eprocess_ptr->Vm.Instance.PeakWorkingSetSize*4*1024/1024  );
		if (eprocess_ptr->Vm.Instance.Flags.MemoryPriority )
			printf ("MemoryPriority: Foreground\n");
		else
		{
			printf ("MemoryPriority: Background\n");
		}
		
		printf("Commit charge %llu \n", eprocess_ptr->CommitCharge*4*1024/1024  ); 
		printf("Virtual Size %llu (in Mega bytes) \n", (eprocess_ptr->VirtualSize/(1024*1024)) ); 
		printf("Peak Virtual Size %llu (in Mega bytes) \n", (eprocess_ptr->PeakVirtualSize/(1024*1024)) ); 
	}
	else
	{
		printf("No matching process found for %s \n", $1);
		exit(0);
	}

}
