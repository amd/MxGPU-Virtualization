Start at the project root

``` sh
cd ./gim
```

If you are not using git, or doesn't have the proper git tags, you can set the version with

``` sh
echo 1.2.3.K > ./VERSION
```

# To make a deb package:

Install the dependencies, for newer distros like ubuntu 24.04, do:

``` sh
apt install devscripts debhelper-compat dh-dkms
```

For older distros like ubuntu 22.04, do:

``` sh
apt install devscripts debhelper-compat dkms
```

then package can be built with

``` sh
# copy the package files to the project root
cp -r ./package/deb ./debian
# update the change log
debchange --newversion=$(./dkms/get-version)
# build the package,
dpkg-buildpackage -tc
```

This should create a package at the parent directory, i.e. in the same directory as ./gim.

# To make a rpm package:

Install the dependencies:
``` sh
    dnf install rpm-build
```

and run

``` sh
rpmbuild -bb --build-in-place ./package/rpm/gim-dkms.spec
```

This should create the package in ~/rpmbuild/RPMS/noarch/
