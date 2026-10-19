#!/usr/bin/env python3
#
# Writes a MaxMind DB file (https://maxmind.github.io/MaxMind-DB/) from a
# JSON description, so that the geo tests do not depend on the databases
# installed on the system:
#
#   {"database_type": "...", "networks": {"192.0.2.0/24": {...}, ...}}
#
# Integers are written as the smallest unsigned type that holds them, as
# MaxMind does (e.g. accuracy_radius as uint16).
#
# Usage: mkmmdb.py INPUT.json OUTPUT.mmdb

import ipaddress
import json
import struct
import sys

TYPE_UTF8 = 2
TYPE_DOUBLE = 3
TYPE_UINT16 = 5
TYPE_UINT32 = 6
TYPE_MAP = 7
TYPE_UINT64 = 9
TYPE_ARRAY = 11
TYPE_BOOLEAN = 14

RECORD_SIZE = 24


class Uint:
    """An unsigned integer of a fixed type, as the metadata requires"""
    def __init__(self, type_, value):
        self.type = type_
        self.value = value


def control(type_, size):
    if size < 29:
        out, ext = size, b''
    elif size < 29 + 256:
        out, ext = 29, bytes([size - 29])
    elif size < 285 + 65536:
        out, ext = 30, struct.pack('>H', size - 285)
    else:
        out, ext = 31, (size - 65821).to_bytes(3, 'big')
    if type_ <= 7:
        return bytes([(type_ << 5) | out]) + ext
    return bytes([out, type_ - 7]) + ext


def encode_uint(type_, value):
    data = value.to_bytes((value.bit_length() + 7) // 8, 'big')
    return control(type_, len(data)) + data


def encode(value):
    if isinstance(value, Uint):
        return encode_uint(value.type, value.value)
    if isinstance(value, bool):
        return control(TYPE_BOOLEAN, int(value))
    if isinstance(value, int):
        if value < 1 << 16:
            return encode_uint(TYPE_UINT16, value)
        if value < 1 << 32:
            return encode_uint(TYPE_UINT32, value)
        return encode_uint(TYPE_UINT64, value)
    if isinstance(value, float):
        return control(TYPE_DOUBLE, 8) + struct.pack('>d', value)
    if isinstance(value, str):
        data = value.encode('utf-8')
        return control(TYPE_UTF8, len(data)) + data
    if isinstance(value, dict):
        return control(TYPE_MAP, len(value)) + b''.join(
            encode(k) + encode(v) for k, v in value.items())
    if isinstance(value, list):
        return control(TYPE_ARRAY, len(value)) + b''.join(
            encode(v) for v in value)
    raise TypeError('cannot encode %r' % (value,))


class Data:
    def __init__(self, offset):
        self.offset = offset


def insert(root, network, data):
    """Adds a leaf for network; IPv4 networks go under ::/96, where
    libmaxminddb looks them up in an IPv6 tree"""
    net = ipaddress.ip_network(network)
    bits = int(net.network_address)
    plen = net.prefixlen
    if net.version == 4:
        plen += 96
    node = root
    for depth in range(plen):
        bit = (bits >> (127 - depth)) & 1
        if depth == plen - 1:
            if node[bit] is not None:
                sys.exit('%s overlaps another network' % network)
            node[bit] = data
        else:
            if node[bit] is None:
                node[bit] = [None, None]
            elif isinstance(node[bit], Data):
                sys.exit('%s overlaps another network' % network)
            node = node[bit]


def main():
    with open(sys.argv[1], encoding='utf-8') as f:
        desc = json.load(f)

    root = [None, None]
    data_section = b''
    for network, record in desc['networks'].items():
        insert(root, network, Data(len(data_section)))
        data_section += encode(record)

    nodes = [root]
    i = 0
    while i < len(nodes):
        for child in nodes[i]:
            if isinstance(child, list):
                nodes.append(child)
        i += 1
    index = {id(n): i for i, n in enumerate(nodes)}
    node_count = len(nodes)

    def record(child):
        if child is None:
            return node_count
        if isinstance(child, Data):
            return node_count + 16 + child.offset
        return index[id(child)]

    tree = b''.join(record(n[0]).to_bytes(3, 'big') +
                    record(n[1]).to_bytes(3, 'big') for n in nodes)

    metadata = {
        'node_count': Uint(TYPE_UINT32, node_count),
        'record_size': Uint(TYPE_UINT16, RECORD_SIZE),
        'ip_version': Uint(TYPE_UINT16, 6),
        'database_type': desc['database_type'],
        'languages': ['en'],
        'binary_format_major_version': Uint(TYPE_UINT16, 2),
        'binary_format_minor_version': Uint(TYPE_UINT16, 0),
        'build_epoch': Uint(TYPE_UINT64, 1),
        'description': {'en': 'ipcalc test database'},
    }

    with open(sys.argv[2], 'wb') as f:
        f.write(tree + bytes(16) + data_section +
                b'\xab\xcd\xefMaxMind.com' + encode(metadata))


if __name__ == '__main__':
    main()
