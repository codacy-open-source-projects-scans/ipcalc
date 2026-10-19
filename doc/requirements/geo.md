---
title: Geographic information (-g/--geoinfo)
id-prefix: REQ-GEO
sources:
  - ipcalc-maxmind.c
  - ipcalc.h (geo_ip_lookup, geo_setup stubs)
  - ipcalc.c (main, get_ipv4_info, get_ipv6_info, long_options, usage)
  - meson.build, meson_options.txt, Makefile
  - ipcalc.1.md
---

# Geographic information (`-g/--geoinfo`)

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
The lookups are tested against databases generated from
`tests/geo-*.json` (REQ-GEO-TEST-001), which use documentation
addresses: 192.0.2.0/24 and 2001:db8::/48 have a full City record,
198.51.100.0/24 has location data but no country, only a registered
or represented country in parts of it; 203.0.113.0/24 is only in the ASN
database.

### REQ-GEO-BUILD-001
**Requirement:** ipcalc MUST be buildable with libmaxminddb as its geo
backend, either linked at build time or loaded at run time with
`dlopen()`, or with no geo backend. libGeoIP, whose databases are no
longer published, MUST NOT be supported as a backend; the meson option
`use_geoip` MUST still be accepted and ignored, so that existing build
commands keep working.
**Strength:** MUST
**Status:** DERIVED
**Source:** meson.build (`use_maxminddb`, `use_runtime_linking`); meson_options.txt (`use_geoip`, unused); Makefile (`USE_MAXMIND`, `USE_RUNTIME_LINKING`); ipcalc.h (stubs when `USE_MAXMIND` is undefined)
**Acceptance:** each configuration in `.gitlab-ci.yml` builds with `-Werror` — positive ; `meson setup -Duse_maxminddb=disabled -Duse_geoip=enabled` builds without a geo backend (`-g` rejected) — positive ; test: CI jobs (fedora.nogeo passes `-Duse_geoip=enabled`; not a meson test)
**Links:** REQ-GEN-BUILD-001

### REQ-GEO-INPUT-001
**Requirement:** `-g`/`--geoinfo` MUST be accepted only when ipcalc is
built with a geo backend. In a build without one, both forms MUST be
rejected as unrecognized options (REQ-CLI-ERR-002, exit status 1), and
neither `--help` nor `--usage` MUST list them, so that a script relying
on geo output fails instead of silently getting none.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:long_options, ipcalc.c:usage, ipcalc.c:main (`GEO_SHORT_OPTION`, `case 'g'`) — all under `#ifdef USE_MAXMIND`; meson.build (`have_geo`); ipcalc.1.md#Options (`--geoinfo`)
**Acceptance:** build with a backend: `ipcalc -g 127.0.0.1` → exit 0 — positive ; build without a backend: `ipcalc -g 127.0.0.1`, `ipcalc --geoinfo 127.0.0.1` → exit 1; `ipcalc --usage` → does not mention `-g` or `--geoinfo` — negative ; test: GeoInfoAccepted (with a backend), GeoInfoShortUnavailable, GeoInfoLongUnavailable, GeoInfoNotInUsage (without a backend)
**Links:** REQ-CLI-ERR-002, REQ-CLI-COMPAT-002, REQ-GEO-ERR-001

### REQ-GEO-CALC-001
**Requirement:** With `-g`, or with `--all-info` when the backend is
usable, ipcalc MUST look up the input address (not the network address)
in the system geo databases, subject to REQ-GEO-CALC-002, and report
the English continent name (`continent/names/en`) and the country name
and ISO country code (REQ-GEO-CALC-003) from the Country database; the
English city name (`city/names/en`), the postal code (`postal/code`),
the `<latitude>,<longitude>` coordinates (printed with `%f`), the
accuracy radius of the coordinates in kilometers
(`location/accuracy_radius`), and the IANA time zone
(`location/time_zone`) from the City database; the autonomous system
number (`autonomous_system_number`) and organization
(`autonomous_system_organization`) from the ASN database; the network
of the records (REQ-GEO-CALC-004); and the network of the ASN record
(REQ-GEO-CALC-005); omitting any value not found.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info (`geo_ip_lookup(ipStr, …)`), ipcalc.c:main (`geo_setup() == 0 && FLAG_SHOW_ALL_INFO`); ipcalc-maxmind.c:geo_ip_lookup (GeoLite2-Country.mmdb, GeoLite2-City.mmdb, GeoLite2-ASN.mmdb under /usr/share/GeoIP by default); ipcalc.1.md#Options (`--geoinfo`, `--all-info`)
**Acceptance:** with a backend and databases: `ipcalc -g 193.92.150.2` → `COUNTRYCODE=GR`, `COUNTRY=Greece` (ipcalc.1.md example); with the test databases: `-g 192.0.2.5` → tests/geo-192.0.2.5 (with `CONTINENT=Europe`, `CITY=Athens`, `POSTALCODE="105 52"`, `ACCURACYRADIUS=200` and `TIMEZONE=Europe/Athens`, `ASN=64496`, `ASORG="Example Net"`), `-g 203.0.113.1` (only in the ASN database) → only `ASN`, `ASORG`, `ASNETWORK` and `GEONETWORK`, `-g 2001:db8::1` → tests/geo-2001:db8::1 (with `CITY=Berlin`), `-g 198.51.100.200` (no country of any kind, city, or postal code) → only `CONTINENT`, `COORDINATES`, `ACCURACYRADIUS=1000`, `TIMEZONE` and `GEONETWORK` — positive ; test: GeoInfoIPv4, GeoInfoIPv6, GeoInfoNoCountry, GeoInfoASNOnly
**Links:** REQ-CLI-OUTPUT-004, REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-003

### REQ-GEO-CALC-002
**Requirement:** For a network, ipcalc MUST report the values of a
database record only when the whole input network lies within the
network of that record, i.e. when the input prefix is not shorter than
the record's, and MUST omit them otherwise, since they are known only
for a part of the input network. This applies to the Country, City,
and ASN databases separately; for an IPv4 address in an IPv6 database, the
record's prefix is taken relative to the IPv4 subtree (`::/96`).
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-maxmind.c:lookup (`result->netmask` compared with the input prefix), ipcalc-maxmind.c:geo_ip_lookup; ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info (pass the prefix); ipcalc.1.md#Options (`--geoinfo`)
**Acceptance:** with the test databases (Country record 192.0.0.0/22, City and ASN records 192.0.2.0/24): `-g 192.0.2.0/24` → country, city, coordinates, radius and ASN; `-g 192.0.2.77/23` → only the continent, the country and `GEONETWORK` (no ASN); `-g 2001:db8::/32` (Country and ASN /32, City /48) → only the continent, the country, the ASN and `GEONETWORK` — positive ; `--all-info 192.0.0.0/16` → no geo lines, exit 0 — negative ; test: GeoInfoNetworkWithinRecords, GeoInfoNetworkLargerThanCity, GeoInfoNetworkLargerThanCityIPv6, GeoInfoNetworkLargerThanRecords
**Links:** REQ-GEO-CALC-001, REQ-CLI-COMPAT-005

### REQ-GEO-CALC-003
**Requirement:** When the Country database record has no `country`,
ipcalc MUST report the name and ISO code of its `represented_country`
(the country its users represent, e.g. for a military base) or, without
one, of its `registered_country` (the country in which the network is
registered) as the country, taking the name and the code from the same
one of the three; when the record has none of them, the country MUST be
omitted.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-maxmind.c:geo_ip_lookup (`country_keys`)
**Acceptance:** with the test databases: `-g 198.51.100.5` (only `registered_country` US) → `COUNTRYCODE=US`, `COUNTRY="United States"`; `-g 198.51.100.130` (`represented_country` US, `registered_country` DE) → `COUNTRYCODE=US` — positive ; `-g 198.51.100.200` (none of them) → no `COUNTRYCODE` — negative ; test: GeoInfoRegisteredCountry, GeoInfoRepresentedCountry, GeoInfoNoCountry
**Links:** REQ-GEO-CALC-001

### REQ-GEO-CALC-004
**Requirement:** ipcalc MUST report as the geo network the network,
in CIDR notation, of the database record from which it reported values
or, when it reported values from several of the Country, City and ASN
records, the smallest of their networks, which is the network for which
all reported values hold; for an IPv4 address in an IPv6 database the
network MUST be printed as IPv4.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-maxmind.c:geo_ip_lookup (`record_prefix`), ipcalc-maxmind.c:record_network
**Acceptance:** with the test databases: `-g 192.0.2.5` (Country /22, City /24) → `GEONETWORK=192.0.2.0/24`; `-g 198.51.100.5` (Country /25, City /24, ASN /27) → `GEONETWORK=198.51.100.0/27`; `-g 198.51.100.200` (Country /26, City /24, no ASN) → `GEONETWORK=198.51.100.192/26`; `-g 192.0.2.77/23` (City omitted) → `GEONETWORK=192.0.0.0/22`; `-g 2001:db8::1` → `GEONETWORK=2001:db8::/48` — positive ; `--all-info 192.0.0.0/16` (no record covers it) → no geo network — negative ; test: GeoInfoIPv4, GeoInfoRegisteredCountry, GeoInfoNoCountry, GeoInfoNetworkLargerThanCity, GeoInfoIPv6, GeoInfoNetworkLargerThanRecords
**Links:** REQ-GEO-CALC-001, REQ-GEO-CALC-002

### REQ-GEO-CALC-005
**Requirement:** When ipcalc reports the values of the ASN database
record (REQ-GEO-CALC-002), it MUST also report as the AS network the
network of that record, in CIDR notation, independently of the Country
and City records; for an IPv4 address in an IPv6 database the network
MUST be printed as IPv4. This applies to IPv4 and IPv6. The ASN
database is built from routing data, so its record's network
approximates the prefix the AS announces, which is the network the
address belongs to (e.g. the ISP's allocation), whereas the geo network
(REQ-GEO-CALC-004) is usually the much smaller network of the City
record.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-maxmind.c:geo_ip_lookup (`record_prefix` of the ASN lookup), ipcalc-maxmind.c:record_network; ipcalc.1.md#Options (`--geoinfo`)
**Acceptance:** with the test databases: `-g 2001:db8::1` (ASN /32, City /48) → `ASNETWORK=2001:db8::/32` and `GEONETWORK=2001:db8::/48`; `-g 192.0.2.5` → `ASNETWORK=192.0.2.0/24`; `-g 198.51.100.5` (ASN /27) → `ASNETWORK=198.51.100.0/27`; `-g 203.0.113.1` (only in the ASN database) → `ASNETWORK=203.0.113.0/24` — positive ; `-g 198.51.100.200` (no ASN record) and `-g 192.0.2.77/23` (ASN record omitted) → no `ASNETWORK` — negative ; test: GeoInfoIPv6, GeoInfoIPv4, GeoInfoRegisteredCountry, GeoInfoASNOnly, GeoInfoNoCountry, GeoInfoNetworkLargerThanCity
**Links:** REQ-GEO-CALC-001, REQ-GEO-CALC-002, REQ-GEO-CALC-004

### REQ-GEO-ERR-001
**Requirement:** A missing database or an address not found in it MUST
NOT be an error: ipcalc MUST print the remaining output without the
geo fields and exit with status 0. When the backend library cannot be
loaded at run time, `-g` MUST print `ipcalc: could not open <library>;
geographic information requires libmaxminddb and the GeoLite2
databases, install them if they are missing` (or `could not find
symbols in …`) to stderr unless `-s` is given, so that the user
learns what to install, and
`--all-info` MUST omit the geo fields silently.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-maxmind.c:geo_ip_lookup (a database that `lookup()` cannot open or that has no record leaves its values NULL), ipcalc-maxmind.c:geo_setup (storing and printing `err` on the second call); ipcalc.c:main
**Acceptance:** runtime-linked build without the library (`ipcalc-nolib`, a test build that loads the library from a nonexistent directory): `-g 192.0.2.5` → tests/geo-nolib-192.0.2.5 (the message on stderr), exit 0; `--all-info 192.0.2.5` → tests/geo-nolib-all-info-192.0.2.5 (no geo lines, no message), exit 0; `-s -g 192.0.2.5` → no output; with the test databases: `--all-info 10.0.0.1` (not in them) → no geo lines, exit 0 — positive ; test: GeoInfoNotFound, GeoLibraryMissing, GeoLibraryMissingAllInfo, GeoLibraryMissingSilent (the last three in runtime-linked builds only)

### REQ-GEO-OUTPUT-001
**Requirement:** Geo values MUST be printed as `CONTINENT`,
`COUNTRYCODE`, `COUNTRY`, `CITY`, `POSTALCODE`, `COORDINATES`,
`ACCURACYRADIUS`, `TIMEZONE`, `ASN`, `ASORG`, `ASNETWORK`, `GEONETWORK` in
`NAME=value` and JSON output, and as `Continent`, `Country code`,
`Country`, `City`, `Postal code`, `Coordinates`, `Accuracy`,
`Time zone`, `Routed by AS`, `Operated by`, `AS network`, `Geo network`
after a blank line in the summary output, in that order. The accuracy
radius MUST be a number of kilometers, followed by ` km` in the summary
only.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:show_info_fields (`CONTINENT_NAME` … `GEONETWORK_NAME` rows, `SECTION_GEO`)
**Acceptance:** see ipcalc.1.md examples "Display all information of an IPv4" and "Display JSON output"; with the test databases: `--all-info 192.0.2.5` → tests/geo-all-info-192.0.2.5, `--format=json -g 2001:db8::1` → tests/geo-json-2001:db8::1 — positive ; test: GeoInfoAllInfo, GeoInfoJson
**Links:** REQ-CLI-OUTPUT-005, REQ-CLI-OUTPUT-012

### REQ-GEO-TEST-001
**Requirement:** Tests of geo lookups MUST run a test-only build of
ipcalc, `ipcalc-geo`, that reads MaxMind DB files generated at build
time by `tests/mkmmdb.py` from `tests/geo-country.json`,
`tests/geo-city.json` and `tests/geo-asn.json`, and MUST NOT read the databases installed on the
system, whose contents vary between hosts and over time.
**Strength:** MUST
**Status:** DERIVED
**Source:** tests/meson.build (`ipcalc_geo`, `geo_dbs`); tests/mkmmdb.py; ipcalc-maxmind.c (`MAXMINDDB_LOCATION_COUNTRY`, `MAXMINDDB_LOCATION_CITY`, `MAXMINDDB_LOCATION_ASN` overridable at build time)
**Acceptance:** in a build with a geo backend, the Geo* `--test-outfile` tests pass on a host without `/usr/share/GeoIP` — positive ; test: GeoInfoIPv4 and the other `ipcalc-geo` tests
**Links:** REQ-GEN-TEST-001, REQ-GEN-TEST-007

## Open items

REQ-GEO-ERR-001 is tested only for an address that is not in the
databases.
