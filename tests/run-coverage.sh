#! /bin/sh

cat public/index.html|grep headerCovTableEntry|grep "%"|head -1|sed 's/^.*>\([0-9\.\ %]*\)<.*$/coverage lines: \1/'

