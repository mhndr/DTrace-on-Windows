

/*++

Copyright (c) Microsoft Corporation

Module Name:

    filedeletetracker.d

Abstract:

    This script tracks files deleted in the system & reports process which deleted the file.

Usage:

     dtrace -s filedeletetracker.d

--*/

ERROR
{
    exit(0);
}

struct ustr{uint16_t buffer[256];};

inline ULONG FILE_DELETE_ON_CLOSE = 0x00001000;

syscall::NtOpenFile:entry
{
    this->deleted = args[5] & FILE_DELETE_ON_CLOSE; /* & with  */

    if (this->deleted)
    {
        this->attr = (POBJECT_ATTRIBUTES)
            copyin(arg2, sizeof(OBJECT_ATTRIBUTES));

        if (this->attr->ObjectName)
        {
            this->objectName = (PUNICODE_STRING)
                copyin((uintptr_t)this->attr->ObjectName,
                       sizeof(UNICODE_STRING));

            this->fname = (uint16_t*)
                copyin((uintptr_t)this->objectName->Buffer,
                       this->objectName->Length);

            printf("Process %s PID %d deleted file %*ws \n",
                   execname,
                   pid,
                   this->objectName->Length / 2,
                   ((struct ustr*)this->fname)->buffer);
        }
    }
}
