---
title: Geographic information (-g/--geoinfo)
id-prefix: REQ-GEO
sources:
  - ipcalc-maxmind.c
  - ipcalc-geoip.c
  - ipcalc.h (geo_ip_lookup, geo_setup stubs)
  - ipcalc.c (main, get_ipv4_info, get_ipv6_info, long_options, usage)
  - meson.build, meson_options.txt, Makefile
  - ipcalc.1.md
---

# Geographic information (`-g/--geoinfo`)

Generated with `contrib/ai/protocols/requirements-from-implementation.md`.
Only the build without a geo backend was run while deriving this
document (no libmaxminddb or libGeoIP development files were
available); the backend requirements below are derived from code
reading and are not covered by any test in `tests/meson.build`.

### REQ-GEO-BUILD-001
**Requirement:** ipcalc MUST be buildable with exactly one geo backend —
libmaxminddb (preferred when both are found) or libGeoIP — or with none,
and with either backend either linked at build time or loaded at run
time with `dlopen()`.
**Strength:** MUST
**Status:** DERIVED
**Source:** meson.build (`use_maxminddb`, else `use_geoip`, `use_runtime_linking`); meson_options.txt; Makefile (`USE_MAXMIND`, `USE_GEOIP`, `USE_RUNTIME_LINKING`); ipcalc.h (stubs when neither `USE_GEOIP` nor `USE_MAXMIND`)
**Acceptance:** each configuration in `.gitlab-ci.yml` builds with `-Werror` — positive ; test: CI jobs (not a meson test)
**Links:** REQ-GEN-BUILD-001

### REQ-GEO-INPUT-001
**Requirement:** `-g`/`--geoinfo` MUST be accepted only when ipcalc is
built with a geo backend. In a build without one, both forms MUST be
rejected as unrecognized options (REQ-CLI-ERR-002, exit status 1), and
neither `--help` nor `--usage` MUST list them, so that a script relying
on geo output fails instead of silently getting none.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:long_options, ipcalc.c:usage, ipcalc.c:main (`GEO_SHORT_OPTION`, `case 'g'`) — all under `#if defined(USE_GEOIP) || defined(USE_MAXMIND)`; meson.build (`have_geo`); ipcalc.1.md#Options (`--geoinfo`)
**Acceptance:** build with a backend: `ipcalc -g 127.0.0.1` → exit 0 — positive ; build without a backend: `ipcalc -g 127.0.0.1`, `ipcalc --geoinfo 127.0.0.1` → exit 1; `ipcalc --usage` → does not mention `-g` or `--geoinfo` — negative ; test: GeoInfoAccepted (with a backend), GeoInfoShortUnavailable, GeoInfoLongUnavailable, GeoInfoNotInUsage (without a backend)
**Links:** REQ-CLI-ERR-002, REQ-CLI-COMPAT-002, REQ-GEO-ERR-001

### REQ-GEO-CALC-001
**Requirement:** With `-g`, or with `--all-info` when the backend is
usable, ipcalc MUST look up the input address (not the network address)
in the system geo databases and report the country name, ISO country
code, city, and `<latitude>,<longitude>` coordinates (printed with
`%f`) that the databases provide, omitting any value not found.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:get_ipv4_info, ipcalc.c:get_ipv6_info (`geo_ip_lookup(ipStr, …)`), ipcalc.c:main (`geo_setup() == 0 && FLAG_SHOW_ALL_INFO`); ipcalc-maxmind.c:geo_ip_lookup (GeoLite2-Country.mmdb, GeoLite2-City.mmdb under /usr/share/GeoIP by default); ipcalc-geoip.c:geo_ipv4_lookup, geo_ipv6_lookup; ipcalc.1.md#Options (`--geoinfo`, `--all-info`)
**Acceptance:** with a backend and databases: `ipcalc -g 193.92.150.2` → `COUNTRYCODE=GR`, `COUNTRY=Greece` (ipcalc.1.md example) — positive ; test: none
**Links:** REQ-CLI-OUTPUT-004, REQ-INFO-OUTPUT-001, REQ-INFO-OUTPUT-003

> [REVIEW] The maxmind backend never sets the city (comment: not in
> the free database), so `CITY` is GeoIP-only. The GeoIP backend checks
> `gir->longitude != 0 && gir->longitude != 0` (latitude is never
> checked) and returns without `GeoIP_delete()` when the country lookup
> fails.

### REQ-GEO-ERR-001
**Requirement:** A missing database or an address not found in it MUST
NOT be an error: ipcalc MUST print the remaining output without the
geo fields and exit with status 0. When the backend library cannot be
loaded at run time, `-g` MUST print `ipcalc: could not open <library>`
(or `could not find symbols in …`) to stderr unless `-s` is given, and
`--all-info` MUST omit the geo fields silently.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc-maxmind.c, ipcalc-geoip.c (`/* Else fail silently */`, `geo_setup()` storing and printing `err` on the second call); ipcalc.c:main
**Acceptance:** runtime-linked build without the library: `ipcalc -g 8.8.8.8` → error message, exit 0; `ipcalc --all-info 8.8.8.8` → no geo lines, exit 0 — positive ; test: none

### REQ-GEO-OUTPUT-001
**Requirement:** Geo values MUST be printed as `COUNTRYCODE`, `COUNTRY`,
`CITY`, `COORDINATES` in `NAME=value` and JSON output, and as
`Country code`, `Country`, `City`, `Coordinates` after a blank line in
the summary output, in that order.
**Strength:** MUST
**Status:** DERIVED
**Source:** ipcalc.c:main (`COUNTRYCODE_NAME` … `COORDINATES_NAME`)
**Acceptance:** see ipcalc.1.md examples "Display all information of an IPv4" and "Display JSON output" — positive ; test: none
**Links:** REQ-CLI-OUTPUT-005

## Open items

`REVIEW`: REQ-GEO-CALC-001 (backend differences and GeoIP code issues).
Only REQ-GEO-INPUT-001 has tests; geo builds were not exercised while
deriving this document.
