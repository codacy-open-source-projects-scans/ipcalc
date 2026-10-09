#!/bin/bash

echo "This script will make a new release and create tags at gitlab"
echo "It will use your ssh keys, gpg key, glab, and gitlab token as placed in ~/.gitlab-token"
echo "Press enter to continue..."
read

if test -z "$1";then
	echo "usage: $0 [VERSION]"
	echo "No version was specified"
	exit 1
fi

if ! test -f ~/.gitlab-token;then
	echo "Cannot find ~/.gitlab-token"
	exit 1
fi

PROJECT=7517683
REPO=ipcalc/ipcalc
TOKEN=$(cat ~/.gitlab-token)
version=$1

meson_version=$(sed -n "s/^[[:space:]]*version[[:space:]]*:[[:space:]]*'\([^']*\)'.*/\1/p" meson.build|head -1)
if test "$meson_version" != "$version";then
	echo "Version in meson.build ($meson_version) does not match $version"
	exit 1
fi

echo "Creating tag $version and gitlab release"
echo "Press enter to continue or type skip to skip..."
read s
if test "$s" != "s" && test "$s" != "skip";then
	git tag -s ${version} -m "Released ${version}"
	git push origin ${version}
fi

tarball=ipcalc-${version}.tar.xz

echo "Creating and signing $tarball"
echo "Press enter to continue or type skip to skip..."
read s
if test "$s" != "s" && test "$s" != "skip";then
	if test "$(git rev-parse HEAD)" != "$(git rev-parse ${version}^{commit})";then
		echo "HEAD is not at tag $version; check it out first"
		exit 1
	fi
	set -e
	builddir=$(mktemp -d)
	trap 'rm -rf "$builddir"' EXIT
	meson setup "$builddir"
	meson dist -C "$builddir" --formats xztar
	cp "$builddir/meson-dist/$tarball" .
	gpg --detach-sign "$tarball"
	set +e
fi

echo "Creating gitlab $version release with $tarball and $tarball.sig"
echo "Press enter to continue or type skip to skip..."
read s

if test "$s" != "s" && test "$s" != "skip";then
	line=$(grep -n "Version ${version}" NEWS|cut -d ':' -f 1)
	test -z "$line" && exit 1

	stopline="$(head -n 100 NEWS|tail -n $((100-$line))|grep -n Version|head -1|cut -d ':' -f 1)"
	test -z "$stopline" && exit 1

	set -e
	notes=$(mktemp)
	head -n 100 NEWS|tail -n +$((1+$line))|head -n $(($stopline-1)) >"$notes"

	export GITLAB_TOKEN="$TOKEN"
	milestone_opt=""
	if test "$(glab api "projects/${PROJECT}/milestones?title=${version}&include_ancestors=true")" != "[]";then
		milestone_opt="--milestone=${version}"
	fi

	glab release create "$version" -R "$REPO" --name "$version" --notes-file "$notes" \
	     $milestone_opt --use-package-registry --package-name ipcalc \
	     "$tarball#Tarball#package" "$tarball.sig#PGP signature#other"
	rm -f "$notes"
fi

echo ""
echo Done

exit 0
