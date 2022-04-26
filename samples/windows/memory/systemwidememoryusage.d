

/*++

Copyright (c) Microsoft Corporation

Module Name:

    systemwidememoryusage.d

Abstract:

    This script system-wide memory usage stats.

Requirements:

    This script needs symbols to be configured.

Usage:

     dtrace -s systemwidememoryusage.d -y srv*

--*/

#pragma D option quiet
#pragma D option destructive

tick-3s
{
    system ("cls");

    this->Partition = (struct nt`_MI_PARTITION*)&nt`MiSystemPartition;
    this->Commit = (struct nt`_MI_PARTITION_COMMIT*)&this->Partition->Commit;

    printf("***** Printing System wide page information ******* \n");
    printf( "Total Pages Entire Node: %u MB \n", this->Partition->Core.NodeInformation->TotalPagesEntireNode*4096/(1024*1024));
    printf("Total Available Pages: %u Mb \n", this->Partition->Vp.AvailablePages*4096/(1024*1024));
    printf("Total ResAvail Pages: %u  Mb \n", this->Partition->Vp.ResidentAvailablePages*4096/(1024*1024));
    printf("Total Shared Commit: %u  Mb \n", this->Partition->Vp.SharedCommit*4096/(1024*1024));
    printf("Total Pages for PagingFile: %u  Mb \n", this->Partition->Vp.TotalPagesForPagingFile*4096/(1024*1024));
    printf("Modified Pages : %u  Mb \n", this->Partition->Vp.ModifiedPageListHead.Total*4096/(1024*1024));
    printf("Modified No Write Page Count : %u  Mb \n", this->Partition->Vp.ModifiedNoWritePageListHead.Total*4096/(1024*1024));
    printf("Bad Page Count : %d  Mb \n", this->Partition->PageLists.BadPageListHead.Total*4096/(1024*1024));
    printf("Zero Page Count : %d  Mb \n", this->Partition->PageLists.ZeroedPageListHead.Total*4096/(1024*1024));
    printf("Free Page Count : %d  Mb \n", this->Partition->PageLists.FreePageListHead.Total*4096/(1024*1024));


    /********** Printing Commit info ******************/

    printf("***** Printing Commit Info ******* \n");
    printf("Total Committed Pages: %u  Mb \n", this->Partition->Vp.TotalCommittedPages*4096/(1024*1024));
    printf("Total Commit limit: %u  Mb  \n", this->Partition->Vp.TotalCommitLimit*4096/(1024*1024));
    printf("Peak Commitment: %u Mb \n", this->Commit->PeakCommitment*4096/(1024*1024));
    printf("Low Commit Threshold: %u Mb \n", this->Commit->LowCommitThreshold*4096/(1024*1024));
    printf("High Commit Threshold: %u Mb \n", this->Commit->HighCommitThreshold*4096/(1024*1024));
    printf("System Commit Reserve: %u Mb \n", this->Commit->SystemCommitReserve*4096/(1024*1024));


    printf("******** Gathering details for Partition %d *********\n", this->Partition->Vp.PartitionWs[0].PartitionId);
    printf("Total WorkingSet Size: %u  Mb \n", this->Partition->Vp.PartitionWs[0].WorkingSetSize*4096/(1024*1024));
    printf("Total Page Fault Count: %u  Mb \n", this->Partition->Vp.PartitionWs[0].PageFaultCount*4096/(1024*1024));
    printf("Total Hard Fault Count: %u  Mb \n", this->Partition->Vp.PartitionWs[0].HardFaultCount*4096/(1024*1024));
    printf("Total WorkingSet Leaf Size: %u  Mb  \n", this->Partition->Vp.PartitionWs[0].WorkingSetLeafSize*4096/(1024*1024));
    printf("Trimmed Page Count: %u  Mb \n", this->Partition->Vp.PartitionWs[0].TrimmedPageCount*4096/(1024*1024));
}


