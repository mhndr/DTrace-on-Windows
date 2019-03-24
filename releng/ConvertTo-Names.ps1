param(
	[Parameter(Mandatory=$true)][String] $File,
	[Parameter(Mandatory=$true)][String] $OutFile
)

Set-Content $OutFile @"
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident`t`"%Z%%M%`t%I%`t%E% SMI`"

#include <dtrace.h>

/*ARGSUSED*/
const char *
dtrace_subrstr(dtrace_hdl_t *dtp, int subr)
{
    switch (subr) {
"@

foreach($line in Get-Content -Path $File) {
	$expr = '#define[ 	]*(DIF_SUBR_(\w*))'
	if($line -match $expr -and $Matches[1] -ne "DIF_SUBR_MAX") {
		"`t`t" + "case $($Matches[1]): return (`"$($Matches[2].ToLowerInvariant())`");" | 
      Add-Content $OutFile
	}
}

Add-Content $OutFile @"
`t`tdefault: return (`"unknown`");
      }
    }
"@
