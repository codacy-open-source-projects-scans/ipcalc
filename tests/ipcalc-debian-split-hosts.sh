#!/bin/sh

# Copyright (C) 2026 Nikos Mavrogiannopoulos
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

# Usage: ipcalc-debian-split-hosts.sh NETWORK COUNT[,COUNT...]
#
# Checks that "ipcalc --split-hosts=COUNTS NETWORK" prints the same values
# as "ipcalc -s COUNT..." of the Debian ipcalc (REQ-SPLIT-COMPAT-001).
# Both outputs are reduced to "<request> <field> <value>" lines, since
# the two tools lay out the fields differently.

IPCALC="${IPCALC:-build/ipcalc}"
DEBIAN_IPCALC="${DEBIAN_IPCALC:-tests/debian-ipcalc/ipcalc}"
NETWORK="$1"
COUNTS="$2"

command -v perl >/dev/null || { echo "perl: not found"; exit 1; }
perl -Mbignum -e 1 2>/dev/null || { echo "perl: bignum module not found"; exit 1; }

# Fields of the requested subnets; the Debian tool prints a block for the
# base network before them, and ipcalc a Hosts/Net after "Total:".
normalize() {
	awk '
	/^[0-9]+\. Requested size:/ { req++; print req, "Requested", $4; next }
	/^(Total|Needed size):/ && req { req = 0 }
	req && /^(Network|Broadcast|HostMin|HostMax|Hosts\/Net):/ { print req, $1, $2; next }
	req && /^Netmask:/ { print req, $1, $2, $4; next }
	/^Needed size:/ { print "Needed", $3 }
	/^Used network:/ { print "Used", $3 }
	/^(Unused:|\[Unused networks\])/ { unused = 1; next }
	unused && /^Network:/ { print "Unused", ++n, $2; next }
	unused && /^[0-9]/ { print "Unused", ++n, $1 }
	' | sort
}

OURS=$(${IPCALC} --split-hosts="${COUNTS}" "${NETWORK}") || { echo "ipcalc failed"; exit 1; }
OURS=$(echo "${OURS}" | normalize)
THEIRS=$(perl "${DEBIAN_IPCALC}" -n -b "${NETWORK}" -s $(echo "${COUNTS}" | tr ',' ' ') | normalize)

if test -z "${OURS}" || test "${OURS}" != "${THEIRS}"; then
	echo "Values differ from the Debian ipcalc for ${NETWORK} with ${COUNTS}"
	echo "ipcalc:"
	echo "${OURS}"
	echo "Debian ipcalc:"
	echo "${THEIRS}"
	exit 1
fi

exit 0
