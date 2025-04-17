%define full_version %(./dkms/get-version)
%define version %(./dkms/get-version | cut -d- -f1)
%define release %(./dkms/get-version | cut -d- -f2)

Name: gim-dkms
Version: %{version}
Release: %{release}
Summary: DKMS source for GIM driver.

License: MIT
URL: https://github.com/amd/mxgpu-virtualization
Source0: ./gim-%{full_version}.tar.gz
BuildArch: noarch
Requires: dkms kernel-devel

%description
DKMS source for GIM driver.

%global debug_package %{nil}

%prep
%autosetup -c -n gim-%{full_version}

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/src/gim-%{full_version}
cp -r Makefile dkms gim-coms-lib gim_shim libgv smi-lib %{buildroot}/usr/src/gim-%{full_version}/
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
