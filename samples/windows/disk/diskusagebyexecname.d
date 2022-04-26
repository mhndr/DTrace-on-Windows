
/*++

Copyright (c) Microsoft Corporation

Module Name:

    diskusagebyexecname.d

Abstract:

    This script provides the disk counters for
    a given execname. The execname is case sensitive.

Requirements:

    This script needs symbol's to be configured.

Usage:

     dtrace -s diskusagebyexecname.d <execname>

--*/




#pragma D option quiet
#pragma D option destructive

PLIST_ENTRY head;
PLIST_ENTRY curptr;
struct nt`_EPROCESS *eprocess_ptr;
int found;
int firstpass;
uint64_t bytesread;
uint64_t byteswrite;
int readcount;
int writecount;
int flushcount;


BEGIN
{
    printf("Looking for process '%s'...\n", $1);
    head = (PLIST_ENTRY)&nt`PsActiveProcessHead;
    curptr = head->Flink;
    found = 0;
    firstpass = 1;
    bytesread = 0;
    byteswrite = 0;
    readcount = 0;
    writecount = 0;
    flushcount = 0;
}

tick-10ms
/!found/
{
    /* use this for pointer parsing */
    eprocess_ptr = (struct nt`_EPROCESS *)((intptr_t)curptr - offsetof(nt`_EPROCESS, ActiveProcessLinks));
    processid = (int) eprocess_ptr->UniqueProcessId;
    processName = (string)eprocess_ptr->ImageFileName;

    if ($1 == processName) {
        found = 1;
    } else {
        /*printf("No match on '%s' (%d)\n", processName, processid);*/
        curptr = curptr->Flink;
        if (curptr == head) {
            exit(0);
        }
    }
}

tick-10s
/!found/
{
    exit(0);
}

END
/!found/
{
    printf("No matching process found for %s \n", $1);
}

tick-1s
/found/
{
    system("cls");

    self->DiskCounters = eprocess_ptr->DiskCounters;

    if (firstpass) {
        firstpass = 0;
    } else {
        self->bytesread = self->DiskCounters->BytesRead - bytesread;
        self->byteswrite = self->DiskCounters->BytesWritten - byteswrite;
        self->readcount = self->DiskCounters->ReadOperationCount - readcount;
        self->writecount = self->DiskCounters->WriteOperationCount - writecount;
        self->flushcount = self->DiskCounters->FlushOperationCount - flushcount;

        printf("*** Reports disk read/write every second *** \n");

        printf("Process name: %s\n", eprocess_ptr->ImageFileName);
        printf("Process Id: %d\n", (int) eprocess_ptr->UniqueProcessId);
        printf("Bytes Read %llu \n",  (uint64_t)self->bytesread);
        printf("Bytes Written %llu \n", (uint64_t)self->byteswrite);
        printf("Read Operation Count %d \n", self->readcount);
        printf("Write Operation Count %d \n", self->writecount);
        printf("Flush Operation Count %d \n", self->flushcount);

    }

    bytesread = self->DiskCounters->BytesRead;
    byteswrite = self->DiskCounters->BytesWritten;
    readcount = self->DiskCounters->ReadOperationCount;
    writecount = self->DiskCounters->WriteOperationCount;
    flushcount =  self->DiskCounters->FlushOperationCount;
}

