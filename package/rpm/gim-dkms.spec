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
	cp -r gim gim-coms-lib libgv smi-lib %{buildroot}/usr/src/gim-%{full_version}/
sed 's/#MODULE_VERSION#/%{full_version}/g' ./package/rpm/dkms.conf > %{buildroot}/usr/src/gim-%{full_version}/dkms.conf
echo %{full_version} > %{buildroot}/usr/src/gim-%{full_version}/gim/dkms/VERSION

%post
dkms add -m gim/%{full_version} --rpm_safe_upgrade
dkms build -m gim/%{full_version}
dkms install -m gim/%{full_version} --force

%preun
# Remove all versions from DKMS registry:
dkms remove -m gim/%{full_version} --all --rpm_safe_upgrade

%files
%defattr(-,root,root)
/usr/src/gim-%{full_version}

%changelog
* Mon Mar 17 2025 AMD <gim-maintainer@amd.com>
- Placeholder
