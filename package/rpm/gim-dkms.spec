# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

%define full_version %(./gim/dkms/get-version)
%define version %(echo %{full_version}-0 | cut -sd- -f1)
%define release %(echo %{full_version}-0 | cut -sd- -f2)

Name: gim-dkms
Version: %{version}
Release: %{release}
Summary: DKMS source for GIM driver.

License: MIT
URL: https://github.com/amd/mxgpu-virtualization
Source0: ./gim-%{full_version}.tar.gz
BuildArch: noarch
Requires: autoconf cmake dkms gcc-c++ kernel-devel

%description
DKMS source for GIM driver.

%global debug_package %{nil}

%prep
%autosetup -c -n gim-%{full_version}

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/src/gim-%{full_version}
cp -r gim gim-coms-lib libgv smi-lib uniras %{buildroot}/usr/src/gim-%{full_version}/
sed 's/#MODULE_VERSION#/%{full_version}/g' ./package/rpm/dkms.conf > %{buildroot}/usr/src/gim-%{full_version}/dkms.conf
echo %{full_version} > %{buildroot}/usr/src/gim-%{full_version}/gim/dkms/VERSION

%post
dkms add -m gim/%{full_version} --rpm_safe_upgrade
dkms build -m gim/%{full_version}
dkms install -m gim/%{full_version} --force

%posttrans
# Runs after the old package's %postun on upgrade. install-smi.sh builds the
# replacement, removes the previous userspace AMD SMI, and installs the new
# version. An older package's DKMS POST_REMOVE runs in %preun, so this cannot
# be done from %post.
sh /usr/src/gim-%{full_version}/gim/dkms/install-smi.sh

%preun
# Remove all versions from DKMS registry:
dkms remove -m gim/%{full_version} --all --rpm_safe_upgrade

%postun
# Remove userspace AMD SMI only when the package is erased ($1 == 0).
# On upgrade, the new %posttrans replaces it through install-smi.sh.
if [ "$1" -eq 0 ]; then
    if [ -f /var/lib/gim/smi-manifest ]; then
        xargs -r rm -f < /var/lib/gim/smi-manifest
        rm -f /var/lib/gim/smi-manifest
        rmdir /var/lib/gim 2>/dev/null || true
    fi
    ldconfig || true
fi

%files
%defattr(-,root,root)
/usr/src/gim-%{full_version}

%changelog
* Mon Mar 17 2025 AMD <gim-maintainer@amd.com>
- Placeholder
