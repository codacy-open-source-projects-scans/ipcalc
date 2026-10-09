#! /bin/sh

set -e

entries=$(grep headerCovTableEntry public/index.html)
echo "$entries"|grep "%"|head -1|sed 's/&nbsp;//g'|sed 's/^.*>\([0-9\.\ %]*\)<.*$/coverage lines: \1/'
