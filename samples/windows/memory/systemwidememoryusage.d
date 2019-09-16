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

    systemwidememoryusage.d

Abstract:

    This script system-wide memory usage stats. 

Requirements: 

    This script needs symbols to be configured.

Usage: 

     dtrace -s systemwidememoryusage.d

--*/

#pragma D option quiet
#pragma D option destructive

tick-3s
{
	system ("cls");

	printf("***** Printing System wide page information ******* \n");
	printf( "Total Pages Entire Node: %u MB \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Core.NodeInformation->TotalPagesEntireNode*4096/(1024*1024));
	printf("Total Available Pages: %u Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.AvailablePages*4096/(1024*1024));
	printf("Total ResAvail Pages: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.ResidentAvailablePages*4096/(1024*1024));
	printf("Total Shared Commit: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.SharedCommit*4096/(1024*1024));
	printf("Total Pages for PagingFile: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.TotalPagesForPagingFile*4096/(1024*1024));
	printf("Modified Pages : %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.ModifiedPageListHead.Total*4096/(1024*1024));
	printf("Modified No Write Page Count : %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.ModifiedNoWritePageListHead.Total*4096/(1024*1024));
	printf("Bad Page Count : %d  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->PageLists.BadPageListHead.Total*4096/(1024*1024));
	printf("Zero Page Count : %d  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->PageLists.ZeroedPageListHead.Total*4096/(1024*1024));
	printf("Free Page Count : %d  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->PageLists.FreePageListHead.Total*4096/(1024*1024));


	/********** Printing Commit info ******************/ 
	
	printf("***** Printing Commit Info ******* \n");
	printf("Total Committed Pages: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.TotalCommittedPages*4096/(1024*1024));
	printf("Total Commit limit: %u  Mb  \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.TotalCommitLimit*4096/(1024*1024));
	printf("Peak Commitment: %u Mb \n", ((struct nt`_MI_PARTITION_COMMIT ) ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Commit).PeakCommitment*4096/(1024*1024));
	printf("Low Commit Threshold: %u Mb \n", ((struct nt`_MI_PARTITION_COMMIT ) ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Commit).LowCommitThreshold*4096/(1024*1024));
	printf("High Commit Threshold: %u Mb \n", ((struct nt`_MI_PARTITION_COMMIT ) ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Commit).HighCommitThreshold*4096/(1024*1024));
	printf("System Commit Reserve: %u Mb \n", ((struct nt`_MI_PARTITION_COMMIT ) ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Commit).SystemCommitReserve*4096/(1024*1024));
	

	printf("******** Gathering details for Partition %d *********\n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.PartitionWs[0].PartitionId);
	printf("Total WorkingSet Size: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.PartitionWs[0].WorkingSetSize*4096/(1024*1024));
	printf("Total Page Fault Count: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.PartitionWs[0].PageFaultCount*4096/(1024*1024));
	printf("Total Hard Fault Count: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.PartitionWs[0].HardFaultCount*4096/(1024*1024));	
	printf("Total WorkingSet Leaf Size: %u  Mb  \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.PartitionWs[0].WorkingSetLeafSize*4096/(1024*1024));	
	printf("Trimmed Page Count: %u  Mb \n", ((struct nt`_MI_PARTITION *) &nt`MiSystemPartition)->Vp.PartitionWs[0].TrimmedPageCount*4096/(1024*1024));	
}