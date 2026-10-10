testrunner = find_program('ipcalc-testrunner.sh')

# Incompatible combinations test
test('Split-Info',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -abmnp -S 26 10.100.1.0/24'
	]
)
test('SplitHosts-Info',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -n --split-hosts=10 10.100.1.0/24'
	]
)
test('Check-Info',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c -abmnp 10.100.1.1'
	]
)
test('Check-Split',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c -S 26 10.100.1.0/24'
	]
)
test('Check-Host',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c -h 127.0.0.1'
	]
)
test('Random-Info',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -r 24 -i 127.0.0.1'
	]
)
test('Random',
	testrunner,
	args : [
		'--test-success',
		'out=$(' + ipcalc.full_path() + ' -r 24) && echo "$out" | grep -Eq \'^Network:[[:space:]]+(10\.|172\.(1[6-9]|2[0-9]|3[01])\.|192\.168\.)\' && ! echo "$out" | grep -q \'^Address:\''
	]
)
test('RandomAddress',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -r 24 -a' + '|grep ADDRESS'
	]
)
test('RandomIPv6Implicit',
	testrunner,
	args : [
		'--test-success',
		'out=$(' + ipcalc.full_path() + ' -r 112) && echo "$out" | grep -Eq \'^Network:[[:space:]]+fd\' && ! echo "$out" | grep -q \'^Address:\''
	]
)
test('RandomIPv6PrefixIsValidULA',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -6 -r 48 -a' + '|grep ^ADDRESS=fd'
	]
)
test('RandomIPv6Explicit',
	testrunner,
	args : [
		'--test-success',
		'out=$(' + ipcalc.full_path() + ' -6 -r 24) && echo "$out" | grep -Eq \'^Network:[[:space:]]+fd\' && ! echo "$out" | grep -q \'^Address:\''
	]
)
test('HostnameIPv6Localhost',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -6 -o localhost',
		files('hostname-localhost-ipv6')
	]
)
test('HostnameIPv4Localhost',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -4 -o localhost',
		files('hostname-localhost-ipv4')
	]
)
test('HostnameIPv4LocalhostJson',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -4 -o localhost',
		files('hostname-localhost-ipv4-json')
	]
)
# Values that reach the JSON output from outside ipcalc (here a host
# name) must be escaped. The test needs jq, and fails when it is missing
# rather than being skipped (REQ-GEN-TEST-007).
if host_machine.system() == 'linux'
	fake_getnameinfo = shared_module('fake-getnameinfo', 'fake-getnameinfo.c')
	test('JsonEscapeHostname',
		testrunner,
		args : [
			'--test-success',
			'{ command -v jq >/dev/null || { echo "jq: not found"; exit 1; }; } && ASAN_OPTIONS="$ASAN_OPTIONS:verify_asan_link_order=0" LD_PRELOAD=' + fake_getnameinfo.full_path() + ' ' + ipcalc.full_path() + ' -j -h 127.0.0.1 | jq -e \'.HOSTNAME == "a\\"b\\\\c\\td"\''
		],
		depends : fake_getnameinfo
	)

	# NAME=value output must stay usable with eval for a name from DNS
	foreach c : [
		['ShellEscapeHostname', '--format=shell -h 127.0.0.1'],
		['ShellEscapeHostnameDefault', '-h 127.0.0.1'],
	]
		test(c[0],
			testrunner,
			args : [
				'--test-success',
				'out=$(ASAN_OPTIONS="$ASAN_OPTIONS:verify_asan_link_order=0" LD_PRELOAD=' + fake_getnameinfo.full_path() + ' ' + ipcalc.full_path() + ' ' + c[1] + ') && eval "$out" && [ "$HOSTNAME" = "$(printf \'a"b\\\\c\\td\')" ]'
			],
			depends : fake_getnameinfo
		)
	endforeach

	# values-only output prints the name verbatim, as released
	foreach c : [
		['NoDecorateHostnameVerbatim', '--no-decorate -h 127.0.0.1'],
		['FormatValueHostnameVerbatim', '--format=value -h 127.0.0.1'],
	]
		cmd = 'ASAN_OPTIONS="$ASAN_OPTIONS:verify_asan_link_order=0" LD_PRELOAD=' + fake_getnameinfo.full_path() + ' ' + ipcalc.full_path() + ' ' + c[1]
		test(c[0],
			testrunner,
			args : ['--test-status', '0', cmd, '--test-output', cmd, 'a"b\\c\td'],
			depends : fake_getnameinfo
		)
	endforeach

	# deprecated options suggest --format on a terminal: [name, arguments, hint]
	fake_isatty = shared_module('fake-isatty', 'fake-isatty.c')
	foreach c : [
		['HintJson', '-j 10.0.0.1/24', 'ipcalc: hint: -j is deprecated; use --format=json'],
		['HintJsonFields', '-j -m 10.0.0.1/24', 'ipcalc: hint: -j is deprecated; use --format=json, which prints only the selected fields'],
		['HintJsonEquals', '-j 10.0.0.0/8 --equals=10.0.0.0/8', 'ipcalc: hint: -j is deprecated; use -s to print no result'],
		['HintNoDecorateFields', '--no-decorate -m 10.0.0.1/24', 'ipcalc: hint: --no-decorate is deprecated; use --format=value'],
		['HintNoDecorateSplit', '--no-decorate -S 26 10.0.0.0/24', 'ipcalc: hint: --no-decorate is deprecated; use --format=value'],
		['HintNoDecorateSummary', '--no-decorate 10.0.0.1/24', 'ipcalc: hint: --no-decorate is deprecated; use --format=human'],
		['NoHintFormat', '--format=json -m 10.0.0.1/24', ''],
		['NoHintFields', '-m -n 10.0.0.1/24', ''],
		['NoHintFormatValueSplit', '--format=value -S 26 10.0.0.0/24', ''],
		['NoHintSilent', '-s -j 10.0.0.1/24', ''],
	]
		cmd = 'ASAN_OPTIONS="$ASAN_OPTIONS:verify_asan_link_order=0" LD_PRELOAD=' + fake_isatty.full_path() + ' ' + ipcalc.full_path() + ' ' + c[1] + ' >/dev/null'
		test(c[0],
			testrunner,
			args : ['--test-status', '0', cmd, '--test-output', cmd, c[2]],
			depends : fake_isatty
		)
	endforeach

	# -h rejects a network before looking up its name
	fail_getnameinfo = shared_module('fail-getnameinfo', 'fail-getnameinfo.c')
	cmd = 'ASAN_OPTIONS="$ASAN_OPTIONS:verify_asan_link_order=0" LD_PRELOAD=' + fail_getnameinfo.full_path() + ' ' + ipcalc.full_path() + ' -h 127.0.0.1/8'
	test('HostnameNetworkNoLookup',
		testrunner,
		args : ['--test-status', '1', cmd, '--test-output', cmd,
			'ipcalc: -h requires a single address, not a network: 127.0.0.1/8'],
		depends : fail_getnameinfo
	)

	# --all-info does not look up the hostname
	test('AllInfoNoLookup',
		testrunner,
		args : ['--test-success',
			'out=$(ASAN_OPTIONS="$ASAN_OPTIONS:verify_asan_link_order=0" LD_PRELOAD=' + fake_getnameinfo.full_path() + ' ' + ipcalc.full_path() + ' -j --all-info 127.0.0.1) && ! echo "$out" | grep -q HOSTNAME'],
		depends : fake_getnameinfo
	)
endif
test('IPIPv6Localhost',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -h ::1',
		files('ip-localhost-ipv6')
	]
)
test('IPIPv4Localhost',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -h 127.0.0.1',
		files('ip-localhost-ipv4')
	]
)
test('IPIPv4LocalhostJson',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -h 127.0.0.1',
		files('ip-localhost-ipv4-json')
	]
)
# --class-prefix tests
test('AssignClassPrefix12',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 12.15.1.5 --class-prefix',
		files('12.15.1.5')
	]
)
test('AssignClassPrefix129',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 129.15.31.5 --class-prefix',
		files('129.15.31.5')
	]
)
test('AssignClassPrefix193',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 193.92.31.0 --class-prefix',
		files('193.92.31.0')
	]
)

# prefix tests
test('TestPrefix31',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 192.168.1.5/31',
		files('192.168.1.5-31')
	]
)
test('TestPrefix24',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 10.10.10.5/24',
		files('192.168.1.5-24')
	]
)
test('TestPrefix30',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 10.100.4.1/30',
		files('192.168.1.5-30')
	]
)
test('TestPrefix16',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 10.100.4.1/16',
		files('192.168.1.5-16')
	]
)
test('TestPrefix8',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp 10.10.10.10/8',
		files('192.168.1.5-8')
	]
)

# --split tests
test('SplitPrefix18',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 18 10.10.10.10/16',
		files('split-10.10.10.0-16-18')
	]
)
test('SplitPrefix24',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 24 10.10.10.0/16',
		files('split-10.10.10.0-16-24')
	]
)
test('SplitPrefix26',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 26 192.168.5.45/24',
		files('split-192.168.5.45-24-26')
	]
)
test('SplitPrefix29',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 29 192.168.5.0/24',
		files('split-192.168.5.0-24-29')
	]
)
test('SplitPrefix31',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 31 192.168.5.0/24',
		files('split-192.168.5.0-24-31')
	]
)
test('SplitPrefix32',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 32 192.168.5.0/24',
		files('split-192.168.5.0-24-32')
	]
)
test('SplitPrefix64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 64 2a03:2880:20:4f06:face::/56',
		files('split-2a03:2880:20:4f06:face::-56-64')
	]
)
test('SplitPrefix128',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 128 fcfa:b4ca:f1d8:125b:dc00::/127',
		files('split-fcfa:b4ca:f1d8:125b:dc00::-127-128')
	]
)
test('SplitPrefix120',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 120 fcfa:b4ca:f1d8:125b:dc00::/112',
		files('split-fcfa:b4ca:f1d8:125b:dc00::-112-120')
	]
)
test('SplitPrefix0IPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 0 0.0.0.0/0',
		files('split-0.0.0.0-0-0')
	]
)
test('SplitPrefix3Of1',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 3 10.0.0.0/1',
		files('split-10.0.0.0-1-3')
	]
)
test('SplitPrefix2Of0IPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 2 ::/0',
		files('split-::-0-2')
	]
)
test('SplitPrefix124EndOfSpace',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 124 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/120',
		files('split-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff-120-124')
	]
)

# --split tests with subnets of different sizes (VLSM)
test('SplitVLSMPrefix58And64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 1:58 -S 64 2001:db8:1c88:6000::/56',
		files('split-2001:db8:1c88:6000::-56-1:58-64')
	]
)
test('NoDecorateSplitVLSMPrefix58And64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 1:58 -S 64 2001:db8:1c88:6000::/56',
		files('nsplit-2001:db8:1c88:6000::-56-1:58-64')
	]
)
test('JsonSplitVLSMPrefix58And64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -S 1:58 -S 64 2001:db8:1c88:6000::/56',
		files('json-split-2001:db8:1c88:6000::-56-1:58-64')
	]
)
test('SplitVLSMUnused',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 1:28 -S 2:27 192.168.0.0/24',
		files('split-192.168.0.0-24-1:28-2:27')
	]
)
test('NoDecorateSplitVLSMUnused',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 1:28 -S 2:27 192.168.0.0/24',
		files('nsplit-192.168.0.0-24-1:28-2:27')
	]
)
test('JsonSplitVLSMUnused',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -S 1:28 -S 2:27 192.168.0.0/24',
		files('json-split-192.168.0.0-24-1:28-2:27')
	]
)
test('SplitVLSMFillAligned',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 3:64 -S 60 2001:db8::/56',
		files('split-2001:db8::-56-3:64-60')
	]
)
test('SplitVLSMSamePrefix',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 2:26 -S 26 192.168.1.0/24',
		files('split-192.168.1.0-24-2:26-26')
	]
)
test('SplitVLSMNetmask',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 2:255.255.255.192 192.168.1.0/24',
		files('split-192.168.1.0-24-2:255.255.255.192')
	]
)
test('SplitVLSMWholeSpace',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -S 2 -S 1:1 ::/0',
		files('split-::-0-2-1:1')
	]
)
test('SplitVLSMTooSmall',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S 1:25 -S 1:25 -S 1:26 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S 1:25 -S 1:25 -S 1:26 192.168.0.0/24',
		'ipcalc: 192.168.0.0/24 is too small for the requested subnets'
	]
)
test('SplitVLSMNoSpaceForFill',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S 2:25 -S 26 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S 2:25 -S 26 192.168.0.0/24',
		'ipcalc: 192.168.0.0/24 is too small for the requested subnets'
	]
)
test('SplitVLSMTwoFills',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S 26 -S 27 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S 26 -S 27 192.168.0.0/24',
		'ipcalc: only one split prefix may be given without a count'
	]
)
test('SplitVLSMZeroCount',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S 0:26 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S 0:26 192.168.0.0/24',
		'ipcalc: bad split count: 0:26'
	]
)
test('SplitVLSMBadCount',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S x:26 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S x:26 192.168.0.0/24',
		'ipcalc: bad split count: x:26'
	]
)
test('SplitVLSMCountOverflow',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S 99999999999999999999:26 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S 99999999999999999999:26 192.168.0.0/24',
		'ipcalc: bad split count: 99999999999999999999:26'
	]
)
test('SplitVLSMBadPrefix',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S 1:abc 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' -S 1:abc 192.168.0.0/24',
		'ipcalc: bad IPv4 prefix: 1:abc'
	]
)

# --split-hosts tests
test('SplitHosts',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --split-hosts=10,20,30 192.168.0.0/24',
		files('split-hosts-192.168.0.0-24-10,20,30')
	]
)
test('NoDecorateSplitHosts',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate --split-hosts=10,20,30 192.168.0.0/24',
		files('nsplit-hosts-192.168.0.0-24-10,20,30')
	]
)
test('JsonSplitHosts',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j --split-hosts=10,20,30 192.168.0.0/24',
		files('json-split-hosts-192.168.0.0-24-10,20,30')
	]
)
test('SplitHostsSmallest',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --split-hosts=1,2,62,63 192.168.0.0/24',
		files('split-hosts-192.168.0.0-24-1,2,62,63')
	]
)
test('SplitHostsRepeated',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --split-hosts=1,2 --split-hosts=62,63 192.168.0.0/24',
		files('split-hosts-192.168.0.0-24-1,2,62,63')
	]
)
test('SplitHostsExactFit',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate --split-hosts=126,62,62 192.168.0.0/24',
		files('nsplit-hosts-192.168.0.0-24-126,62,62')
	]
)
test('SplitHostsIPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --split-hosts=1,2,3,256 2001:db8::/112',
		files('split-hosts-2001:db8::-112-1,2,3,256')
	]
)
test('JsonSplitHostsWholeIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j --split-hosts=4294967294 0.0.0.0/0',
		files('json-split-hosts-0.0.0.0-0-4294967294')
	]
)
test('SplitHostsTooSmall',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=200,100 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=200,100 192.168.0.0/24',
		'ipcalc: 192.168.0.0/24 is too small for the requested subnets'
	]
)
test('SplitHostsLargerThanNetwork',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=300 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=300 192.168.0.0/24',
		'ipcalc: 192.168.0.0/24 is too small for the requested subnets'
	]
)
test('SplitHostsTooManyHosts',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=4294967295 0.0.0.0/0',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=4294967295 0.0.0.0/0',
		'ipcalc: too many hosts requested: 4294967295'
	]
)
test('SplitHostsWithSplit',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=10 -S 26 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=10 -S 26 192.168.0.0/24',
		'ipcalc: --split-hosts cannot be combined with --split'
	]
)
test('SplitHostsZero',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=0 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=0 192.168.0.0/24',
		'ipcalc: bad host count: 0'
	]
)
test('SplitHostsEmptyItem',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=1,,2 192.168.0.0/24',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=1,,2 192.168.0.0/24',
		'ipcalc: bad host count: 1,,2'
	]
)
test('SplitHostsOverflow',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --split-hosts=18446744073709551616 2001:db8::/64',
		'--test-output',
		ipcalc.full_path() + ' --split-hosts=18446744073709551616 2001:db8::/64',
		'ipcalc: bad host count: 18446744073709551616'
	]
)

# --no-decorate tests
test('NoDecorateSplitPrefix18',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 18 10.10.10.10/16',
		files('nsplit-10.10.10.0-16-18')
	]
)
test('NoDecorateSplitPrefix24',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 24 10.10.10.0/16',
		files('nsplit-10.10.10.0-16-24')
	]
)
test('NoDecorateSplitPrefix26',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 26 192.168.5.45/24',
		files('nsplit-192.168.5.45-24-26')
	]
)
test('NoDecorateSplitPrefix29',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 29 192.168.5.0/24',
		files('nsplit-192.168.5.0-24-29')
	]
)
test('NoDecorateSplitPrefix31',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 31 192.168.5.0/24',
		files('nsplit-192.168.5.0-24-31')
	]
)
test('NoDecorateSplitPrefix32',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 32 192.168.5.0/24',
		files('nsplit-192.168.5.0-24-32')
	]
)
test('NoDecorateSplitPrefix64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 64 2a03:2880:20:4f06:face::/56',
		files('nsplit-2a03:2880:20:4f06:face::-56-64')
	]
)
test('NoDecorateSplitPrefix128',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 128 fcfa:b4ca:f1d8:125b:dc00::/127',
		files('nsplit-fcfa:b4ca:f1d8:125b:dc00::-127-128')
	]
)
test('NoDecorateSplitPrefix120',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -S 120 fcfa:b4ca:f1d8:125b:dc00::/112',
		files('nsplit-fcfa:b4ca:f1d8:125b:dc00::-112-120')
	]
)

# --addrspace tests
test('AllocateAddressSpacePrefix24',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --addrspace -abmnp 193.92.150.3/24',
		files('193.92.150.3-24')
	]
)
test('AllocateAddressSpacePrefix64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --addrspace -abmnp fd95:6be5:0ae0:84a5::/64',
		files('fd95:6be5:0ae0:84a5::-64')
	]
)
test('AllocateAddressSpacePrefix48',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --addrspace -abmnp fd0b:a336:4e7d::/48',
		files('fd0b:a336:4e7d::-48')
	]
)

# --info tests
test('TestHumanReadableGenericInfoNoPrefix',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2a03:2880:20:4f06:face:b00c:0:1',
		files('i-2a03:2880:20:4f06:face:b00c:0:1')
	]
)
test('TestHumanReadableGenericInfoInternetIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 3.130.45.15',
		files('i-3.130.45.15')
	]
)
test('TestHumanReadableThisNetwork',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 0.0.0.1',
		files('i-0.0.0.1')
	]
)
test('TestHumanReadablePrivateUse1',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 10.0.0.1',
		files('i-10.0.0.1')
	]
)
test('TestHumanReadablePrivateUse2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 172.16.0.1',
		files('i-172.16.0.1')
	]
)
test('TestHumanReadablePrivateUse3',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 172.31.255.254',
		files('i-172.31.255.254')
	]
)
test('TestHumanReadablePrivateUse4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.168.0.1',
		files('i-192.168.0.1')
	]
)
test('TestHumanReadableSharedAddressSpace',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 100.64.0.1',
		files('i-100.64.0.1')
	]
)
test('TestHumanReadableLoopback',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 127.0.0.1',
		files('i-127.0.0.1')
	]
)
test('TestHumanReadableLoopback2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 127.0.1.1',
		files('i-127.0.1.1')
	]
)
test('TestHumanReadableLinkLocal',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 169.254.0.1',
		files('i-169.254.0.1')
	]
)
test('TestHumanReadableIETFProtocolAssignmentsIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.254',
		files('i-192.0.0.254')
	]
)
test('TestHumanReadableServiceContinuityPrefixIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.6',
		files('i-192.0.0.6')
	]
)
test('TestHumanReadableDummyAddressIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.8',
		files('i-192.0.0.8')
	]
)
test('TestHumanReadablePortControlProtocolAnycastIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.9',
		files('i-192.0.0.9')
	]
)
test('TestHumanReadableTraversalUsingRelaysAroundNATAnycast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.10',
		files('i-192.0.0.10')
	]
)
test('TestHumanReadableNAT64DNS64Discovery1',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.170',
		files('i-192.0.0.170')
	]
)
test('TestHumanReadableNAT64DNS64Discovery2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.0.171',
		files('i-192.0.0.171')
	]
)
test('TestHumanReadableAS112v4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.31.196.1',
		files('i-192.31.196.1')
	]
)
test('TestHumanReadableDeprecated6to4RelayAnycast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.88.99.1',
		files('i-192.88.99.1')
	]
)
test('TestHumanReadableAMTIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.52.193.5',
		files('i-192.52.193.5')
	]
)
test('TestHumanReadableDirectDelegationAS112ServiceIPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.175.48.12',
		files('i-192.175.48.12')
	]
)
test('TestHumanReadableTestNet1',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.0.2.1',
		files('i-192.0.2.1')
	]
)
test('TestHumanReadableTestNet2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 198.51.100.1',
		files('i-198.51.100.1')
	]
)
test('TestHumanReadableTestNet3',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 203.0.113.1',
		files('i-203.0.113.1')
	]
)
test('TestHumanReadableBenchmarking1',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 198.18.0.1',
		files('i-198.18.0.1')
	]
)
test('TestHumanReadableBenchmarking2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 198.19.201.71',
		files('i-198.19.201.71')
	]
)
test('TestHumanReadableBenchmarking3',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 198.19.255.254',
		files('i-198.19.255.254')
	]
)
test('TestHumanReadableMulticast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 224.0.0.1',
		files('i-224.0.0.1')
	]
)
test('TestHumanReadableReserved1',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 240.0.0.1',
		files('i-240.0.0.1')
	]
)
test('TestHumanReadableReserved2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 250.7.93.1',
		files('i-250.7.93.1')
	]
)
test('TestHumanReadableReserved3',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 255.255.255.254',
		files('i-255.255.255.254')
	]
)
test('TestHumanReadableLimitedBroadcast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 255.255.255.255',
		files('i-255.255.255.255')
	]
)
test('TestHumanReadableTranslat96',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 64:ff9b::2100:951a',
		files('i-64:ff9b::2100:951a')
	]
)
test('TestHumanReadableTranslat48',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 64:ff9b:1:ea21:4316:55::1',
		files('i-64:ff9b:1:ea21:4316:55::1')
	]
)
test('TestHumanReadableTEREDO',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:0000:50::d78a:0:c',
		files('i-2001:0:50::d78a:0:c')
	]
)
test('TestHumanReadablePortControlProtocolAnycast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:1::1',
		files('i-2001:1::1')
	]
)
test('TestHumanReadableTraversalUsingRelays',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:1::2',
		files('i-2001:1::2')
	]
)
test('TestHumanReadableBenchmarking',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:2:0:a::5:b7f',
		files('i-2001:2:0:a::5:b7f')
	]
)
test('TestHumanReadableAMT',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:3:89:71::ab:1',
		files('i-2001:3:89:71::ab:1')
	]
)
test('TestHumanReadableAS112v6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:4:112::6483:bd12:9010',
		files('i-2001:4:112::6483:bd12:9010')
	]
)
test('TestHumanReadableORCHID',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:1e:ab:7::',
		files('i-2001:1e:ab:7::')
	]
)
test('TestHumanReadableORCHIDv2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:2c:ab:7::',
		files('i-2001:2c:ab:7::')
	]
)
test('TestHumanReadableDocumentation',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:db8:329::4a',
		files('i-2001:db8:329::4a')
	]
)
test('TestHumanReadableIETFProtocolAssignments',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:190:70::e86a:0:b',
		files('i-2001:190:70::e86a:0:b')
	]
)
test('TestHumanReadable6to4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2002::34',
		files('i-2002::34')
	]
)
test('TestHumanReadableDirectDelegationAS112Service',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2620:4f:8000::7372:ad1',
		files('i-2620:4f:8000::7372:ad1')
	]
)
test('TestHumanReadableUniqueLocalUnicast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i fc00:0:a001::bcdf:4',
		files('i-fc00:0:a001::bcdf:4')
	]
)
test('TestHumanReadableLinkScopedUnicast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i fe80::b:0:7acd:0:5',
		files('i-fe80::b:0:7acd:0:5')
	]
)
test('TestHumanReadableThisHostOnThisNetwork',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 0.0.0.0',
		files('i-0.0.0.0')
	]
)
test('TestHumanReadable6a44Relay',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.88.99.2',
		files('i-192.88.99.2')
	]
)
test('AddrspaceIPv4LargerThanBlock',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addrspace 192.168.0.0/8',
		'ADDRSPACE=Internet'
	]
)
test('AddrspaceIPv4LargerThanPrivateBlock',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addrspace 10.0.0.0/7',
		'ADDRSPACE=Internet'
	]
)
test('AddrspaceIPv4LargerThanThisNetwork',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addrspace 0.0.0.0/7',
		'ADDRSPACE=Internet'
	]
)
test('TestHumanReadableDiscardOnly',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 100::1',
		files('i-100::1')
	]
)
test('AddrspaceNotDiscardOnly',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addrspace 1000::1',
		'ADDRSPACE=Reserved'
	]
)
test('TestHumanReadableDummyIPv6Prefix',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 100:0:0:1::5',
		files('i-100:0:0:1::5')
	]
)
test('TestHumanReadableDNSSDSRPAnycast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:1::3',
		files('i-2001:1::3')
	]
)
test('TestHumanReadableDRIPDET',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 2001:30::1',
		files('i-2001:30::1')
	]
)
test('TestHumanReadableDocumentation3fff',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 3fff::1',
		files('i-3fff::1')
	]
)
test('TestHumanReadableSRv6SIDs',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 5f00::1',
		files('i-5f00::1')
	]
)
test('AddrspaceIPv6LargerThanBlock',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addrspace 2002::/15',
		'ADDRSPACE="Global Unicast"'
	]
)
test('TestHumanReadableGenericInfoPrefix48',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i fd0b:a336:4e7d::/48',
		files('i-fd0b:a336:4e7d::-48')
	]
)

# --split advanced tests
test('SplitLinesByNetworkIPv4',
	testrunner,
	args : [
		'--test-equal',
		ipcalc.full_path() + ' -S 29 192.168.5.0/24 | grep ^Network | wc -l | tr -d [:space:]',
		ipcalc.full_path() + ' -S 29 192.168.5.0/24 | grep ^Total | cut -d: -f 2 | tr -d [:space:]'
	]
)
test('SplitLinesByNetworkIPv6',
	testrunner,
	args : [
		'--test-equal',
		ipcalc.full_path() + ' -S 120 fcfa:b4ca:f1d8:125b:dc00::/112 | grep ^Network | wc -l | tr -d [:space:]',
		ipcalc.full_path() + ' -S 120 fcfa:b4ca:f1d8:125b:dc00::/112 | grep ^Total|cut -d: -f 2 | tr -d [:space:]'
	]
)

# --help test
test('TestHelpCommand',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' --help'
	]
)
# -g/--geoinfo exist only with a geo backend
if have_geo
	test('GeoInfoAccepted',
		testrunner,
		args : [
			'--test-success',
			ipcalc.full_path() + ' -g 127.0.0.1'
		]
	)
else
	test('GeoInfoShortUnavailable',
		testrunner,
		args : [
			'--test-failure',
			ipcalc.full_path() + ' -g 127.0.0.1'
		]
	)
	test('GeoInfoLongUnavailable',
		testrunner,
		args : [
			'--test-failure',
			ipcalc.full_path() + ' --geoinfo 127.0.0.1'
		]
	)
	test('GeoInfoNotInUsage',
		testrunner,
		args : [
			'--test-failure',
			ipcalc.full_path() + ' --usage 2>&1 | grep -e -g -e geoinfo'
		]
	)
endif
test('TestShortHelpCommand',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -?'
	]
)
test('UnknownLongOption',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' --bogus 1.2.3.4'
	]
)
test('UnknownShortOption',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -y 1.2.3.4'
	]
)
test('MissingOptionArgument',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -S'
	]
)

# --check loopback tests
test('ValidateLoopbackIPv4',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c 127.0.0.1'
	]
)
test('ValidateLoopbackExplicitIPv6',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c -6 ::1'
	]
)
test('ValidateLoopbackIPv6',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c ::1'
	]
)

# --check tests
test('ValidatePrivateIPv4',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c 192.168.1.1'
	]
)
test('ValidateAbbreviatedGlobalIPv6',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c -6 2a01:198:200:300::2'
	]
)
test('ValidateExpandedGlobalIPv6',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c -6 2a01:198:200:300:0000:0000:0000:2'
	]
)
test('ValidateCompleteGlobalIPv6',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c -6 2a01:0198:0200:0300:0000:0000:0000:0002'
	]
)
test('ValidateLoopbackIPv6WithPrefix',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c -6 ::1/128'
	]
)
test('ValidateReservedIPv6WithPrefix',
	testrunner,
	args : [
		'--test-success',
		ipcalc.full_path() + ' -c -6 fec0::1:0:0:c0a8:8002/64'
	]
)

# --check invalid input tests
test('NoValidateInvalidHexIPv6',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c -6 gggg::'
	]
)
# FIXME: This test case unexpectedly works, so it is disabled for now.
#test('NoValidateInvalidBroadcastIPv6',
#	testrunner,
#	args : [
#		'--test-failure',
#		ipcalc.full_path() + ' -b -6 ::1/128'
#	]
#)
test('NoAllowIPv4AndIPv6',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -4 -6 2a01:198:200:300:0000:0000:0000:2'
	]
)
test('NoValidateIPv6InIPv4Mode',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c -4 2a01:198:200:300:0000:0000:0000:2'
	]
)
test('NoValidateIPv4AndIPv6WithIPv4Address',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c -4 -6 127.0.0.1'
	]
)
test('NoValidateInvalidPrefixIPv6',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -c ::1/999'
	]
)

# --netmask invalid input tests
test('NoValidateNegativePrefix',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -m 192.168.1.1/-1'
	]
)
test('NoValidateIPv6PrefixInIPv4Mode',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -m 192.168.1.1/64'
	]
)
test('NoValidateInvalidPrefixIPv4',
	testrunner,
	args : [
		'--test-failure',
		ipcalc.full_path() + ' -m 192.168.1.1/99999'
	]
)

# --broadcast output tests
test('CalculateBroadcastAddressFromPrefixIPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -b 192.168.1.1/24',
		'BROADCAST=192.168.1.255'
	]
)
test('CalculateBroadcastAddressFromNetmaskIPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -b 192.168.1.1 255.255.255.0',
		'BROADCAST=192.168.1.255'
	]
)

# --netmask output tests
test('CalculateNetmaskPrefix24',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -m 192.168.1.1/24',
		'NETMASK=255.255.255.0'
	]
)
test('CalculateNetmaskPrefix22',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -m 172.16.59.222/22',
		'NETMASK=255.255.252.0'
	]
)
test('CalculateNetmaskPrefix1',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -m 10.0.0.0/1',
		'NETMASK=128.0.0.0'
	]
)
test('CalculateNetmaskNoPrefix',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -m 192.168.1.1',
		'NETMASK=255.255.255.255'
	]
)

# --prefix output tests
test('CalculatePrefix24FromNetmask',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -p 192.168.1.1 255.255.255.0',
		'PREFIX=24'
	]
)
test('CalculatePrefix32FromNetmask',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -p 192.168.1.1 255.255.255.255',
		'PREFIX=32'
	]
)
test('CalculatePrefix0FromNetmask',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -p 192.168.1.1 0.0.0.0',
		'PREFIX=0'
	]
)
test('CalculatePrefix22FromNetmask',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -p 172.16.59.222 255.255.252.0',
		'PREFIX=22'
	]
)

# --class-prefix output tests
test('CalculateClassNetmask24',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --class-prefix -m 192.168.1.1',
		'NETMASK=255.255.255.0'
	]
)
test('CalculateClassNetmask8',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --class-prefix -m 10.1.2.3',
		'NETMASK=255.0.0.0'
	]
)
test('CalculateClassNetmask16',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --class-prefix -m 129.22.4.3',
		'NETMASK=255.255.0.0'
	]
)

# --network output tests
test('CalculateNetworkFromPrefix32',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -n 192.168.1.1/32',
		'NETWORK=192.168.1.1'
	]
)
test('CalculateNetworkFromPrefix0',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' -n 192.168.1.1/0',
		'NETWORK=0.0.0.0'
	]
)
test('AddressesPrefix0IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addresses 0.0.0.0/0',
		'ADDRESSES=4294967294'
	]
)
test('AddressesPrefix1IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addresses 10.0.0.0/1',
		'ADDRESSES=2147483646'
	]
)
test('AddressesPrefix0IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addresses ::/0',
		'ADDRESSES=340282366920938463463374607431768211456'
	]
)

# --reverse-dns output tests
test('LookupReverseDNSFromPrefix32IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 193.92.150.3/32',
		'REVERSEDNS=3.150.92.193.in-addr.arpa.'
	]
)
test('LookupReverseDNSFromPrefix26IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 193.92.150.3/26',
		'REVERSEDNS=0-63.150.92.193.in-addr.arpa.'
	]
)
test('LookupReverseDNSFromPrefix16IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 193.92.150.3/16',
		'REVERSEDNS=92.193.in-addr.arpa.'
	]
)
test('LookupReverseDNSFromPrefix12IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 193.92.150.3/12',
		'REVERSEDNS=80-95.193.in-addr.arpa.'
	]
)

test('LookupReverseDNSFromPrefix4IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 193.92.150.3/4',
		'REVERSEDNS=192-207.in-addr.arpa.'
	]
)

test('LookupReverseDNSFromPrefix0IPv4',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 193.92.150.3/0',
		'REVERSEDNS=in-addr.arpa.'
	]
)
test('LookupReverseDNS4321IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 4321:0:1:2:3:4:567:89ab',
		'REVERSEDNS=b.a.9.8.7.6.5.0.4.0.0.0.3.0.0.0.2.0.0.0.1.0.0.0.0.0.0.0.1.2.3.4.ip6.arpa.'
	]
)
test('LookupReverseDNSFirstIPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 1::',
		'REVERSEDNS=0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.1.0.0.0.ip6.arpa.'
	]
)
test('LookupReverseDNSLoopbackIPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns ::1',
		'REVERSEDNS=1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa.'
	]
)
test('LookupReverseDNSFromPrefix128IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 1234::4321/128',
		'REVERSEDNS=1.2.3.4.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.4.3.2.1.ip6.arpa.'
	]
)
test('LookupReverseDNSFromPrefix124IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 1234::4321/124',
		'REVERSEDNS=2.3.4.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.4.3.2.1.ip6.arpa.'
	]
)
test('LookupReverseDNSFromPrefix120IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 1234::4321/120',
		'REVERSEDNS=3.4.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.4.3.2.1.ip6.arpa.'
	]
)

test('LookupReverseDNSFromPrefix0IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns ::/0',
		'REVERSEDNS=ip6.arpa.'
	]
)

test('LookupReverseDNSFromPrefix31IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 2001:db8::/31',
		'REVERSEDNS="8.b.d.0.1.0.0.2.ip6.arpa. 9.b.d.0.1.0.0.2.ip6.arpa."'
	]
)

test('LookupReverseDNSFromPrefix33IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 2001:db8:8000::/33',
		'REVERSEDNS="8.8.b.d.0.1.0.0.2.ip6.arpa. 9.8.b.d.0.1.0.0.2.ip6.arpa. a.8.b.d.0.1.0.0.2.ip6.arpa. b.8.b.d.0.1.0.0.2.ip6.arpa. c.8.b.d.0.1.0.0.2.ip6.arpa. d.8.b.d.0.1.0.0.2.ip6.arpa. e.8.b.d.0.1.0.0.2.ip6.arpa. f.8.b.d.0.1.0.0.2.ip6.arpa."'
	]
)

test('LookupReverseDNSFromPrefix34IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 2001:db8:4000::/34',
		'REVERSEDNS="4.8.b.d.0.1.0.0.2.ip6.arpa. 5.8.b.d.0.1.0.0.2.ip6.arpa. 6.8.b.d.0.1.0.0.2.ip6.arpa. 7.8.b.d.0.1.0.0.2.ip6.arpa."'
	]
)

test('LookupReverseDNSFromPrefix127IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 1234::4321/127',
		'REVERSEDNS="0.2.3.4.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.4.3.2.1.ip6.arpa. 1.2.3.4.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.4.3.2.1.ip6.arpa."'
	]
)

test('LookupReverseDNSUnmaskedPrefix35IPv6',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --reverse-dns 2001:db8:3fff::1/35',
		'REVERSEDNS="2.8.b.d.0.1.0.0.2.ip6.arpa. 3.8.b.d.0.1.0.0.2.ip6.arpa."'
	]
)

test('LookupReverseDNSNoDecoratePrefix37IPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --reverse-dns --no-decorate 2001:db8:abcd::/37',
		files('reverse-dns-no-decorate-2001:db8:abcd::-37')
	]
)

test('AllInfoReverseDNSPrefix31IPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 2001:db8::/31',
		files('all-info-2001:db8::-31')
	]
)

test('JsonAllInfoReverseDNSPrefix31IPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info --json 2001:db8::/31',
		files('json-all-info-2001:db8::-31')
	]
)

test('JsonAllInfoReverseDNSPrefix32IPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info --json 2001:db8::/32',
		files('json-all-info-2001:db8::-32')
	]
)

test('JsonAllInfoReverseDNSPrefix24IPv4',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info --json 192.168.2.7/24',
		files('json-all-info-192.168.2.7-24')
	]
)

# specific info decorated & no-decorate test
test('SpecificInfoOutput',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp --minaddr --maxaddr --addresses 192.168.2.6/24',
		files('192.168.2.6')
	]
)
test('SpecificInfoOutputNoDecorate',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -abmnp --minaddr --maxaddr --addresses --no-decorate 192.168.2.7/24',
		files('192.168.2.7')
	]
)
test('SpecificInfoOutputNoDecorateQuotedAddrspace',
	testrunner,
	args : [
		'--test-output',
		ipcalc.full_path() + ' --addrspace --no-decorate 10.0.0.1',
		'"Private Use"'
	]
)

# --json output tests
test('JsonSplitPrefix24',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -S 24 10.10.10.0/16',
		files('json-split-10.10.10.0-16-24')
	]
)
test('JsonSplitPrefix26',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -S 26 192.168.5.45/24',
		files('json-split-192.168.5.45-24-26')
	]
)
test('JsonNoDecorateSplitPrefix24',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j --no-decorate -S 24 10.10.10.0/16',
		files('json-nsplit-10.10.10.0-16-24')
	]
)
test('JsonNoDecorateSplitPrefix26',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j --no-decorate -S 26 192.168.5.45/24',
		files('json-nsplit-192.168.5.45-24-26')
	]
)
test('JsonSplitPrefix64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -S 64 2a03:2880:20:4f06:face::/56',
		files('json-nsplit-2a03:2880:20:4f06:face::-56-64')
	]
)
test('JsonNoDecorateSplitPrefix64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j --no-decorate -S 64 2a03:2880:20:4f06:face::/56',
		files('json-nsplit-2a03:2880:20:4f06:face::-56-64')
	]
)
test('JsonTestHumanReadableGenericInfoPrefix56',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -i 2a03:2880:20:4f06:face:b00c:0:1/56',
		files('json-i-2a03:2880:20:4f06:face:b00c:0:1-56')
	]
)
test('JsonTestHumanReadableSpecificInfo',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -n 2a03:2880:20:4f06:face:b00c:0:1/56',
		files('json-i-2a03:2880:20:4f06:face:b00c:0:1-56')
	]
)
test('AllInfo',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 192.168.2.7/24',
		files('all-info-192.168.2.7')
	]
)
test('AllInfoClassDLowest',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 224.0.0.1',
		files('all-info-224.0.0.1')
	]
)
test('AllInfoClassDHighestOctet',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 239.1.1.1',
		files('all-info-239.1.1.1')
	]
)
test('AllInfoClassE',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 240.0.0.1',
		files('all-info-240.0.0.1')
	]
)
test('AllInfoIIDLinkLocalUniversal',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info fe80::21b:21ff:fe3a:5c7d/64',
		files('all-info-fe80::21b:21ff:fe3a:5c7d-64')
	]
)
test('JsonAllInfoIIDLinkLocalUniversal',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info --json fe80::21b:21ff:fe3a:5c7d/64',
		files('json-all-info-fe80::21b:21ff:fe3a:5c7d-64')
	]
)
test('NoDecorateAllInfoIIDLinkLocalUniversal',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info --no-decorate fe80::21b:21ff:fe3a:5c7d/64',
		files('no-decorate-all-info-fe80::21b:21ff:fe3a:5c7d-64')
	]
)
test('AllInfoIIDLinkLocalLocal',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info fe80::ff:fe00:1',
		files('all-info-fe80::ff:fe00:1')
	]
)
test('AllInfoIIDGroup',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 2001:db8::300:ff:fe00:1',
		files('all-info-2001:db8::300:ff:fe00:1')
	]
)
test('AllInfoIIDNotEUI64',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info 2001:db8::f00d:1234:5678:9abc/64',
		files('all-info-2001:db8::f00d:1234:5678:9abc-64')
	]
)
test('AllInfoIIDNetwork',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info fe80::/64',
		files('all-info-fe80::-64')
	]
)
test('AllInfoIIDMapped',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info ::ffff:192.0.2.1',
		files('all-info-::ffff:192.0.2.1')
	]
)
test('AllInfoIIDMulticast',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --all-info ff02::21b:21ff:fe3a:5c7d',
		files('all-info-ff02::21b:21ff:fe3a:5c7d')
	]
)
test('InfoIIDLinkLocal',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i fe80::21b:21ff:fe3a:5c7d/64',
		files('i-fe80::21b:21ff:fe3a:5c7d-64')
	]
)
test('Info',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -i 192.168.2.7/24',
		files('info-192.168.2.7')
	]
)
test('InfoImplicit',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' 192.168.2.7/24',
		files('info-192.168.2.7')
	]
)

# --deagrregate tests
test('DeaggregateNoDecorate',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -d 192.168.2.7-192.168.2.13',
		files('deaggregate-192.168.2.7-192.168.2.13')
	]
)
test('Deaggregate',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d 192.168.2.0-192.168.3.255',
		files('deaggregate-192.168.2.0-192.168.3.255')
	]
)
test('DeaggregateLargeNum',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d 240.0.0.0-255.255.255.255',
		files('deaggregate-240.0.0.0-255.255.255.255')
	]
)
test('DeaggregateLargeNum2',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d 127.0.0.0-255.255.255.255',
		files('deaggregate-127.0.0.0-255.255.255.255')
	]
)
test('DeaggregateLargeNum3',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d 0.0.0.0-255.255.255.255',
		files('deaggregate-0.0.0.0-255.255.255.255')
	]
)
test('DeaggregateSpaces',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d "  192.168.2.0   - 192.168.3.255   "',
		files('deaggregate-192.168.2.0-192.168.3.255')
	]
)
test('DeaggregateJson',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -d 192.168.2.33-192.168.3.2',
		files('deaggregate-192.168.2.33-192.168.3.2')
	]
)
test('DeaggregateIPv6NoDecorate',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' --no-decorate -d 2a03:2880:20:4f06:face::0-2a03:2880:20:4f06:face::fffe',
		files('deaggregate-2a03:2880:20:4f06:face::0-2a03:2880:20:4f06:face::fffe')
	]
)
test('DeaggregateIPv6',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d fcd3:57d1:733:c18f:b498:25e1:788:0-fcd3:57d1:733:c18f:b498:25e1:788:ffff',
		files('deaggregate-fcd3:57d1:733:c18f:b498:25e1:788:0-fcd3:57d1:733:c18f:b498:25e1:788:ffff')
	]
)
test('DeaggregateIPv6Json',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -j -d fcd3:57d1:733:c18f:b498:25e1:788:f-fcd3:57d1:733:c18f:b498:25e1:789:ffa9',
		files('deaggregate-fcd3:57d1:733:c18f:b498:25e1:788:f-fcd3:57d1:733:c18f:b498:25e1:789:ffa9')
	]
)
test('DeaggregateIPv6All',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d ::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff',
		files('deaggregate-::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff')
	]
)
test('DeaggregateIPv6UpperHalf',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d 8000::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff',
		files('deaggregate-8000::-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff')
	]
)
test('DeaggregateIPv6LastAddress',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff',
		files('deaggregate-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff-ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff')
	]
)
test('DeaggregateIPv6LowerHalf',
	testrunner,
	args : [
		'--test-outfile',
		ipcalc.full_path() + ' -d ::-8000::',
		files('deaggregate-::-8000::')
	]
)
test('StandardInfo',
	testrunner,
	args :	[	'--test-outfile',
			ipcalc.full_path() + ' 192.168.2.0/24',
			files('standard-192.168.2.0-24')
	]
)
test('NoDecorateInfo',
	testrunner,
	args :	[	'--test-outfile',
			ipcalc.full_path() + ' --no-decorate 192.168.2.0/24',
			files('no-decorate-192.168.2.0-24')
	]
)
test('NoDecorateAllInfo',
	testrunner,
	args :	[	'--test-outfile',
			ipcalc.full_path() + ' --all-info --no-decorate 192.168.2.0/24',
			files('no-decorate-all-info-192.168.2.0-24')
	]
)
# Test whether we can deaggregate a randomly generated network
test('DeaggregateIPv6Random',
	find_program('ipcalc-delegate-ipv6-random.sh'),
	env : ['IPCALC=' + ipcalc.full_path()]
)
test('DeaggregateSplitIPv6',
	find_program('ipcalc-delegate-split-ipv6.sh'),
	env : ['IPCALC=' + ipcalc.full_path()]
)
test('DeaggregateSplitIPv4',
	find_program('ipcalc-delegate-split-ipv4.sh'),
	env : ['IPCALC=' + ipcalc.full_path()]
)
# Compare --split-hosts with the Debian ipcalc (REQ-SPLIT-COMPAT-001); the
# Debian script is only in the git tree, not in release tarballs. The tests
# need perl and fail when it is missing (REQ-GEN-TEST-007).
if import('fs').exists('debian-ipcalc/ipcalc')
	debian_split_hosts = find_program('ipcalc-debian-split-hosts.sh')
	foreach c : [
		['192.168.0.0/24', '10,20,30'],
		['192.168.0.0/24', '1,2,62,63'],
		['192.168.0.0/24', '126,62,62'],
		['192.168.0.0/24', '5,5,5'],
		['192.168.0.0/24', '30,30,30,30,30,30,30,30'],
		['192.168.5.45/24', '10,20'],
		['192.168.1.0/24', '254'],
		['10.0.0.0/30', '2'],
		['172.16.0.0/12', '1'],
		['10.0.0.0/8', '1000,5,5,70000,300,2'],
		['10.0.0.0/16', '2000,1000,500,250,120,60,30,14,6,2,1'],
		['10.20.30.0/23', '100,100,50,3,3,1'],
	]
		test('DebianSplitHosts-' + c[0].replace('/', '-') + '-' + c[1],
			debian_split_hosts,
			args : c,
			env : ['IPCALC=' + ipcalc.full_path(),
			       'DEBIAN_IPCALC=' + meson.current_source_dir() / 'debian-ipcalc' / 'ipcalc']
		)
	endforeach
endif

# Network comparison: [name, expected exit status, arguments, expected
# output]. The output of errors (status 2) is not compared.
foreach c : [
	['EqualsHostBits', '0', '192.168.7.15/24 --equals=192.168.7.5/24', 'The networks are equal'],
	['EqualsNetmask', '0', '192.168.7.15 255.255.255.0 --equals=192.168.7.5/255.255.255.0', 'The networks are equal'],
	['EqualsClassPrefix', '0', '--class-prefix 10.0.0.0/8 --equals=10.9.9.9', 'The networks are equal'],
	['EqualsJson', '0', '-j 192.168.7.15/24 --equals=192.168.7.5/24', ''],
	['EqualsNoDecorate', '1', '--no-decorate 192.168.7.15/24 --equals=192.168.7.5/25', ''],
	['SubnetOfNoDecorate', '0', '--no-decorate 192.168.0.1/24 --subnet-of=192.168.1.15/23', ''],
	['OverlapsSilent', '1', '-s 10.0.0.0/8 --overlaps=::/0', ''],
	['EqualsHost32', '0', '192.168.1.15 --equals=192.168.1.15/32', 'The networks are equal'],
	['EqualsHostBitsIPv6', '0', '2001:db8::1/64 --equals=2001:db8::ffff/64', 'The networks are equal'],
	['EqualsPrefix0', '0', '1.2.3.4/0 --equals=0.0.0.0/0', 'The networks are equal'],
	['EqualsDifferentPrefix', '1', '192.168.0.0/24 --equals=192.168.0.0/23', 'The networks are not equal'],
	['EqualsDisjoint', '1', '192.168.1.15/24 --equals=192.168.7.1/24', 'The networks are not equal'],
	['EqualsDifferentPrefixIPv6', '1', '2001:db8::/64 --equals=2001:db8::/65', 'The networks are not equal'],
	['SubnetOf', '0', '192.168.0.1/24 --subnet-of=192.168.1.15/23', '192.168.0.0/24 is a subnet of 192.168.0.0/23'],
	['SubnetOfEqual', '0', '10.0.0.0/8 --subnet-of=10.0.0.0/8', '10.0.0.0/8 is a subnet of 10.0.0.0/8'],
	['SubnetOfPartialByte', '0', '10.64.0.0/10 --subnet-of=10.0.0.0/9', '10.64.0.0/10 is a subnet of 10.0.0.0/9'],
	['SubnetOfPrefix0', '0', '10.1.2.3 --subnet-of=0.0.0.0/0', '10.1.2.3/32 is a subnet of 0.0.0.0/0'],
	['SubnetOfIPv6', '0', '2001:db8:1::/48 --subnet-of=2001:db8::/32', '2001:db8:1::/48 is a subnet of 2001:db8::/32'],
	['SubnetOfPrefix0IPv6', '0', '::1 --subnet-of=::/0', '::1/128 is a subnet of ::/0'],
	['SubnetOfSupernet', '1', '192.168.1.15/23 --subnet-of=192.168.0.1/24', '192.168.0.0/23 is not a subnet of 192.168.0.0/24'],
	['SubnetOfSiblingPartialByte', '1', '10.128.0.0/10 --subnet-of=10.0.0.0/9', '10.128.0.0/10 is not a subnet of 10.0.0.0/9'],
	['SubnetOfPrefix0Supernet', '1', '0.0.0.0/0 --subnet-of=0.0.0.0/1', '0.0.0.0/0 is not a subnet of 0.0.0.0/1'],
	['SubnetOfSupernetIPv6', '1', '2001:db8::/32 --subnet-of=2001:db8:1::/48', '2001:db8::/32 is not a subnet of 2001:db8:1::/48'],
	['OverlapsSupernet', '0', '192.168.1.15/23 --overlaps=192.168.0.1/24', 'The networks overlap'],
	['OverlapsSubnet', '0', '192.168.0.1/24 --overlaps=192.168.1.15/23', 'The networks overlap'],
	['OverlapsHost', '0', '10.0.0.1 --overlaps=10.0.0.1', 'The networks overlap'],
	['OverlapsPrefix0IPv6', '0', '::/0 --overlaps=2001:db8::1', 'The networks overlap'],
	['OverlapsDisjoint', '1', '192.168.1.15/24 --overlaps=192.168.7.1/24', 'The networks do not overlap'],
	['OverlapsAdjacent', '1', '192.168.0.0/24 --overlaps=192.168.1.0/24', 'The networks do not overlap'],
	['OverlapsAdjacentIPv6', '1', '2001:db8::/33 --overlaps=2001:db8:8000::/33', 'The networks do not overlap'],
	['OverlapsMixedFamily', '1', '10.0.0.0/8 --overlaps=::/0', 'The networks do not overlap'],
	['EqualsMappedIPv4', '1', '::ffff:10.0.0.1 --equals=10.0.0.1', 'The networks are not equal'],
	['EqualsBadAddress', '2', '10.0.0.0/8 --equals=1.2.3', ''],
	['SubnetOfBadPrefixIPv6', '2', '::/0 --subnet-of=::/129', ''],
	['OverlapsBadNetmask', '2', '10.0.0.0/8 --overlaps=10.0.0.0/255.0.255.0', ''],
	['OverlapsForcedFamily', '2', '-4 10.0.0.0/8 --overlaps=::/0', ''],
	['EqualsBadInputAddress', '2', '1.2.3 --equals=1.2.3.0/24', ''],
	['Equals-Overlaps', '2', '10.0.0.0/8 --equals=10.0.0.0/8 --overlaps=10.0.0.0/8', ''],
	['Equals-Equals', '2', '10.0.0.0/8 --equals=10.0.0.0/8 --equals=10.0.0.0/8', ''],
	['Equals-Info', '2', '--equals=10.0.0.0/8 -n 10.0.0.0/8', ''],
	['Check-Equals', '2', '-c 10.0.0.0/8 --equals=10.0.0.0/8', ''],
	['Equals-IPv4-IPv6', '2', '-4 -6 10.0.0.0/8 --equals=10.0.0.0/8', ''],
	['Overlaps-UnknownOption', '2', '10.0.0.0/8 --overlaps=10.0.0.0/8 --bogus', ''],
	['UnknownOption-Overlaps', '2', '--bogus 10.0.0.0/8 --overlaps=10.0.0.0/8', ''],
	['EqualsMissingArgument', '2', '10.0.0.0/8 --equals', ''],
	['SubnetOfNoAddress', '2', '--subnet-of=10.0.0.0/8', ''],
]
	cmd = ipcalc.full_path() + ' ' + c[2]
	args = ['--test-status', c[1], cmd]
	if c[1] != '2'
		args += ['--test-output', cmd, c[3]]
	endif
	test(c[0], testrunner, args : args)
endforeach

# Without a comparison option, errors keep exit status 1
test('UnknownOptionStatus',
	testrunner,
	args : [
		'--test-status', '1',
		ipcalc.full_path() + ' --bogus 1.2.3.4'
	]
)

# --cidr: [name, arguments, expected output]
foreach c : [
	['CidrIPv4', '--cidr 192.168.123.7/24', 'CIDR=192.168.123.0/24'],
	['CidrNoDecorateIPv6', '--cidr --no-decorate 2001:db8::1/64', '2001:db8::/64'],
	['CidrSingleHost', '--cidr 192.168.1.1', 'CIDR=192.168.1.1/32'],
	['CidrSingleHostIPv6', '--cidr ::1', 'CIDR=::1/128'],
	['CidrFieldOrder', '-n --cidr -p 10.1.2.3/8', 'NETWORK=10.0.0.0\nPREFIX=8\nCIDR=10.0.0.0/8'],
]
	cmd = ipcalc.full_path() + ' ' + c[1]
	test(c[0], testrunner,
		args : ['--test-status', '0', cmd, '--test-output', cmd, c[2]])
endforeach
test('RandomCidr', testrunner,
	args : ['--test-success', ipcalc.full_path() + ' -r 24 --cidr --no-decorate'])

# --wildcard: [name, arguments, expected output]
foreach c : [
	['WildcardIPv4', '--wildcard 192.168.123.7/24', 'WILDCARD=0.0.0.255'],
	['WildcardSingleHost', '--wildcard 192.168.1.1', 'WILDCARD=0.0.0.0'],
	['WildcardPrefix0', '--wildcard 0.0.0.0/0', 'WILDCARD=255.255.255.255'],
	['WildcardPrefix1', '--wildcard 10.0.0.1/1', 'WILDCARD=127.255.255.255'],
	['WildcardPrefix8', '--wildcard 10.0.0.1/8', 'WILDCARD=0.255.255.255'],
	['WildcardPrefix12', '--wildcard 172.16.5.4/12', 'WILDCARD=0.15.255.255'],
	['WildcardPrefix16', '--wildcard 172.16.5.4/16', 'WILDCARD=0.0.255.255'],
	['WildcardPrefix25', '--wildcard 192.168.1.130/25', 'WILDCARD=0.0.0.127'],
	['WildcardPrefix27', '--wildcard 192.168.1.130/27', 'WILDCARD=0.0.0.31'],
	['WildcardPrefix31', '--wildcard 192.168.1.130/31', 'WILDCARD=0.0.0.1'],
	['WildcardNetmask', '--wildcard 10.1.2.3/255.255.255.252', 'WILDCARD=0.0.0.3'],
	['WildcardNoDecorate', '--wildcard --no-decorate 172.16.5.4/12', '0.15.255.255'],
	['WildcardIPv6', '--wildcard 2001:db8::1/64', ''],
	['WildcardPrefix0IPv6', '--wildcard ::/0', ''],
	['WildcardFieldOrder', '--wildcard -m -n 10.1.2.3/255.255.240.0', 'NETWORK=10.1.0.0\nNETMASK=255.255.240.0\nWILDCARD=0.0.15.255'],
]
	cmd = ipcalc.full_path() + ' ' + c[1]
	test(c[0], testrunner,
		args : ['--test-status', '0', cmd, '--test-output', cmd, c[2]])
endforeach
test('InfoWildcard', testrunner,
	args : ['--test-outfile', ipcalc.full_path() + ' -i --wildcard 192.168.2.7/24',
		files('info-wildcard-192.168.2.7-24')])
test('JsonWildcard', testrunner,
	args : ['--test-outfile', ipcalc.full_path() + ' -j --wildcard 192.168.2.7/24',
		files('json-wildcard-192.168.2.7-24')])
test('InfoWildcardIPv6', testrunner,
	args : ['--test-outfile', ipcalc.full_path() + ' -i --wildcard 2001:db8::/32',
		files('info-wildcard-2001:db8::-32')])
test('RandomWildcard', testrunner,
	args : ['--test-success', ipcalc.full_path() + ' -r 24 --wildcard --no-decorate'])

# one field order for the summary and NAME=value output
foreach c : [
	['FieldOrderNetworkNetmask', '-m -n 10.1.2.3/8', 'NETWORK=10.0.0.0\nNETMASK=255.0.0.0'],
	['AddressAndLookupHost', '-a -o localhost -4', 'ADDRESS=127.0.0.1'],
	['HostnameHostPrefix', '-h 127.0.0.1/32', 'HOSTNAME=localhost'],
]
	cmd = ipcalc.full_path() + ' ' + c[1]
	test(c[0], testrunner,
		args : ['--test-status', '0', cmd, '--test-output', cmd, c[2]])
endforeach
test('InfoReverseDNS', testrunner,
	args : ['--test-outfile', ipcalc.full_path() + ' -i --reverse-dns 192.168.2.7/24',
		files('info-reverse-dns-192.168.2.7-24')])
test('JsonReverseDNS', testrunner,
	args : ['--test-outfile', ipcalc.full_path() + ' -j --reverse-dns 192.168.2.7/24',
		files('json-reverse-dns-192.168.2.7-24')])

# -h rejects a network: [name, arguments, expected output]
foreach c : [
	['HostnameNetwork', '-h 127.0.0.1/8', 'ipcalc: -h requires a single address, not a network: 127.0.0.1/8'],
	['HostnameNetworkJson', '-j -h 127.0.0.1/8', 'ipcalc: -h requires a single address, not a network: 127.0.0.1/8'],
	['HostnameNetworkIPv6', '-h ::1/64', 'ipcalc: -h requires a single address, not a network: ::1/64'],
	['HostnameNetworkSilent', '-s -h 127.0.0.1/8', ''],
	['HostnameNetworkFormatJson', '--format=json -h 127.0.0.1/8', 'ipcalc: -h requires a single address, not a network: 127.0.0.1/8'],
]
	cmd = ipcalc.full_path() + ' ' + c[1]
	test(c[0], testrunner,
		args : ['--test-status', '1', cmd, '--test-output', cmd, c[2]])
endforeach

# -i stays within 15 lines of at most 80 columns: [name, input]
foreach c : [
	['InfoSummarySizeIPv6', 'ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff/1'],
	['InfoSummarySizeIPv4', '255.255.255.255/1'],
]
	test(c[0], testrunner,
		args : ['--test-success',
			'out=$(' + ipcalc.full_path() + ' -i ' + c[1] + ') && [ "$(echo "$out" | wc -l)" -le 15 ] && ! echo "$out" | expand | grep -q \'.\{81\}\''])
endforeach

# --all-info has every key the field-selecting options add, except
# -h and -o, which need DNS, and -g, which needs a geo database
all_fields = ' -j -a -b -m -n -p --cidr --wildcard --reverse-dns --addrspace --minaddr --maxaddr --addresses '
foreach c : [
	['AllInfoJsonKeysIPv4', '192.168.2.7/24'],
	['AllInfoJsonKeysIPv6', 'fe80::21b:21ff:fe3a:5c7d/64'],
]
	test(c[0], testrunner,
		args : ['--test-success',
			'all=$(' + ipcalc.full_path() + ' -j --all-info ' + c[1] + ' | grep -o \'"[A-Z0-9]*":\') && ' +
			'sel=$(' + ipcalc.full_path() + all_fields + c[1] + ' | grep -o \'"[A-Z0-9]*":\') && ' +
			'for k in $sel; do echo "$all" | grep -qxF "$k" || { echo "missing $k"; exit 1; }; done'])
endforeach

# --format selects the output format: [name, arguments, expected output]
foreach c : [
	['FormatJsonFields', '--format=json -m -n 192.168.2.7/24', '{\n  "NETWORK":"192.168.2.0",\n  "NETMASK":"255.255.255.0"\n}'],
	['FormatHumanFields', '--format=human -m -n 192.168.2.7/24', 'Network:\t192.168.2.0/24\nNetmask:\t255.255.255.0 = 24'],
	['FormatJsonReverseDNS', '--format=json --reverse-dns 2001:db8::/31', '{\n  "REVERSEDNS":[\n    "8.b.d.0.1.0.0.2.ip6.arpa.",\n    "9.b.d.0.1.0.0.2.ip6.arpa."]\n}'],
	['FormatJsonHostname', '--format=json -h 127.0.0.1', '{\n  "HOSTNAME":"localhost"\n}'],
	['FormatJsonLookupHost', '--format=json -4 -o localhost', '{\n  "ADDRESS":"127.0.0.1"\n}'],
	['FormatJsonWithJ', '-j --format=json -n 10.1.2.3/8', '{\n  "NETWORK":"10.0.0.0"\n}'],
	['FormatValueWithNoDecorate', '--no-decorate --format=value -n 10.1.2.3/8', '10.0.0.0'],
	['FormatHumanEquals', '--format=human 192.168.7.15/24 --equals=192.168.7.5/24', 'The networks are equal'],
	['FormatCheck', '-c --format=shell 127.0.0.1', ''],
]
	cmd = ipcalc.full_path() + ' ' + c[1]
	test(c[0], testrunner,
		args : ['--test-status', '0', cmd, '--test-output', cmd, c[2]])
endforeach

# --format with the output of existing tests: [name, arguments, file]
foreach c : [
	['FormatHumanSummary', '--format=human 192.168.2.0/24', 'standard-192.168.2.0-24'],
	['FormatJsonAllInfo', '--format=json --all-info 192.168.2.7/24', 'json-all-info-192.168.2.7-24'],
	['FormatShellSummary', '--format=shell 192.168.2.7/24', 'shell-192.168.2.7-24'],
	['FormatJsonInfoLookupHost', '--format=json -i -4 -o localhost', 'hostname-localhost-ipv4-json'],
	['FormatValueSplit', '--format=value -S 26 192.168.5.45/24', 'nsplit-192.168.5.45-24-26'],
	['FormatJsonSplit', '--format=json -S 26 192.168.5.45/24', 'json-split-192.168.5.45-24-26'],
	['FormatHumanSplit', '--format=human -S 26 192.168.5.45/24', 'split-192.168.5.45-24-26'],
	['FormatValueDeaggregate', '--format=value -d 192.168.2.7-192.168.2.13', 'deaggregate-192.168.2.7-192.168.2.13'],
]
	test(c[0], testrunner,
		args : ['--test-outfile', ipcalc.full_path() + ' ' + c[1], files(c[2])])
endforeach

# --format prints what the legacy options print: [name, arguments, legacy arguments]
foreach c : [
	['FormatShellFields', '--format=shell -m -n 10.1.2.3/8', '-m -n 10.1.2.3/8'],
	['FormatValueFields', '--format=value -m -n 10.1.2.3/8', '--no-decorate -m -n 10.1.2.3/8'],
]
	test(c[0], testrunner,
		args : ['--test-equal', ipcalc.full_path() + ' ' + c[1], ipcalc.full_path() + ' ' + c[2]])
endforeach

# --format rejects ambiguous combinations: [name, status, arguments, expected output]
foreach c : [
	['FormatUnknown', '1', '--format=xml 10.0.0.1', 'ipcalc: unknown output format: xml; use human, shell, json or value'],
	['FormatUnknownSilent', '1', '-s --format=xml 10.0.0.1', ''],
	['FormatTwice', '1', '--format=json --format=shell 10.0.0.1', 'ipcalc: conflicting output formats: --format=json and --format=shell'],
	['FormatConflictJson', '1', '-j --format=shell 10.0.0.1', 'ipcalc: conflicting output formats: -j and --format=shell'],
	['FormatConflictNoDecorate', '1', '--no-decorate --format=json -m 10.0.0.1', 'ipcalc: conflicting output formats: --no-decorate and --format=json'],
	['FormatInfoWithField', '1', '--format=json -i -m 10.0.0.1', 'ipcalc: -i and --all-info cannot be combined with options that select fields'],
	['FormatAllInfoWithField', '1', '--format=human --all-info -n 10.0.0.1/24', 'ipcalc: -i and --all-info cannot be combined with options that select fields'],
	['FormatValueSummary', '1', '--format=value 10.0.0.1/24', 'ipcalc: --format=value needs options that select fields; use --format=shell or --format=json for the summary'],
	['FormatShellSplit', '1', '--format=shell -S 26 10.0.0.0/24', 'ipcalc: --format=shell is not supported with --split, --split-hosts or --deaggregate; use --format=value'],
	['FormatShellDeaggregate', '1', '--format=shell -d 10.0.0.1-10.0.0.9', 'ipcalc: --format=shell is not supported with --split, --split-hosts or --deaggregate; use --format=value'],
	['FormatJsonEquals', '2', '--format=json 10.0.0.0/8 --equals=10.0.0.0/8', 'ipcalc: --format=json is not supported with --equals, --subnet-of or --overlaps'],
	['FormatValueSubnetOf', '2', '--format=value 10.0.0.0/8 --subnet-of=10.0.0.0/7', 'ipcalc: --format=value is not supported with --equals, --subnet-of or --overlaps'],
	['FormatShellOverlaps', '2', '--format=shell 10.0.0.0/8 --overlaps=10.0.0.0/7', 'ipcalc: --format=shell is not supported with --equals, --subnet-of or --overlaps'],
]
	cmd = ipcalc.full_path() + ' ' + c[2]
	test(c[0], testrunner,
		args : ['--test-status', c[1], cmd, '--test-output', cmd, c[3]])
endforeach
