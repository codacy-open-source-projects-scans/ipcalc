# Debian ipcalc 0.51-1

`ipcalc` is the unmodified ipcalc script by Krischan Jodies, as shipped
in Debian's ipcalc 0.51-1 package (the Debian package applies no
patches to it), from
https://sources.debian.org/src/ipcalc/0.51-1/ipcalc
(sha256 52cdeb286b7f62ac88a6613b792b526b5bf5a714c66c3225cefd412d19cb59e4).
It is licensed under the GNU GPL, version 2 or later; see its header.

`tests/ipcalc-debian-split-hosts.sh` runs it to check that
`--split-hosts` prints the same values as its `-s` split
(REQ-SPLIT-COMPAT-001). It requires `perl` with the `bignum` module.
The directory is excluded from release tarballs (`.gitattributes`), in
which case the tests are not registered.
