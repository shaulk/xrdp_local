Name:           xrdp-local
Version:        0.10.4.1
Release:        1%{?dist}
Summary:        XRDP local client application

License:        Apache 2.0
URL:            https://example.org/xrdp_local
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  argparse-devel
BuildRequires:  xrdp-devel = 1:%{version}

Requires:       qt6-qtbase
Requires:       xrdp = 1:%{version}
Requires:       xorgxrdp-glamor >= 0.10.4

%description
xrdp_local is a client application for XRDP, built with Qt6 and libargparse.

%prep
%setup -q

# Pre-build hook: ensure installed xrdp-devel version matches the package version
%build
# Extract installed xrdp-devel version
INSTALLED_VER=$(rpm -q --qf '%{VERSION}' xrdp-devel 2>/dev/null || echo "none")

if [ "$INSTALLED_VER" != "%{version}" ]; then
	echo "Error: Installed xrdp-devel version ($INSTALLED_VER) does not match the version being built (%{version})."
	exit 1
fi

mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make %{?_smp_mflags} VERBOSE=1

%install
cd build
make install .. DESTDIR=%{buildroot}
cd ..
mkdir -p %{buildroot}/usr/share/doc/%{name}
cp README.md %{buildroot}/usr/share/doc/%{name}/
cp COPYING %{buildroot}/usr/share/doc/%{name}/

%files
/usr/bin/xrdp_local
/usr/share/doc/%{name}/README.md
/usr/share/doc/%{name}/COPYING

%changelog
* Fri Aug 15 2025 Shaul Kremer <shaulk@users.noreply.github.com> - 0.10.4-1
- Initial package
