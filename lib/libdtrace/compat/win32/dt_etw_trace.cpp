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

#ifdef _WIN32

#include "dt_etw_trace.h"

#include <dtrace.h>
#include <dt_impl.h>

#include <minwindef.h>
#include <winmeta.h>
#include <rpc.h>
#include <traceloggingdynamic.h>

#include <memory>
#include <map>
#include <vector>

typedef struct dt_etw_trace_add_params {
	tld::EventBuilder<std::vector<BYTE>> &event;
	const char* event_name;
	tld::Type type;
	caddr_t data;
	uint32_t data_size;
} dt_etw_trace_add_params_t;

template<typename T>
int
dt_etw_trace_add_type(const dt_etw_trace_add_params_t &params)
{
	if (params.data_size == 0 || params.data_size > sizeof (T))
		return (-1);

	params.event.AddField(params.event_name, params.type);
	params.event.AddValue(*((T *)params.data));

	return (0);
}

typedef int dt_etw_trace_check_f(dt_node_t *);
typedef int dt_etw_trace_pl_add_f(const dt_etw_trace_add_params_t &params);

typedef struct dt_etw_trace_type {
	const char *det_type_name;			/* string name of the etw type */
	tld::Type det_tld_type;				/* trace logging dynamic type */
	dt_etw_trace_check_f *det_check;	/* function to use for type checking */
	dt_etw_trace_pl_add_f *det_add;		/* function to use for type add */
} dt_etw_trace_type_t;

template<typename T>
int
dt_etw_trace_int_check(dt_node_t *dnp)
{
	if (dt_node_type_size(dnp) > sizeof (T))
		return (-1);

	return dt_node_is_integer(dnp) ? (0) : (-1);
}

template<typename T>
int
dt_etw_trace_float_check(dt_node_t *dnp)
{
	if (dt_node_type_size(dnp) > sizeof (T))
		return (-1);

	return dt_node_is_float(dnp) || dt_node_is_integer(dnp) ? (0) : (-1);
}

int
dt_etw_trace_string_check(dt_node_t *dnp)
{
	return dt_node_is_string(dnp) ? (0) : (-1);
}

int
dt_etw_trace_string_add(const dt_etw_trace_add_params_t &params)
{
	if (params.data_size == 0)
		return (-1);

	params.event.AddField(params.event_name, params.type);
	params.event.AddString(params.data);

	return (0);
}

int
dt_etw_trace_pointer_check(dt_node_t *dnp)
{
	if (dt_node_type_size(dnp) > sizeof (void *))
		return (-1);

	return dt_node_is_integer(dnp) ? (0) : (-1);
}

int
dt_etw_trace_pointer_add(const dt_etw_trace_add_params_t &params)
{
	return sizeof (void *) == sizeof (int32_t) ?
		dt_etw_trace_add_type<int32_t>(params) :
		dt_etw_trace_add_type<int64_t>(params) ;
}

#define INT_CHECK_AND_ADD_FUNCS(_T_) \
	[](dt_node_t *dnp) \
	{ \
		return dt_etw_trace_int_check<_T_>(dnp); \
	}, \
	[](const dt_etw_trace_add_params_t &params) \
	{ \
		return dt_etw_trace_add_type<_T_>(params); \
	}

#define FLOAT_CHECK_AND_ADD_FUNCS(_T_) \
	[](dt_node_t *dnp) \
	{ \
		return dt_etw_trace_float_check<_T_>(dnp); \
	}, \
	[](const dt_etw_trace_add_params_t &params) \
	{ \
		return dt_etw_trace_add_type<_T_>(params); \
	}

/*
 * Table of etw trace types.
 * These types are conversible to other "D" and "C" types,
 * but a call to etw_trace MUST always only refer to the etw types
 * listed in this table.
 *
 * This approach allows for an extensible design where we can add windows
 * specific etw types without having to extend the "D" supported types.
 *
 * This array is used to validate the payload type argument to a call to
 * etw_trace, and then to validate that the payload value type is conversible
 * and/or equivalent.
 *
 *
 * TODO: Extend this table to add support for more types and/or update the
 * existing ones to add check and add functions.
 */
static dt_etw_trace_type _dt_etw_types[] = {
	{
		"etw_struct", /* THIS MUST ALWAYS BE AT INDEX 0 */
		tld::TypeUInt32,
		[](dt_node_t *dnp)
		{
			return dt_etw_trace_int_check<uint32_t>(dnp);
		},
		NULL /* Doesn't produce payload value, rather than just metadata */
	},
	{
		"etw_widestring",
		tld::TypeUtf16String,
		NULL,
		NULL
	},
	{
		"etw_string",
		tld::TypeMbcsString,
		dt_etw_trace_string_check,
		dt_etw_trace_string_add
	},
	{
		"etw_utf16string",
		tld::TypeUtf16String,
		NULL,
		NULL
	},
	{
		"etw_mbcsstring",
		tld::TypeMbcsString,
		dt_etw_trace_string_check,
		dt_etw_trace_string_add
	},
	{
		"etw_int8",
		tld::TypeInt8,
		INT_CHECK_AND_ADD_FUNCS(int8_t)
	},
	{
		"etw_uint8",
		tld::TypeUInt8,
		INT_CHECK_AND_ADD_FUNCS(uint8_t)
	},
	{
		"etw_int16",
		tld::TypeInt16,
		INT_CHECK_AND_ADD_FUNCS(int16_t)
	},
	{
		"etw_uint16",
		tld::TypeUInt16,
		INT_CHECK_AND_ADD_FUNCS(uint16_t)
	},
	{
		"etw_int32",
		tld::TypeInt32,
		INT_CHECK_AND_ADD_FUNCS(int32_t)
	},
	{
		"etw_uint32",
		tld::TypeUInt32,
		INT_CHECK_AND_ADD_FUNCS(uint32_t)
	},
	{
		"etw_int64",
		tld::TypeInt64,
		INT_CHECK_AND_ADD_FUNCS(int64_t)
	},
	{
		"etw_uint64",
		tld::TypeUInt64,
		INT_CHECK_AND_ADD_FUNCS(uint64_t)
	},
	{
		"etw_float",
		tld::TypeFloat,
		FLOAT_CHECK_AND_ADD_FUNCS(float_t)
	},
	{
		"etw_double",
		tld::TypeDouble,
		FLOAT_CHECK_AND_ADD_FUNCS(double_t)
	},
	{
		"etw_bool32",
		tld::TypeBool32,
		INT_CHECK_AND_ADD_FUNCS(int32_t)
	},
	{
		"etw_binary",
		tld::TypeBinary,
		NULL,
		NULL
	},
	{
		"etw_guid",
		tld::TypeGuid,
		NULL,
		NULL
	},
	{
		"etw_filetime",
		tld::TypeFileTime,
		NULL,
		NULL
	},
	{
		"etw_systemtime",
		tld::TypeSystemTime,
		NULL,
		NULL
	},
	{
		"etw_sid",
		tld::TypeSid,
		NULL,
		NULL
	},
	{
		"etw_hexint32",
		tld::TypeHexInt32,
		INT_CHECK_AND_ADD_FUNCS(int32_t)
	},
	{
		"etw_hexint64",
		tld::TypeHexInt64,
		INT_CHECK_AND_ADD_FUNCS(int64_t)
	},
	{
		"etw_countedutf16string",
		tld::TypeCountedUtf16String,
		NULL,
		NULL
	},
	{
		"etw_countedmbcsstring",
		tld::TypeCountedMbcsString,
		NULL,
		NULL
	},
	{
		"etw_intptr",
		tld::TypeIntPtr,
		dt_etw_trace_pointer_check,
		dt_etw_trace_pointer_add
	},
	{
		"etw_uintptr",
		tld::TypeUIntPtr,
		dt_etw_trace_pointer_check,
		dt_etw_trace_pointer_add
	},
	{
		"etw_pointer",
		tld::TypePointer,
		dt_etw_trace_pointer_check,
		dt_etw_trace_pointer_add
	},
	{
		"etw_char16",
		tld::TypeChar16,
		INT_CHECK_AND_ADD_FUNCS(int16_t)
	},
	{
		"etw_char8",
		tld::TypeChar8,
		INT_CHECK_AND_ADD_FUNCS(int8_t)
	},
	{
		"etw_bool8",
		tld::TypeBool8,
		INT_CHECK_AND_ADD_FUNCS(int8_t)
	},
	{
		"etw_hexint8",
		tld::TypeHexInt8,
		INT_CHECK_AND_ADD_FUNCS(int8_t)
	},
	{
		"etw_hexint16",
		tld::TypeHexInt16,
		INT_CHECK_AND_ADD_FUNCS(int16_t)
	},
	{
		"etw_pid",
		tld::TypePid,
		INT_CHECK_AND_ADD_FUNCS(int32_t)
	},
	{
		"etw_tid",
		tld::TypeTid,
		INT_CHECK_AND_ADD_FUNCS(int32_t)
	},
	{
		"etw_port",
		tld::TypePort,
		NULL,
		NULL
	},
	{
		"etw_ipv4",
		tld::TypeIPv4,
		NULL,
		NULL
	},
	{
		"etw_ipv6",
		tld::TypeIPv6,
		NULL,
		NULL
	},
	{
		"etw_socketaddress",
		tld::TypeSocketAddress,
		NULL,
		NULL
	},
	{
		"etw_utf16xml",
		tld::TypeUtf16Xml,
		NULL,
		NULL
	},
	{
		"etw_mbcsxml",
		tld::TypeMbcsXml,
		dt_etw_trace_string_check,
		dt_etw_trace_string_add
	},
	{
		"etw_utf16json",
		tld::TypeUtf16Json,
		NULL,
		NULL
	},
	{
		"etw_mbcsjson",
		tld::TypeMbcsJson,
		dt_etw_trace_string_check,
		dt_etw_trace_string_add
	},
	{
		"etw_countedutf16xml",
		tld::TypeCountedUtf16Xml,
		NULL,
		NULL
	},
	{
		"etw_countedmbcsxml",
		tld::TypeCountedMbcsXml,
		NULL,
		NULL
	},
	{
		"etw_countedutf16json",
		tld::TypeCountedUtf16Json,
		NULL,
		NULL
	},
	{
		"etw_countedmbcsjson",
		tld::TypeCountedMbcsJson,
		NULL,
		NULL
	},
	{
		"etw_win32error",
		tld::TypeWin32Error,
		INT_CHECK_AND_ADD_FUNCS(uint32_t)
	},
	{
		"etw_ntstatus",
		tld::TypeNTStatus,
		INT_CHECK_AND_ADD_FUNCS(uint32_t)
	},
	{
		"etw_hresult",
		tld::TypeHResult,
		INT_CHECK_AND_ADD_FUNCS(uint32_t)
	},
	{ NULL, tld::TypeNone, NULL, NULL } /* mark end of array */
};

/*
 * Returns the index (starting at 0) to the type name in the supported type
 * names table. Returns -1 if the supplied type name is not supported.
 */
int
dt_etw_trace_type_lookup(const char *type_name)
{
	int i;
	for (i = 0; _dt_etw_types[i].det_type_name != NULL; i++) {
		if (strcmp(type_name, _dt_etw_types[i].det_type_name) == 0) {
			break;
		}
	}

	return _dt_etw_types[i].det_type_name != NULL ? i : (-1);
}

const char *
dt_etw_trace_type_name(int typeidx)
{
	int size = 0;
	for (int i = 0; _dt_etw_types[i].det_type_name != NULL; i++)
		size++; /* count up type name size */

	return typeidx < size ?
		_dt_etw_types[typeidx].det_type_name : NULL;
}

/*
 * Returns -1 if the node's type is not compatible/conversible
 * to the supplied etw trace type index.
 * Returns -2 if the node's type is not supported/implemented.
 */
int
dt_etw_trace_type_compat(int typeidx, dt_node_t *dnp)
{
	/* sanity check that the supplied type index is valid */
	int result = dt_etw_trace_type_name(typeidx) == NULL ? -1 : 1;

	if (result == 1) {
		if (_dt_etw_types[typeidx].det_check != NULL) {
			result = _dt_etw_types[typeidx].det_check(dnp);
		} else {
			result = -2;
		}
	}

	return result;
}

bool
parse_guid(char *guid_string, GUID *guid)
{
	return guid_string != NULL &&
		::UuidFromStringA((unsigned char*)guid_string, guid) == RPC_S_OK;
}

struct guid_less
{
    bool
    operator()(
        _In_ REFGUID Left,
        _In_ REFGUID Right
        ) const
    {
        RPC_STATUS status;
        return UuidCompare(const_cast<GUID*>(&Left), const_cast<GUID*>(&Right), &status) < 0;
    }
};

/*
 * Global map of ETW TraceLogging providers.
 * The key to an entry on this map is a trace provider GUID.
 * When adding a new provider, its construction enables it with ETW.
 * On map scope exit, all providers will get destroyed and disabled.
 */
std::map<GUID, std::shared_ptr<tld::Provider>, guid_less> _trace_providers;

std::shared_ptr<tld::Provider>
dt_etw_trace_provider(const char *provider_name, char *provider_id,
	char *provider_group)
{
	try
	{
		GUID provider_guid;

		if (!parse_guid(provider_id, &provider_guid))
			return {}; /* Return invalid pointer on parse guid error */

		GUID provider_group_guid;
		bool valid_group = parse_guid(provider_group, &provider_group_guid);

		if (_trace_providers.find(provider_guid) != _trace_providers.end())
			return _trace_providers[provider_guid];

		_trace_providers.emplace(provider_guid,
			std::make_shared<tld::Provider>(
				provider_name,
				provider_guid,
				valid_group ? &provider_group_guid : NULL));

		return _trace_providers[provider_guid];
	}
	catch (...)
	{
		/* On any error return an invalid provider */
		return {};
	}
}

/*
 * Prints into the D's libdtrace output buffer a representation of an
 * etw trace descriptor.
 */
void
dt_etw_trace_fprintf(dtrace_hdl_t *dtp, FILE *fp, dt_etw_trace_desc_t *trace)
{
	dt_printf(dtp, fp, "\netw trace descriptor:\n"
		"\tprovider name: %s\n"
		"\tprovider guid: %s\n"
		"%s"
		"\tevent name: %s\n"
		"\tlevel: %u\n"
		"\tkeywords: 0x%016llx\n"
		"\tpayload count: %u\n",
		trace->det_provider_name,
		trace->det_provider_guid,
		trace->det_provider_group_guid != NULL ?
			trace->det_provider_group_guid : "",
		trace->det_event_name,
		trace->det_level,
		trace->det_keyword,
		trace->det_plcount);

	dt_etw_trace_payload_t *plarr = DT_ETW_TRACE_PLARR_PTR(trace);
	for (int i = 0; i < trace->det_plcount; i++) {
		dt_printf(dtp, fp, "\t\tpayload #%d type: %s\n"
			"\t\tpayload #%d name: %s\n",
			i + 1,
			dt_etw_trace_type_name(trace->det_pl[i].det_pltype),
			i + 1,
			trace->det_pl[i].det_plname);
	}
}

/*
 * This function validates that the supplied etw_struct payload count
 * is valid given the node it starts with.
 *
 * If this function is called on a nested struct check, returns a pointer
 * to the payload type node right after the struct ends. Otherwise, returns NULL.
 */
dt_node_t *
dt_etw_trace_check_struct_payload(dt_node_t *dnp, int argi, dt_node_t *pl_value)
{
	/*
	 * We are guaranteed to have a valid count of payload tuples.
	 */

	if (pl_value->dn_kind == DT_NODE_VAR) {
		dnerror(pl_value, D_ETW_TRACE_PAYLOAD,
			"%s( ) argument #%d must not be a variable\n",
			dnp->dn_ident->di_name, argi);
	}

	int struct_pl_size = pl_value->dn_value;
	int pl_count = 0;
	dt_node_t *anp = pl_value->dn_list;

	while (anp != NULL) {
		dt_node_t *pl_type = anp;
		assert(pl_type != NULL);

		dt_node_t *pl_name = pl_type->dn_list;
		assert(pl_name != NULL);

		dt_node_t *pl_value = pl_name->dn_list;
		assert(pl_value != NULL);

		if (dt_node_is_string(pl_type) &&
			strcmp("etw_struct", pl_type->dn_string) == 0) {
			anp = dt_etw_trace_check_struct_payload(dnp,
				argi + DT_ETW_TRACE_PAYLOAD_TUPLE_COUNT, pl_value);
		} else {
			anp = pl_value->dn_list;
		}

		++pl_count;

		if (pl_count == struct_pl_size)
			break;
	}

	if (pl_count < struct_pl_size) {
		dnerror(pl_value, D_ETW_TRACE_PAYLOAD,
			"%s( ) argument #%d payload struct size (%d) cannot be satisfied "
			"with the remaining payload count (%d)\n",
			dnp->dn_ident->di_name, argi, struct_pl_size, pl_count);
	}

	return anp;
}

/*
 * Creates an etw trace descriptor based on the provided node.
 * The returned etw trace descriptor MUST be used on a call to
 * dt_etw_trace_validate after creating it to ensure proper fixup
 * of internal pointers.
 *
 * The implementation of this function sets string pointers to the strings
 * allocated for each node, and must be changed to point to the etw trace's
 * internal data string table byte blob. This pointer shuffling is handled
 * by dt_etw_trace_validate.
 */
dt_etw_trace_desc_t *
dt_etw_trace_create(dtrace_hdl_t *dtp, dt_node_t *dnp)
{
	/*
	 * Validate argument count and common parameters to etw_trace.
	 */

	dt_etw_trace_desc_t ph_trace; /* placeholder etw trace descriptor */
	bzero(&ph_trace, sizeof (ph_trace));

	int argc = 0; /* argument count */
	int argi = 0; /* argument index */
	dt_node_t *anp;

	for (anp = dnp->dn_args; anp != NULL; anp = anp->dn_list)
		++argc; /* count up arguments */

	dt_etw_trace_desc_t *trace = NULL;
	dt_etw_trace_payload_t *plarr = NULL;
	void *raw_data = NULL;

	dt_node_t *provider_name;
	{
		argi++;

		if ((argi > argc) || dnp->dn_args == NULL) {
			dnerror(dnp, D_ETW_TRACE_PARAMS,
				"%s( ) argument #%d missing - provider name\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((provider_name = dnp->dn_args) != NULL &&
			!dt_node_is_string(provider_name)) {
			dnerror(provider_name, D_ETW_TRACE_PNAME,
				"%s( ) argument #%d must be a string\n",
				dnp->dn_ident->di_name, argi);
		}

		if (provider_name->dn_kind == DT_NODE_VAR) {
			dnerror(provider_name, D_ETW_TRACE_PNAME,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		ph_trace.det_provider_name_length = strlen(provider_name->dn_string) + 1;
		ph_trace.det_string_table_size = ph_trace.det_provider_name_length
			* sizeof (char); /* increase the string table size */
		ph_trace.det_provider_name = provider_name->dn_string;
	}

	dt_node_t *provider_guid;
	{
		argi++;

		if ((argi > argc) || provider_name->dn_list == NULL) {
			dnerror(dnp, D_ETW_TRACE_PARAMS,
				"%s( ) argument #%d missing - provider guid\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((provider_guid = provider_name->dn_list) != NULL &&
			!dt_node_is_string(provider_guid)) {
			dnerror(provider_guid, D_ETW_TRACE_PGUID,
				"%s( ) argument #%d must be a string\n",
				dnp->dn_ident->di_name, argi);
		}

		if (provider_guid->dn_kind == DT_NODE_VAR) {
			dnerror(provider_guid, D_ETW_TRACE_PGUID,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		ph_trace.det_provider_guid_length = strlen(provider_guid->dn_string) + 1;
		ph_trace.det_string_table_size += ph_trace.det_provider_guid_length
			* sizeof (char); /* increase the string table size */
		ph_trace.det_provider_guid = provider_guid->dn_string;
	}

	dt_node_t *provider_group_guid;
	{
		argi++;

		if ((argi > argc) || provider_guid->dn_list == NULL) {
			dnerror(dnp, D_ETW_TRACE_PARAMS,
				"%s( ) argument #%d missing - event name or provider group guid\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((provider_group_guid = provider_guid->dn_list) != NULL &&
			!dt_node_is_string(provider_group_guid)) {
			dnerror(provider_group_guid, D_ETW_TRACE_ENAME,
				"%s( ) argument #%d must be a string\n",
				dnp->dn_ident->di_name, argi);
		}

		if (provider_group_guid->dn_kind == DT_NODE_VAR) {
			dnerror(provider_group_guid, D_ETW_TRACE_ENAME,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		GUID group_guid;
		if (parse_guid(provider_group_guid->dn_string, &group_guid)) {
			/*
			 * This is a valid GUID. Treat this as a supplied group GUID.
			 */
			ph_trace.det_provider_group_guid_length =
				strlen(provider_group_guid->dn_string) + 1;
			ph_trace.det_string_table_size +=
				ph_trace.det_provider_group_guid_length * sizeof (char);
			ph_trace.det_provider_group_guid = provider_group_guid->dn_string;
		} else {
			/*
			 * This is not a valid GUID. Treat this as a not supplied group GUID
			 * and decrease the argument index, so that the next parameter is
			 * event name.
			 */
			ph_trace.det_provider_group_guid_length = 0;
			ph_trace.det_provider_group_guid = NULL;
			provider_group_guid = NULL;
			argi--;
		}
	}

	dt_node_t *event_name;
	{
		argi++;

		dt_node_t *previous_node = provider_group_guid != NULL ?
			provider_group_guid : provider_guid;

		if ((argi > argc) || previous_node->dn_list == NULL) {
			dnerror(dnp, D_ETW_TRACE_PARAMS,
				"%s( ) argument #%d missing - event name\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((event_name = previous_node->dn_list) != NULL &&
			!dt_node_is_string(event_name)) {
			dnerror(event_name, D_ETW_TRACE_ENAME,
				"%s( ) argument #%d must be a string\n",
				dnp->dn_ident->di_name, argi);
		}

		if (event_name->dn_kind == DT_NODE_VAR) {
			dnerror(event_name, D_ETW_TRACE_ENAME,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		ph_trace.det_event_name_length = strlen(event_name->dn_string) + 1;
		ph_trace.det_string_table_size += ph_trace.det_event_name_length
			* sizeof (char); /* increase the string table size */
		ph_trace.det_event_name = event_name->dn_string;
	}

	dt_node_t *level;
	{
		argi++;

		if ((argi > argc) || event_name->dn_list == NULL) {
			dnerror(dnp, D_ETW_TRACE_PARAMS,
				"%s( ) argument #%d missing - event level\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((level = event_name->dn_list) != NULL &&
			!dt_node_is_integer(level)) {
			dnerror(level, D_ETW_TRACE_LEVEL,
				"%s( ) argument #%d must be an integer\n",
				dnp->dn_ident->di_name, argi);
		}

		if (level->dn_kind == DT_NODE_VAR) {
			dnerror(level, D_ETW_TRACE_LEVEL,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		if (level->dn_value > WINEVENT_LEVEL_VERBOSE) {
			dnerror(level, D_ETW_TRACE_LEVEL,
				"%s( ) argument #%d must be a valid integer "
				"between 0 and 5\tsupplied level: %d\n",
				dnp->dn_ident->di_name, argi, level->dn_value);
		}

		ph_trace.det_level = level->dn_value;
	}

	dt_node_t *keyword;
	{
		argi++;

		if ((argi > argc) || level->dn_list == NULL) {
			dnerror(dnp, D_ETW_TRACE_PARAMS,
				"%s( ) argument #%d missing - event keywords\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((keyword = level->dn_list) != NULL &&
			!dt_node_is_integer(keyword)) {
			dnerror(keyword, D_ETW_TRACE_KEYWORD,
				"%s( ) argument #%d must be an integer\n",
				dnp->dn_ident->di_name, argi);
		}

		if (keyword->dn_kind == DT_NODE_VAR) {
			dnerror(keyword, D_ETW_TRACE_KEYWORD,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		ph_trace.det_keyword = keyword->dn_value;
	}

	if ((argc - argi) % DT_ETW_TRACE_PAYLOAD_TUPLE_COUNT != 0) {
		dnerror(dnp, D_ETW_TRACE_PAYLOAD,
			"%s( ) parameter count does not match expected payload format\n"
			"\tpayload parameters\n"
			"\t\ttype\n"
			"\t\tname\n"
			"\t\tvalue\n",
			dnp->dn_ident->di_name);
	}

	/*
	 * Iterate in tuples of payload arguments to validate them.
	 * Because we checked earlier in the function that the argument
	 * count after the common parameters is divisable by the payload
	 * tuple count, we can assert there are enough nodes to form valid
	 * payload descriptors.
	 *
	 * As we move forward validating payload, start
	 * constructing the payload descriptor array for the trace descriptor
	 * that will be returned at the end.
	 */

	int plidx = 0; /* current index to payload entry */
	ph_trace.det_plcount = (argc - argi) /
		DT_ETW_TRACE_PAYLOAD_TUPLE_COUNT;
	size_t plarr_size = ph_trace.det_plcount *
		sizeof (dt_etw_trace_payload_t);

	if ((raw_data = dt_alloc(dtp, plarr_size)) == NULL) {
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
	}

	plarr = (dt_etw_trace_payload_t *)raw_data;
	anp = keyword->dn_list;

	while (anp != NULL) {
		dt_node_t *pl_type = anp;
		assert(pl_type != NULL);

		dt_node_t *pl_name = pl_type->dn_list;
		assert(pl_name != NULL);

		dt_node_t *pl_value = pl_name->dn_list;
		assert(pl_value != NULL);

		argi++; /* payload type */

		if (!dt_node_is_string(pl_type)) {
			dt_free(dtp, plarr);
			dnerror(pl_type, D_ETW_TRACE_PAYLOAD,
				"%s( ) argument #%d must be a string\n",
				dnp->dn_ident->di_name, argi);
		}

		if (pl_type->dn_kind == DT_NODE_VAR) {
			dnerror(pl_type, D_ETW_TRACE_PAYLOAD,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		if ((plarr[plidx].det_pltype = dt_etw_trace_type_lookup(
			pl_type->dn_string)) == -1) {
			dt_free(dtp, plarr);
			dnerror(pl_type, D_ETW_TRACE_PAYLOAD,
				"%s( ) argument #%d is not a supported etw type\n",
				dnp->dn_ident->di_name, argi);
		}

		argi++; /* payload name */

		if (!dt_node_is_string(pl_name)) {
			dt_free(dtp, plarr);
			dnerror(pl_name, D_ETW_TRACE_PAYLOAD,
				"%s( ) argument #%d must be a string\n",
				dnp->dn_ident->di_name, argi);
		}

		if (pl_name->dn_kind == DT_NODE_VAR) {
			dnerror(pl_name, D_ETW_TRACE_PAYLOAD,
				"%s( ) argument #%d must not be a variable\n",
				dnp->dn_ident->di_name, argi);
		}

		plarr[plidx].det_plname_length = strlen(pl_name->dn_string) + 1;
		ph_trace.det_string_table_size += plarr[plidx].det_plname_length
			* sizeof (char); /* increase the string table size */
		plarr[plidx].det_plname = pl_name->dn_string;

		argi++; /* payload value */

		int compat_result = dt_etw_trace_type_compat(plarr[plidx].det_pltype,
			pl_value);

		if (compat_result == -1) {
			dt_free(dtp, plarr);
			dnerror(pl_value, D_ETW_TRACE_PAYLOADTYPE,
				"%s( ) argument #%d is not compatible "
				"with payload type specified in argument #%d \"%s\"\n",
				dnp->dn_ident->di_name, argi, argi - 2, pl_type->dn_string);
		} else if (compat_result == -2) {
			dt_free(dtp, plarr);
			dnerror(pl_value, D_ETW_TRACE_PAYLOADTYPE,
				"%s( ) argument #%d is not supported/implemented "
				"with payload type specified in argument #%d \"%s\"\n",
				dnp->dn_ident->di_name, argi, argi - 2, pl_type->dn_string);
		}

		/*
		 * If the payload type is a struct, then we need to validate
		 * that the remainder of the payload in the etw_trace can satisfy
		 * the struct size specified.
		 */
		if (plarr[plidx].det_pltype == 0)
			dt_etw_trace_check_struct_payload(dnp, argi, pl_value);

		anp = pl_value->dn_list;
		plidx++;
	}

	/*
	 * Now that the payload array has been validated and constructed,
	 * allocate the new trace descriptor and fill it in. At this point,
	 * we should know the exact byte size we need for the etw trace data.
	 *
	 * First, allocate enough memory to fit the descriptor struct itself,
	 * the string table and the payload array.
	 *
	 * Second, copy the entirity of the placeholder struct object,
	 * which populates the length/size/count metadata members.
	 *
	 * Third, copy the entire payload array to the start of the data buffer.
	 * It's safe to release the placeholder plarr variable since its contents
	 * have been copied successfully.
	 *
	 * Lastly, copy the individual strings into the string table, which has been
	 * correctly sized to fit all of them as NULL-terminated strings.
	 * The string table sits at the last chunk of the data buffer, right after
	 * the payload entries array.
	 * In this step, we need to iterate over the payload array to copy each
	 * payload's name string.
	 */

	if ((raw_data = dt_alloc(dtp, DT_ETW_TRACE_BSIZE(&ph_trace))) == NULL) {
		dt_free(dtp, plarr);
		longjmp(yypcb->pcb_jmpbuf, EDT_NOMEM);
	}

	trace = (dt_etw_trace_desc_t *)raw_data;

	dt_etw_trace_payload_t *payload = DT_ETW_TRACE_PLARR_PTR(trace);
	bcopy(&ph_trace, trace, sizeof (dt_etw_trace_desc_t));
	bcopy(plarr, payload, plarr_size);
	dt_free(dtp, plarr);

	char *str_offset = DT_ETW_TRACE_STRINGTAB_PTR(trace);

/*
 * Macro that ensures a proper byte copy of a string and that the
 * string offset is advanced. The size is assumed to include the NULL termination
 * character, and then explicitly set at the end of the string.
 */
#define COPY_ETW_TRACE_DESC_STRING(_src_, _sz_) \
	bcopy(_src_, str_offset, ((_sz_) - 1) * sizeof (char)); \
	str_offset[(_sz_) - 1] = '\0'; \
	str_offset += _sz_ /* explicitly avoid ; to force callers to put it */

	COPY_ETW_TRACE_DESC_STRING(trace->det_provider_name,
		trace->det_provider_name_length);
	COPY_ETW_TRACE_DESC_STRING(trace->det_provider_guid,
		trace->det_provider_guid_length);
	COPY_ETW_TRACE_DESC_STRING(trace->det_event_name,
		trace->det_event_name_length);

	if (trace->det_provider_group_guid_length != 0) {
		COPY_ETW_TRACE_DESC_STRING(trace->det_provider_group_guid,
			trace->det_provider_group_guid_length);
	}

	for (int i = 0; i < trace->det_plcount; i++) {
		COPY_ETW_TRACE_DESC_STRING(payload[i].det_plname,
			payload[i].det_plname_length);
	}

	/*
	 * Assert that the last string added correctly finished the string offset
	 * at the end of the string table.
	 */
	assert((str_offset - DT_ETW_TRACE_STRINGTAB_PTR(trace)) ==
		DT_ETW_TRACE_STRINGTAB_BSIZE(trace));

	auto provider = dt_etw_trace_provider(trace->det_provider_name,
		trace->det_provider_guid, trace->det_provider_group_guid);
	assert(provider);

	return trace;
}

void
dt_etw_trace_destroy(dtrace_hdl_t *dtp, dt_etw_trace_desc_t *trace)
{
	GUID provider_guid;

	if (parse_guid(trace->det_provider_guid, &provider_guid)) {
		auto it = _trace_providers.find(provider_guid);

		if (it != _trace_providers.end())
			it->second.reset();
	}

	dt_free(dtp, trace);
}

/*
 * This function fixes up internal pointer members of an etw trace descriptor
 * to refer to the right entries within the embedded data byte blob.
 * This MUST be called after an etw trace descriptor has been retrieved from
 * another routine and it has been byte copied. This is true when DTrace
 * loads the etw trace descriptor from the kernel via an IOCTL and when
 * loading it from the DOF metadata.
 */
void
dt_etw_trace_validate(dt_etw_trace_desc_t *trace)
{
	/*
	 * The supplied ewt trace descriptor is assumed to have correct
	 * data byte blob, and containing the correct entries for the pointer
	 * fixup.
	 */

	trace->det_pl = DT_ETW_TRACE_PLARR_PTR(trace);
	char *str_offset = DT_ETW_TRACE_STRINGTAB_PTR(trace);

/*
 * Macro that ensures a pointer to the current string offset is set
 * to the supplied member and then advanced by the supplied string length.
 */
#define GET_ETW_TRACE_DESC_STRING(_member_, _sz_) \
	_member_ = str_offset; \
	str_offset += _sz_ /* explicitly avoid ; to force callers to put it */

	GET_ETW_TRACE_DESC_STRING(trace->det_provider_name,
		trace->det_provider_name_length);
	GET_ETW_TRACE_DESC_STRING(trace->det_provider_guid,
		trace->det_provider_guid_length);
	GET_ETW_TRACE_DESC_STRING(trace->det_event_name,
		trace->det_event_name_length);

	if (trace->det_provider_group_guid_length != 0) {
		GET_ETW_TRACE_DESC_STRING(trace->det_provider_group_guid,
			trace->det_provider_group_guid_length);
	} else {
		trace->det_provider_group_guid = NULL;
	}

	for (int i = 0; i < trace->det_plcount; i++) {
		GET_ETW_TRACE_DESC_STRING(trace->det_pl[i].det_plname,
			trace->det_pl[i].det_plname_length);
	}
}

/*
 * Macro used to add event trace payload data into the Event builder,
 * which also returns early if there is any type of error.
 */
#define DT_ETW_TRACE_ADD_EVENT_PAYLOAD_DATA() \
	do \
	{ \
		if (_dt_etw_types[trace->det_pl[i].det_pltype].det_add == NULL) { \
			dt_printf(dtp, fp, "\netw trace skipped, payload \"%s\" failed " \
				" to be processed " \
				"for event \"%s\" from provider \"%s\" - \"%s\"\n", \
				trace->det_pl[i].det_plname, \
				trace->det_event_name, \
				trace->det_provider_name, \
				trace->det_provider_guid); \
			return (int)trace->det_plcount; \
		} \
		if (_dt_etw_types[trace->det_pl[i].det_pltype].det_add( \
			{ \
				event, \
				trace->det_pl[i].det_plname, \
				_dt_etw_types[trace->det_pl[i].det_pltype].det_tld_type, \
				(caddr_t)buf + recp[i].dtrd_offset, \
				recp[i].dtrd_size \
			}) == -1) { \
			dt_printf(dtp, fp, "\netw trace skipped, payload \"%s\" failed " \
				" to be added to the etw trace metadata " \
				"for event \"%s\" from provider \"%s\" - \"%s\"\n", \
				trace->det_pl[i].det_plname, \
				trace->det_event_name, \
				trace->det_provider_name, \
				trace->det_provider_guid); \
			return (int)trace->det_plcount; \
		} \
	} while (false)

int
dt_etw_trace_struct(dtrace_hdl_t *dtp, FILE *fp, dt_etw_trace_desc_t *trace,
	tld::EventBuilder<std::vector<BYTE>> &parent_event, int idx,
	const dtrace_recdesc_t *recp, const void *buf, size_t len)
{
	auto event = parent_event.AddStruct(trace->det_pl[idx].det_plname);
	int struct_sz = *(uint32_t *)((caddr_t)buf + recp[idx].dtrd_offset);
	int i;
	int count;

	for (i = idx + 1, count = 0;
		i < trace->det_plcount && count < struct_sz; i++, count++) {
		if (trace->det_pl[i].det_pltype == 0) { /* etw_struct */
			i = dt_etw_trace_struct(dtp, fp, trace, event, i, recp, buf, len);
			continue;
		}

		DT_ETW_TRACE_ADD_EVENT_PAYLOAD_DATA();
	}

	return i - 1;
}

/*
 * This function gets called when a probe fires an action that corresponds
 * to an etw_trace call in the D script.
 * This is what will actually call the Windows ETW TraceLogging APIs
 * to fire an event and send it to the ETW pipeline.
 */
int
dt_etw_trace(dtrace_hdl_t *dtp, FILE *fp, dt_etw_trace_desc_t *trace,
	const dtrace_recdesc_t *recp, uint_t nrecs, const void *buf, size_t len)
{
	try
	{
		int i = 0;

		if (trace->det_plcount > nrecs)
			return (dt_set_errno(dtp, EDT_DMISMATCH));

		for (i = 0; i < trace->det_plcount; i++) {
			if (recp[i].dtrd_action != DTRACEACT_ETWTRACE)
				return (dt_set_errno(dtp, EDT_DMISMATCH));
		}

		/* Uncomment for debugging purposes */
		// dt_etw_trace_fprintf(dtp, fp, trace);

		auto provider = dt_etw_trace_provider(trace->det_provider_name,
			trace->det_provider_guid, trace->det_provider_group_guid);

		if (!provider) {
			dt_printf(dtp, fp,
				"\nskipping etw trace, the provider is not valid [\"%s\" - %s]",
				trace->det_provider_name, trace->det_provider_guid);
			return (int)trace->det_plcount;
		}


		/*
		 * For optimization, skip an etw trace if its provider is not
		 * being listened. This will prevent us from building the event
		 * metadata and make things faster since the trace won't be captured.
		 */
		if (!provider->IsEnabled())
			return (int)trace->det_plcount;

		tld::Event<std::vector<BYTE>> event(trace->det_event_name,
			trace->det_level, trace->det_keyword);

		/*
		 * Because of the compile time guarantees, we can use safely
		 * the payload metadata knowing it's pointing to valid entries
		 * to the global etw types map.
		 *
		 * However, if there is a failure of any kind processing the events,
		 * instead of catastrophically failing we log a message stating on what
		 * payload the failure happened and skip this trace.
		 */
		for (i = 0; i < trace->det_plcount; i++) {
			if (trace->det_pl[i].det_pltype == 0) { /* etw_struct */
				i = dt_etw_trace_struct(dtp, fp, trace, event, i, recp, buf, len);
				continue;
			}

			DT_ETW_TRACE_ADD_EVENT_PAYLOAD_DATA();
		}

		event.Write(*provider);
		dt_printf(dtp, fp,
			"\nlogged etw trace \"%s\" from provider [\"%s\" %s]",
			trace->det_event_name,
			trace->det_provider_name,
			trace->det_provider_guid);

		/*
		* If we reach here, it means that we processed all the records
		* associated to the etw_trace call.
		*/
		return (int)trace->det_plcount;
	}
	catch (...)
	{
		/*
		 * Treat thrown exception in this routine as a catastophric error,
		 * since those shouldn't happen even if error paths.
		 */
		return (dt_set_errno(dtp, EDT_ETWTRACEFAIL));
	}
}

#endif /* _WIN32 */
