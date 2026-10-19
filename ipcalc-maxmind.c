/*
 * Copyright (c) 2018 Red Hat, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Martin Sehnoutka <msehnout@redhat.com>
 */

#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ipcalc.h"

#ifdef USE_MAXMIND
#include <maxminddb.h>

#ifndef MAXMINDDB_LOCATION_COUNTRY
#define MAXMINDDB_LOCATION_COUNTRY "/usr/share/GeoIP/GeoLite2-Country.mmdb"
#endif

#ifndef MAXMINDDB_LOCATION_CITY
#define MAXMINDDB_LOCATION_CITY "/usr/share/GeoIP/GeoLite2-City.mmdb"
#endif

#ifndef MAXMINDDB_LOCATION_ASN
#define MAXMINDDB_LOCATION_ASN "/usr/share/GeoIP/GeoLite2-ASN.mmdb"
#endif

#ifdef USE_RUNTIME_LINKING
# include <dlfcn.h>
# define LIBNAME LIBPATH"/libmaxminddb.so.0"

typedef int (*MMDB_open_fn)(const char *const filename, uint32_t flags,
			    MMDB_s *const mmdb);
typedef void (*MMDB_close_fn)(MMDB_s *const mmdb);
typedef MMDB_lookup_result_s (*MMDB_lookup_string_fn)(MMDB_s *const mmdb,
						      const char *const ipstr,
						      int *const gai_error,
						      int *const mmdb_error);
typedef int (*MMDB_aget_value_fn)(MMDB_entry_s *const start,
				  MMDB_entry_data_s *const entry_data,
				  const char *const *const path);

static MMDB_close_fn pMMDB_close;
static MMDB_aget_value_fn pMMDB_aget_value;
static MMDB_lookup_string_fn pMMDB_lookup_string;
static MMDB_open_fn pMMDB_open;

int geo_setup(void)
{
	static void *ld = NULL;
	static int ret = 0;
	static char err[512] = {0};

	if (ld != NULL || ret != 0) {
		if (!beSilent && err[0] != 0)
			fprintf(stderr, "%s", err);
		return ret;
	}

	ld = dlopen(LIBNAME, RTLD_LAZY);
	if (ld == NULL) {
		snprintf(err, sizeof(err),
			 "ipcalc: could not open %s; geographic information requires libmaxminddb and the GeoLite2 databases, install them if they are missing\n",
			 LIBNAME);
		ret = -1;
		return ret;
	}

	pMMDB_close = dlsym(ld, "MMDB_close");
	pMMDB_aget_value = dlsym(ld, "MMDB_aget_value");
	pMMDB_lookup_string = dlsym(ld, "MMDB_lookup_string");
	pMMDB_open = dlsym(ld, "MMDB_open");

	if (pMMDB_close == NULL || pMMDB_aget_value == NULL ||
	    pMMDB_lookup_string == NULL || pMMDB_open == NULL) {
		snprintf(err, sizeof(err), "ipcalc: could not find symbols in libmaxmind\n");
		ret = -1;
	}

	return ret;
}

#else
#define pMMDB_close MMDB_close
#define pMMDB_aget_value MMDB_aget_value
#define pMMDB_lookup_string MMDB_lookup_string
#define pMMDB_open MMDB_open
#endif /* USE_RUNTIME_LINKING */

static int get_entry_data(MMDB_entry_s *entry, const char *const *path,
			  uint32_t type, MMDB_entry_data_s *data)
{
	return pMMDB_aget_value(entry, data, path) == MMDB_SUCCESS &&
	       data->has_data && data->type == type;
}

/* NULL when the record has no string at path */
static char *get_string(MMDB_entry_s *entry, const char *const *path)
{
	MMDB_entry_data_s data;
	char *str = NULL;

	/* MaxMind DB strings are not NUL-terminated */
	if (get_entry_data(entry, path, MMDB_DATA_TYPE_UTF8_STRING, &data))
		safe_asprintf(&str, "%.*s", (int)data.data_size, data.utf8_string);
	return str;
}

/* NULL when the record has no unsigned integer at path */
static char *get_unsigned(MMDB_entry_s *entry, const char *const *path)
{
	MMDB_entry_data_s data;
	char *str = NULL;

	if (get_entry_data(entry, path, MMDB_DATA_TYPE_UINT16, &data))
		safe_asprintf(&str, "%u", (unsigned)data.uint16);
	else if (get_entry_data(entry, path, MMDB_DATA_TYPE_UINT32, &data))
		safe_asprintf(&str, "%u", (unsigned)data.uint32);
	return str;
}

/* Finds the record of ip, when it covers the whole network ip/prefix, and
 * the prefix of its network; on success the caller must close mmdb */
static int lookup(const char *file, const char *ip, unsigned prefix,
		  MMDB_s *mmdb, MMDB_lookup_result_s *result,
		  unsigned *record_prefix)
{
	int gai_error, mmdb_error;

	if (pMMDB_open(file, MMDB_MODE_MMAP, mmdb) != MMDB_SUCCESS)
		return -1;

	/* mmdb_error is not set when gai_error is */
	*result = pMMDB_lookup_string(mmdb, ip, &gai_error, &mmdb_error);
	if (gai_error != 0 || mmdb_error != MMDB_SUCCESS ||
	    !result->found_entry) {
		pMMDB_close(mmdb);
		return -1;
	}

	*record_prefix = result->netmask;
	/* IPv4 records are in the ::/96 subtree of an IPv6 database */
	if (strchr(ip, ':') == NULL && mmdb->metadata.ip_version == 6)
		*record_prefix = *record_prefix > 96 ? *record_prefix - 96 : 0;
	if (prefix < *record_prefix) {
		pMMDB_close(mmdb);
		return -1;
	}

	return 0;
}

/* The network ip/prefix in CIDR notation */
static char *record_network(const char *ip, unsigned prefix)
{
	unsigned char addr[sizeof(struct in6_addr)];
	char str[INET6_ADDRSTRLEN];
	int family = strchr(ip, ':') ? AF_INET6 : AF_INET;
	unsigned i, len = family == AF_INET6 ? 16 : 4;
	char *network = NULL;

	if (inet_pton(family, ip, addr) != 1)
		return NULL;

	for (i = 0; i < len; i++) {
		if (prefix <= i * 8)
			addr[i] = 0;
		else if (prefix < (i + 1) * 8)
			addr[i] &= 0xff << (8 - (prefix - i * 8));
	}

	if (inet_ntop(family, addr, str, sizeof(str)) == NULL)
		return NULL;
	safe_asprintf(&network, "%s/%u", str, prefix);
	return network;
}

/* A missing database or record is not an error: the values it would
 * provide are left NULL */
void geo_ip_lookup(const char *ip, unsigned prefix, struct geo_info_st *geo)
{
	static const char *const country_keys[] = {
		"country", "represented_country", "registered_country"
	};
	static const char *const continent_path[] = { "continent", "names", "en", NULL };
	static const char *const city_path[] = { "city", "names", "en", NULL };
	static const char *const postal_path[] = { "postal", "code", NULL };
	static const char *const latitude_path[] = { "location", "latitude", NULL };
	static const char *const longitude_path[] = { "location", "longitude", NULL };
	static const char *const radius_path[] = { "location", "accuracy_radius", NULL };
	static const char *const time_zone_path[] = { "location", "time_zone", NULL };
	static const char *const asn_path[] = { "autonomous_system_number", NULL };
	static const char *const as_org_path[] = { "autonomous_system_organization", NULL };
	MMDB_s mmdb;
	MMDB_lookup_result_s result;
	MMDB_entry_data_s latitude, longitude;
	unsigned i, record_prefix, network_prefix = 0;
	int found = 0;

	if (geo_setup() != 0)
		return;

	if (lookup(MAXMINDDB_LOCATION_COUNTRY, ip, prefix, &mmdb, &result,
		   &record_prefix) == 0) {
		found = 1;
		network_prefix = record_prefix;
		geo->continent = get_string(&result.entry, continent_path);
		for (i = 0; i < sizeof(country_keys) / sizeof(country_keys[0]) &&
		     !geo->country && !geo->ccode; i++) {
			const char *const name_path[] = { country_keys[i], "names", "en", NULL };
			const char *const code_path[] = { country_keys[i], "iso_code", NULL };

			geo->country = get_string(&result.entry, name_path);
			geo->ccode = get_string(&result.entry, code_path);
		}
		pMMDB_close(&mmdb);
	}

	if (lookup(MAXMINDDB_LOCATION_CITY, ip, prefix, &mmdb, &result,
		   &record_prefix) == 0) {
		found = 1;
		/* the smallest network is the one where all records hold */
		if (record_prefix > network_prefix)
			network_prefix = record_prefix;
		geo->city = get_string(&result.entry, city_path);
		geo->postal_code = get_string(&result.entry, postal_path);
		if (get_entry_data(&result.entry, latitude_path,
				   MMDB_DATA_TYPE_DOUBLE, &latitude) &&
		    get_entry_data(&result.entry, longitude_path,
				   MMDB_DATA_TYPE_DOUBLE, &longitude))
			safe_asprintf(&geo->coord, "%f,%f",
				      latitude.double_value,
				      longitude.double_value);
		geo->accuracy_radius = get_unsigned(&result.entry, radius_path);
		geo->time_zone = get_string(&result.entry, time_zone_path);
		pMMDB_close(&mmdb);
	}

	if (lookup(MAXMINDDB_LOCATION_ASN, ip, prefix, &mmdb, &result,
		   &record_prefix) == 0) {
		found = 1;
		if (record_prefix > network_prefix)
			network_prefix = record_prefix;
		geo->asn = get_unsigned(&result.entry, asn_path);
		geo->as_org = get_string(&result.entry, as_org_path);
		geo->as_network = record_network(ip, record_prefix);
		pMMDB_close(&mmdb);
	}

	if (found)
		geo->network = record_network(ip, network_prefix);
}

#endif /* USE_MAXMIND */
