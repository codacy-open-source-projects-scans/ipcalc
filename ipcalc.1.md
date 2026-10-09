# ipcalc(1) - Perform simple operations on IP addresses and networks

## SYNOPSIS
**ipcalc** [OPTION]... <IP address>[/prefix] [netmask]


## Description

**ipcalc** provides a simple way to calculate IP information for a host
or network. Depending on the options specified, it may be used to provide
IP network information in human readable format, in a format suitable for
parsing in scripts, generate random private addresses, resolve an IP address,
or check the validity of an address.

By default or when the **--info** or **--all-info** parameters
are specified the information provided is free form and human readable.
Otherwise the output is JSON formatted when **-j** is specified,
or when specific options are given (e.g., **--prefix**) the output is
in the **VAR=VALUE** format.

The various options specify what information **ipcalc** should display
on standard output. Multiple options may be specified.  It is required
to specify an IP address; several operations require
a netmask or a CIDR prefix as well.


## Options

* **-c**, **--check**
  Checks whether this is a valid IP address. When an IP family is specified
  only the addresses of that family are accepted. A zero exit status
  indicates a well formed address and a non-zero exit status indicates a
  malformed address.

* **--equals**=NET, **--subnet-of**=NET, **--overlaps**=NET
  Compare the provided network with NET and print the result, such as
  "The networks overlap" or "192.168.0.0/24 is a subnet of 192.168.0.0/23";
  nothing is printed with **--no-decorate**, **--json** or **--silent**.
  **--equals** holds if both have the
  same network address and prefix, **--subnet-of** if every address of
  the network is in NET (equal networks included), and **--overlaps** if
  the two share at least one address; for CIDR networks the latter holds
  exactly when one is a subnet of the other. Host bits are ignored, and
  NET is given in the same form as the provided address, with an optional
  prefix or netmask after a slash. Networks of different families never
  compare true. The exit status is 0 when the comparison holds, 1 when it
  does not, and 2 on any error, such as a malformed address. Only one
  comparison may be given, and not together with **--check**.
  Example "ipcalc 192.168.0.1/24 --subnet-of=192.168.0.0/23".

* **-i**, **--info**
  Display generic information on the provided network in human readable format.
  This is the default option if no other options are provided.

* **--all-info**
  Display verbose information on the provided network and addresses in human
  readable format. That includes GeoIP information.

  For an IPv6 unicast address it also prints the interface identifier, i.e.,
  the low 64 bits of the address, unless they are all zero or the address
  lies in ::/3. When the interface identifier is a Modified EUI-64 (it
  contains ff:fe in its middle octets), it prints the EUI-64, the MAC
  address it was derived from, whether that MAC address is universally
  (UAA) or locally (LAA) administered (MAC scope: universal or local), and
  whether it is an individual or group address (MAC type). These fields are
  not derived from other interface identifiers, such as stable privacy or
  temporary addresses, whose bits carry no such meaning (RFC 7136).

* **-S**, **--split**=[COUNT:]PREFIX
  Split the provided network using the specified prefix or netmask. That is,
  split up the network into smaller chunks of a specified prefix. When
  combined with no-decorate mode (**--no-decorate**), the split networks
  will be printed in raw form. Example "ipcalc -S 26 192.168.1.0/24".

  The option can be repeated to split the network into subnets of different
  sizes, also known as a VLSM (variable-length subnet mask) split. Each
  request of the form COUNT:PREFIX asks for COUNT subnets of the given
  prefix or netmask, and at most one request without a count takes all the
  space that remains. Requests are allocated from the start of the network,
  the largest subnets first, while the subnets are printed in the order
  of the requests. Space that is not allocated to any request is printed
  as unused networks, except in no-decorate mode. If the network is too
  small for the requests, nothing is printed and the exit status is 1.
  Example "ipcalc -S 1:58 -S 64 2001:db8:1c88:6000::/56".

* **--split-hosts**=N1,N2,...
  Split the provided network into subnets that hold N1, N2, ... hosts,
  one subnet for each count. Each subnet is the smallest that holds the
  hosts; for IPv4 that includes the network and broadcast addresses
  (e.g., 10 hosts need a /28). The subnets are allocated as with **--split**
  and printed with their details, followed by the needed size, the used
  network and the unused networks. The values are those of the split of
  the Debian ipcalc (its **-s** option), except that IPv6 networks are
  supported, a host count of 0 is rejected, and if the network is too
  small for the requests, nothing is printed and the exit status is 1.
  It cannot be combined with **--split**.
  Example "ipcalc --split-hosts=10,20,30 192.168.0.0/24".

* **-d**, **--deaggregate**
  Deaggregates the provided address range. That is, print the networks that
  cover the range. The range is given using the '-' separator, e.g.,
  "192.168.1.3-192.168.1.23". When combined with no-decorate mode
  (**--no-decorate**), the networks are printed in raw form.

* **-r**, **--random-private**
  Generate a random private address using the supplied prefix or mask. By default
  it displays output in human readable format, but may be combined with
  other options (e.g., **--network**) to display specific information in
  **VAR=VALUE** format.

* **-h**, **--hostname**
  Display the hostname for the given IP address.
  The variable exposed is HOSTNAME. When combined with no-decorate mode
  (**--no-decorate**), only the hostname is printed.

* **-o**, **--lookup-host**
  Display the IP address for the given hostname.
  The variable exposed is ADDRESS. When combined with no-decorate mode
  (**--no-decorate**), only the address is printed.

* **-4**, **--ipv4**
  Explicitly specify the IPv4 address family.

* **-6**, **--ipv6**
  Explicitly specify the IPv6 address family.

* **-b**, **--broadcast**
  Display the broadcast address for the given IP address and netmask.
  The variable exposed is BROADCAST (if available).

* **-a**, **--address**
  Display the IP address for the given input.
  The variable exposed is ADDRESS (if available).

* **-g**, **--geoinfo**
  Display geographic information for the given IP address. This option
  requires libGeoIP/libmaxminddb to be available, and is not accepted
  when ipcalc is built without either. The variables exposed are
  COUNTRYCODE, COUNTRY, CITY and COORDINATES (when available).

* **-m**, **--netmask**
  Calculate the netmask for the given IP address. If no mask or prefix
  is provided, in IPv6 a 128-bit mask is assumed, while in IPv4 it assumes
  that the IP address is in a complete class A, B, or C network. Note,
  however, that many networks no longer use the default netmasks in IPv4.
  The variable exposed is NETMASK.

* **-p**, **--prefix**
  Show the prefix for the given mask/IP address.
  The variable exposed is PREFIX.

* **--class-prefix**
  Assign the netmask of the provided IPv4 address based on the address
  class. This was the default in previous versions of this software.

* **-n**, **--network**
  Display the network address for the given IP address and netmask.
  The variable exposed is NETWORK.

* **--cidr**
  Display the network address and the prefix in CIDR notation, e.g.,
  "192.168.123.0/24". The prefix is included for a single address too.
  The variable exposed is CIDR. When combined with no-decorate mode
  (**--no-decorate**), only the network and prefix are printed, e.g.,
  "ipcalc -r 24 --cidr --no-decorate" prints a random private network.

* **--reverse-dns**
  Display the reverse DNS for the given IP address and netmask.
  Reverse DNS domains for IPv6 (RFC 3596) can only be delegated at 4-bit
  boundaries, so for an IPv6 prefix that is not a multiple of 4 the network
  is covered by up to 8 domains, which are all printed. In that case the
  REVERSEDNS variable holds a quoted, space-separated list, with
  **--no-decorate** each domain is printed on its own line. In the JSON
  output of **--all-info**, REVERSEDNS is always an array.
  The variable exposed is REVERSEDNS.

* **--minaddr**
  Display the minimum host address in the provided network.
  The variable exposed is MINADDR.

* **--maxaddr**
  Display the maximum host address in the provided network.
  The variable exposed is MAXADDR.

* **--addresses**
  Display the number of host addresses in the provided network.
  The variable exposed is ADDRESSES.

* **--addrspace**
  Display address space allocation information for the provided network.
  The variable exposed is ADDRSPACE.

* **--no-decorate**
  Print only the requested information. That when combined with
  split networks option, will only print the networks without any
  additions for readability. Values that contain a space, such as
  the address space, are still enclosed in double quotes.

* **-j**, **--json**
  Print the output as a JSON object instead of the usual output format.
  In info mode the object always contains the summary information of
  **--info** (or of **--all-info** when given), regardless of which
  specific info options are given. It applies to **--split** and
  **--deaggregate** as well, while **--check** and the comparison options
  print nothing.

* **-s**, **--silent**
  Don't ever display error messages, nor the result of a comparison.


## Examples

### Display all information of an IPv4

    $ ipcalc --all-info 193.92.150.2/24
    Address:        193.92.150.2
    Network:        193.92.150.0/24
    Netmask:        255.255.255.0 = 24
    Broadcast:      193.92.150.255
    Reverse DNS:    150.92.193.in-addr.arpa.

    Address space:  Internet
    Address class:  Class C
    HostMin:        193.92.150.1
    HostMax:        193.92.150.254
    Hosts/Net:      254
    
    Country code:   GR
    Country:        Greece

### Display information in key-value format

    $ ipcalc -pnmb --minaddr --maxaddr --geoinfo --addrspace 193.92.150.2/255.255.255.224
    NETMASK=255.255.255.224
    PREFIX=27
    BROADCAST=193.92.150.31
    NETWORK=193.92.150.0
    MINADDR=193.92.150.1
    MAXADDR=193.92.150.30
    ADDRSPACE="Internet"
    COUNTRY="Greece"

### Display all information of an IPv6

    $ ipcalc --all-info 2a03:2880:20:4f06:face:b00c:0:14/64
    Full Address:   2a03:2880:0020:4f06:face:b00c:0000:0014
    Address:        2a03:2880:20:4f06:face:b00c:0:14
    Full Network:   2a03:2880:0020:4f06:0000:0000:0000:0000/64
    Network:        2a03:2880:20:4f06::/64
    Netmask:        ffff:ffff:ffff:ffff:: = 64
    Reverse DNS:    6.0.f.4.0.2.0.0.0.8.8.2.3.0.a.2.ip6.arpa.
    
    Address space:  Global Unicast
    Interface ID:   face:b00c:0000:0014
    HostMin:        2a03:2880:20:4f06::
    HostMax:        2a03:2880:20:4f06:ffff:ffff:ffff:ffff
    Hosts/Net:      2^(64) = 18446744073709551616
    
    Country code:   IE
    Country:        Ireland

### Display the MAC address behind an IPv6 link-local address

    $ ipcalc --all-info fe80::21b:21ff:fe3a:5c7d/64
    Full Address:   fe80:0000:0000:0000:021b:21ff:fe3a:5c7d
    Address:        fe80::21b:21ff:fe3a:5c7d
    Full Network:   fe80:0000:0000:0000:0000:0000:0000:0000/64
    Network:        fe80::/64
    Netmask:        ffff:ffff:ffff:ffff:: = 64
    Reverse DNS:    0.0.0.0.0.0.0.0.0.0.0.0.0.8.e.f.ip6.arpa.

    Address space:  Link-Scoped Unicast
    Interface ID:   021b:21ff:fe3a:5c7d
    EUI-64:         00:1b:21:ff:fe:3a:5c:7d
    MAC address:    00:1b:21:3a:5c:7d
    MAC scope:      universal
    MAC type:       individual
    HostMin:        fe80::
    HostMax:        fe80::ffff:ffff:ffff:ffff
    Hosts/Net:      2^(64) = 18446744073709551616

### Display JSON output

    $ ipcalc --all-info -j 2a03:2880:20:4f06:face:b00c:0:14/64
    {
      "FULLADDRESS":"2a03:2880:0020:4f06:face:b00c:0000:0014",
      "ADDRESS":"2a03:2880:20:4f06:face:b00c:0:14",
      "FULLNETWORK":"2a03:2880:0020:4f06:0000:0000:0000:0000",
      "NETWORK":"2a03:2880:20:4f06::",
      "NETMASK":"ffff:ffff:ffff:ffff::",
      "PREFIX":"64",
      "CIDR":"2a03:2880:20:4f06::/64",
      "REVERSEDNS":"6.0.f.4.0.2.0.0.0.8.8.2.3.0.a.2.ip6.arpa.",
      "ADDRSPACE":"Global Unicast",
      "INTERFACEID":"face:b00c:0000:0014",
      "MINADDR":"2a03:2880:20:4f06::",
      "MAXADDR":"2a03:2880:20:4f06:ffff:ffff:ffff:ffff",
      "ADDRESSES":"18446744073709551616",
      "COUNTRYCODE":"IE",
      "COUNTRY":"Ireland",
      "COORDINATES":"53.000000,-8.000000"
    }

### Split a network into subnets of different sizes

    $ ipcalc -S 1:28 -S 2:27 192.168.0.0/24
    [Split networks]
    Network:        192.168.0.64/28
    Network:        192.168.0.0/27
    Network:        192.168.0.32/27

    Total:          3

    [Unused networks]
    Network:        192.168.0.80/28
    Network:        192.168.0.96/27
    Network:        192.168.0.128/25

### Split a network into subnets for a number of hosts

    $ ipcalc --split-hosts=10,20 192.168.0.0/24
    [Split networks]
    1. Requested size: 10 hosts
    Network:        192.168.0.32/28
    Netmask:        255.255.255.240 = 28
    Broadcast:      192.168.0.47
    HostMin:        192.168.0.33
    HostMax:        192.168.0.46
    Hosts/Net:      14

    2. Requested size: 20 hosts
    Network:        192.168.0.0/27
    Netmask:        255.255.255.224 = 27
    Broadcast:      192.168.0.31
    HostMin:        192.168.0.1
    HostMax:        192.168.0.30
    Hosts/Net:      30

    Total:          2
    Needed size:    48
    Used network:   192.168.0.0/26

    [Unused networks]
    Network:        192.168.0.48/28
    Network:        192.168.0.64/26
    Network:        192.168.0.128/25

### Split an IPv6 prefix into one /58 and /64 networks in the remaining space

    $ ipcalc -S 1:58 -S 64 2001:db8:1c88:6000::/56 --no-decorate
    2001:db8:1c88:6000::/58
    2001:db8:1c88:6040::/64
    2001:db8:1c88:6041::/64
    ...
    2001:db8:1c88:60ff::/64

### Check whether a network is a subnet of another

    $ ipcalc 192.168.0.1/24 --subnet-of=192.168.1.15/23
    192.168.0.0/24 is a subnet of 192.168.0.0/23

### Check whether a network overlaps any network in a list

    $ while read net; do
    >   ipcalc -s 10.20.0.0/16 --overlaps="$net"
    >   case $? in 0) echo "conflict: $net" ;; 2) echo "bad entry: $net" ;; esac
    > done < networks.txt

### Lookup of a hostname

    $ ipcalc --lookup-host localhost --no-decorate
    ::1

### IPv4 lookup of a hostname

    $ ipcalc --lookup-host localhost --no-decorate -4
    127.0.0.1

### Reverse lookup of a hostname

    $ ipcalc -h 127.0.0.1 --no-decorate
    localhost

## Authors
* Nikos Mavrogiannopoulos <n.mavrogiannopoulos@gmail.com>
* Erik Troan <ewt@redhat.com>
* Preston Brown <pbrown@redhat.com>
* David Cantrell <dcantrell@redhat.com>

## Reporting Bugs

Report bugs at https://gitlab.com/ipcalc/ipcalc/issues

## Copyright

Copyright © 1997-2024 Red Hat, Inc.
Copyright © 2020-2024 Nikos Mavrogiannopoulos
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.
