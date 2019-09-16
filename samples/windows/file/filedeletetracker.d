
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

    filedeletetracker.d

Abstract:

    This script tracks files deleted in the system & reports process which deleted the file. 

Requirements: 

    This script has no special requirements.

Usage: 

     dtrace -s svchostrpcleak.d

--*/

ERROR{exit(0);}

struct ustr{uint16_t buffer[256];};


syscall::NtOpenFile:entry
{
   this->deleted = arg5 & 0x00001000; /* & with FILE_DELETE_ON_CLOSE */

  if (this->deleted) {
        this->attr = (nt`_OBJECT_ATTRIBUTES*)
            copyin(arg2, sizeof(nt`_OBJECT_ATTRIBUTES));

        if (this->attr->ObjectName) {
            this->objectName = (nt`_UNICODE_STRING*)
                copyin((uintptr_t)this->attr->ObjectName,
                       sizeof(nt`_UNICODE_STRING));

          
            this->fname = (uint16_t*)
                copyin((uintptr_t)this->objectName->Buffer,
                       this->objectName->Length);

            printf("Process %s PID %d deleted file %*ws \n", execname,pid, 
			this->objectName->Length / 2, 
			 ((struct ustr*)this->fname)->buffer);
        }
    }
}
