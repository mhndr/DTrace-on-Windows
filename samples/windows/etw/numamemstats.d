
/*++

Copyright (c) Microsoft Corporation

Module Name:

    numamemstats.d

Abstract:

    This script uses Microsoft-Windows-Kernel-Memory ETW provider to dump per
    NUMA node memory. Page size can be converted to size in KB by multiplying by 4.

Requirements:

    This script has no special requirements.

Usage:

     dtrace -s numamemstats.d

    DTrace supports both manifested and tracelogged events. To probe specific
    keywords/levels/eventIDs, ETW probes will work much more reliably if you don’t use wild cards. Instead fully
    specify your probe based on these rules:
    Probename = etw
    Modname = Provider guid in the form xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, using all lowercase characters.
    Funcname = Level_Keyword of the form 0x00_0x0000000000000000. To match everything this should be set to 0xff_0xffffffffffffffff.
    Probename = Integer Event ID or "generic_event" to match all event IDs.

    Please note that filtering based on Probename only works for manifested events. Use wild-card (*) for tracelogged events.

--*/

typedef struct MemoryNodeInfo
{
    uint64_t TotalPageCount;
    uint64_t SmallFreePageCount;
    uint64_t SmallZeroPageCount;
    uint64_t MediumFreePageCount;
    uint64_t MediumZeroPageCount;
    uint64_t LargeFreePageCount;
    uint64_t LargeZeroPageCount;
    uint64_t HugeFreePageCount;
    uint64_t HugeZeroPageCount;
} m_nodeinfo;

typedef struct KernelMemInfoEvent
{
    EVENT_HEADER Header;
    uint32_t PartitionId;
    uint32_t Count;
    uint32_t NodeNumber;
    /* m_nodeinfo mnt;*/
} kmi;

int printcounter;

BEGIN
{
    printcounter = 0;
}

/* MemNodeInfo */
etw:d1d93ef7-e1f2-4f45-9943-03d245fe6c00:0xff_0xffffffffffffffff:12
{
    if ((printcounter % 10) == 0)
    {
        printf ("\n \n");
        printf("Partition ID: %d \n",((kmi *)arg0)->PartitionId);
        printf("Count: %d \n", ((kmi *)arg0)->Count);

        printf("Node number: %d\n", ((kmi *)arg0)->NodeNumber);
        counters = (m_nodeinfo*)(arg0 + sizeof(EVENT_HEADER) + 12);
        print(*counters);

        /* Dump rest of the NUMA node info */

        if (((kmi *)arg0)->Count > 1)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(1)) + (sizeof(uint32_t)*(1)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(1)) + (sizeof(uint32_t)*(1)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 2)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(2)) + (sizeof(uint32_t)*(2)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(2)) + (sizeof(uint32_t)*(2)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 3)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(3)) + (sizeof(uint32_t)*(3)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(3)) + (sizeof(uint32_t)*(3)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 4)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(4)) + (sizeof(uint32_t)*(4)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(4)) + (sizeof(uint32_t)*(4)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 5)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(5)) + (sizeof(uint32_t)*(5)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(5)) + (sizeof(uint32_t)*(5)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 6)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(6)) + (sizeof(uint32_t)*(6)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(6)) + (sizeof(uint32_t)*(6)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 7)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(7)) + (sizeof(uint32_t)*(7)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(7)) + (sizeof(uint32_t)*(7)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 8)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(8)) + (sizeof(uint32_t)*(8)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(8)) + (sizeof(uint32_t)*(8)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 9)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(9)) + (sizeof(uint32_t)*(9)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(9)) + (sizeof(uint32_t)*(9)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 10)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(10)) + (sizeof(uint32_t)*(10)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(10)) + (sizeof(uint32_t)*(10)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 11)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(11)) + (sizeof(uint32_t)*(11)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(11)) + (sizeof(uint32_t)*(11)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 12)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(12)) + (sizeof(uint32_t)*(12)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(12)) + (sizeof(uint32_t)*(12)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 13)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(13)) + (sizeof(uint32_t)*(13)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(13)) + (sizeof(uint32_t)*(13)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 14)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(14)) + (sizeof(uint32_t)*(14)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(14)) + (sizeof(uint32_t)*(14)) + sizeof(uint32_t));
            print(*counters);
        }
        if (((kmi *)arg0)->Count > 15)
        {
            nodenumber = (uint32_t *) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(15)) + (sizeof(uint32_t)*(15)) );
            printf ("Node Number: %d \n", *nodenumber);
            counters = (m_nodeinfo*) (arg0 + sizeof(EVENT_HEADER) + 8 + (sizeof(m_nodeinfo)*(15)) + (sizeof(uint32_t)*(15)) + sizeof(uint32_t));
            print(*counters);
        }

    }

    exit(1);
    printcounter++;
}

