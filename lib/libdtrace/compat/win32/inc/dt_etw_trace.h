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

/*
 * Portions Copyright Microsoft Corporation.
 *
 * 8-May-2019	Rafael Alcaraz Mercado		Created this.
 */

#ifndef _DT_ETW_TRACE_H
#define _DT_ETW_TRACE_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if _WIN32

#include <ntcompat.h>

typedef struct dtrace_hdl dtrace_hdl_t;
typedef struct dtrace_recdesc dtrace_recdesc_t;
typedef struct dt_node dt_node_t;

#define DT_ETW_TRACE_COMMON_ARGS_COUNT (5)
#define DT_ETW_TRACE_COMMON_ARGS_COUNT_WITH_GROUP (6)
#define DT_ETW_TRACE_PAYLOAD_TUPLE_COUNT (3)

typedef struct dt_etw_trace_payload {
	int det_pltype;					/* index to the etw payload data type */
	size_t det_plname_length;		/* length of payload name */
	char *det_plname;				/* string name of the payload */
} dt_etw_trace_payload_t;

typedef struct dt_etw_trace_desc {
	size_t det_provider_name_length;/* length of TraceLogging provider name */
	char *det_provider_name;		/* TraceLogging provider name */

	size_t det_provider_guid_length;/* length of provider GUID string */
	char *det_provider_guid;		/* TraceLogging provider GUID as string */

	size_t det_provider_group_guid_length;/* length of provider group GUID string */
	char *det_provider_group_guid;	/* TraceLogging provider group GUID as string */

	size_t det_event_name_length;	/* length of event name */
	char *det_event_name;			/* TraceLogging event name */

	unsigned char det_level;		/* TraceLogging level */
	uint64_t det_keyword;			/* TraceLogging keyword */
	dt_etw_trace_payload_t *det_pl;	/* pointer to payload array */

	size_t det_plcount;				/* number of payload entries */
	size_t det_string_table_size;	/* size of string table in chars */

	/*
	 * The data array in an etw trace descriptor is a byte blob that contains
	 * embedded payload descriptors and a string table. This is filled in
	 * accordingly by dt_etw_trace_create.
	 * The internal pointer members of an etw trace descriptor point to this
	 * byte buffer for string names and the payload descriptor entries. This
	 * indirection is ensured to be valid on a call to dt_etw_trace_validate.
	 */
	char det_data[1];				/* byte array with all the metadata */
} dt_etw_trace_desc_t;

/*
 * Macro that returns the etw trace descriptor's
 * embedded payload descriptor array size in bytes.
 * This array is a contiguous array of payload
 * descriptors.
 */
#define DT_ETW_TRACE_PLARR_BSIZE(_trace_) \
	((_trace_)->det_plcount * sizeof (dt_etw_trace_payload_t))

/*
 * Macro that returns the etw trace descriptor's
 * embedded string table size in bytes.
 * This string table is a contiguous array of characters
 * with NULL-terminated strings.
 */
#define DT_ETW_TRACE_STRINGTAB_BSIZE(_trace_) \
	((_trace_)->det_string_table_size * sizeof (char))

/*
 * Macro that returns the etw trace descriptor's
 * data size in bytes.
 * This is basically the sum of the string table
 * plus the payload table, starting offset 0
 * at member det_data;
 */
#define DT_ETW_TRACE_DATA_BSIZE(_trace_) \
	(DT_ETW_TRACE_STRINGTAB_BSIZE(_trace_) + DT_ETW_TRACE_PLARR_BSIZE(_trace_))

/*
 * Macro that returns the etw trace descriptor's
 * total size in bytes, including all embedded data.
 * Note that we need to remove the sizeof member
 * det_data because the data size already includes it.
 */
#define DT_ETW_TRACE_BSIZE(_trace_) \
	(sizeof (*(_trace_)) - sizeof ((_trace_)->det_data) + \
	 DT_ETW_TRACE_DATA_BSIZE(_trace_))

/*
 * Macro that returns a raw pointer to the beginning of the
 * payload array in an etw trace descriptor.
 */
#define DT_ETW_TRACE_PLARR_PTR(_trace_) \
	((dt_etw_trace_payload_t *)((_trace_)->det_data))

/*
 * Macro that returns a raw pointer to the beginning of the
 * string table in an etw trace descriptor.
 */
#define DT_ETW_TRACE_STRINGTAB_PTR(_trace_) \
	((char *)((_trace_)->det_data + DT_ETW_TRACE_PLARR_BSIZE(_trace_)))

extern dt_etw_trace_desc_t *dt_etw_trace_create(dtrace_hdl_t *, dt_node_t *);
extern void dt_etw_trace_destroy(dtrace_hdl_t *, dt_etw_trace_desc_t *);

extern void dt_etw_trace_validate(dt_etw_trace_desc_t *);

extern int dt_etw_trace(dtrace_hdl_t *, FILE *, dt_etw_trace_desc_t *,
	const dtrace_recdesc_t *, uint_t , const void *, size_t);

extern void dt_etw_trace_dprintf(dt_etw_trace_desc_t *);
extern void dt_etw_trace_fprintf(dtrace_hdl_t *, FILE *, dt_etw_trace_desc_t *);

#endif /* _WIN32 */

#ifdef __cplusplus
}
#endif

#endif /* _DT_ETW_TRACE_H */
