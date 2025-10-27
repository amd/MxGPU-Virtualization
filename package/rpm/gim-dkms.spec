# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

%define full_version %(./dkms/get-version)
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
mkdir -p %{buildroot}/usr/src/gim-%{full_version}/
mkdir -p %{buildroot}/usr/src/gim-%{full_version}/gim-coms-lib
mkdir -p %{buildroot}/usr/src/gim-%{full_version}/smi-lib
mkdir -p %{buildroot}/usr/src/gim-%{full_version}/libgv
cp -r Makefile dkms gim_shim %{buildroot}/usr/src/gim-%{full_version}/
cp -r gim-coms-lib/* %{buildroot}/usr/src/gim-%{full_version}/gim-coms-lib/
cp -r smi-lib/* %{buildroot}/usr/src/gim-%{full_version}/smi-lib/
cp -r libgv/core libgv/inc libgv/Makefile %{buildroot}/usr/src/gim-%{full_version}/libgv/
sed 's/#MODULE_VERSION#/%{full_version}/g' ./package/rpm/dkms.conf > %{buildroot}/usr/src/gim-%{full_version}/dkms.conf
echo %{full_version} > %{buildroot}/usr/src/gim-%{full_version}/VERSION

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
* Mon Mar 17 2025 AMD <sriov@amd.com>
- Placeholder
