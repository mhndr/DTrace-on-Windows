param(
	[Parameter(Mandatory=$true)][String] $File,
	[Parameter(Mandatory=$true)][String] $OutFile
)

Set-Content $OutFile @"
/*
 * Copyright 2003 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident`t`"%Z%%M%`t%I%`t%E% SMI`"

#include <dt_errtags.h>

static const char *const _dt_errtags[] = {
"@

foreach($line in Get-Content -Path $File) {
	$expr = '^\s*(D_[A-Z0-9_]*),*'
	if($line -match $expr) {
		$line -replace $expr, '	"${1}",' | Add-Content $OutFile
	}
}

Add-Content $OutFile @'
};

static const int _dt_ntag = sizeof (_dt_errtags) / sizeof (_dt_errtags[0]);

const char *
dt_errtag(dt_errtag_t tag)
{
	return (_dt_errtags[(tag > 0 && tag < _dt_ntag) ? tag : 0]);
}
'@
