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
inline ULONG FILE_DISPOSITION_DELETE = 0x00000001;

syscall::NtOpenFile:entry, syscall::NtCreateFile:entry
{
    if (probefunc == "NtOpenFile")
    {
        this->deleted = arg5 & FILE_DELETE_ON_CLOSE; /* & with  */
    }
    else if (probefunc == "NtCreateFile")
    {
        this->deleted = arg8 & FILE_DELETE_ON_CLOSE;
    }


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

syscall::NtSetInformationFile:entry
{
    self->fileInformationClass = args[4];
    self->handle = arg0;

    if (self->fileInformationClass == FileDispositionInformation ||
        self->fileInformationClass == FileDispositionInformationEx)
    {
        self->fileInformationData = arg2;
        self->fileInformationLength = args[3];
    }
}

syscall::NtSetInformationFile:return
/ self->fileInformationData /
{
    this->fileInformation = copyin(self->fileInformationData, self->fileInformationLength);

    this->deleted = (self->fileInformationClass == FileDispositionInformationEx) ?
                            ((PFILE_DISPOSITION_INFORMATION_EX) this->fileInformation)->Flags & FILE_DISPOSITION_DELETE :
                            ((PFILE_DISPOSITION_INFORMATION) this->fileInformation)->DeleteFile;

    if (this->deleted)
    {
        printf("Process %s PID %d deleted file handle %x \n",
                    execname,
                    pid,
                    self->handle);
    }

    self->fileInformationData = 0;
}
