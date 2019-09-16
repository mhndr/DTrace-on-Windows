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

    IoCreateDevice.d

Abstract:

    This script tracks and reports all new device creation. Plug in a USB like device to validate output.

Requirements: 

    This script has no special requirements.

Usage: 

     dtrace.exe -s IoCreateDevice.d

--*/

struct ustr 
{ 
	uint16_t buffer[256]; 
};

struct _DEVICE_OBJECT {
  short                   Type;
  short                   Size;
  long                    ReferenceCount;
};

/* With FBT, there is an issue with doing symbol resolution. Hence, defining UNICODE_STRING here. */

typedef struct _UNICODE_STRING 
{
  short Length;
  short MaximumLength;
  uint16_t *Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


fbt:nt:IoCreateDeviceSecure:entry
{

	temp = (uint16_t *) ((PUNICODE_STRING) arg2)->Buffer;
	self->spec = speculation();
	speculate (self->spec);
    	printf( "Created device %*ws \n", ((PUNICODE_STRING) arg2)->Length / 2,  ((struct ustr*) temp)->buffer );
	self->sddlstring = 1;
}

fbt:nt:SeConvertStringSecurityDescriptorToSecurityDescriptor:entry
/ self->sddlstring && self->spec/
{
	speculate (self->spec);
	printf(" Device SDDL %*ws \n ", 128, ((struct ustr *) arg0)->buffer);
}
	

fbt:nt:*IoCreateDeviceSecure*:return
{
		if (arg1 == 0)
		{
				commit(self->spec);
				/* printf("Device creation success \n");*/
		}
		else
		{
				discard(self->spec);
				/* printf("Device creation failed %x \n ", arg1); */
		}	
}


