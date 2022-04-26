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


