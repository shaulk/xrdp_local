#!/bin/bash

set -e
set -x

DISTRO="$1"
ARCH="$2"
DISTRO_NAME="${DISTRO%%-*}"
DISTRO_VERSION="${DISTRO#*-}"

deps_flags=""
if [ -f distros/$DISTRO_NAME/needs-deps-override.$DISTRO_NAME-$DISTRO_VERSION ]
then
	deps_flags="-d"
fi

version=$(git describe --tags | cut -b2-)

mkdir -p output
mkdir -p deps

xrdp_pkg=xrdp_$version-1dmabuf_$ARCH.$DISTRO_NAME-$DISTRO_VERSION.deb
if ! [ -f deps/$xrdp_pkg ]; then
	curl -L https://github.com/shaulk/xrdp_local_deps/releases/download/v$version/$xrdp_pkg -o deps/$xrdp_pkg
fi

docker buildx build \
	--platform linux/$ARCH \
	-t builder-$DISTRO_NAME-$DISTRO_VERSION-$ARCH \
	-f distros/$DISTRO_NAME/Dockerfile.$DISTRO_NAME-$DISTRO_VERSION \
	--load \
	.

docker run \
	-i \
	--platform linux/$ARCH \
	-v $PWD/..:/src \
	-v $PWD/deps:/deps \
	-v $PWD/output:/output \
	builder-$DISTRO_NAME-$DISTRO_VERSION-$ARCH \
	bash -c "
		set -x &&
		DEBIAN_FRONTEND=noninteractive dpkg -i /deps/$xrdp_pkg &&
		mkdir /build &&
		cp -r /src /build/xrdp-local-$version &&
		cd /build &&
		tar -czf xrdp-local_$version.orig.tar.gz xrdp-local-$version &&
		cd xrdp-local-$version &&
		dpkg-buildpackage -us -uc $deps_flags &&
		cd .. &&
		for x in *.*deb;
		do
			cp \"\$x\" \"/output/\$(echo \"\$x\" | sed -r \"s/(\\.d?deb)/.$DISTRO_NAME-$DISTRO_VERSION\\1/\")\" || exit 1
		done
	"
