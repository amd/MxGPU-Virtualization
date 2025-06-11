Start at the project root

``` sh
cd ./gim
```

If you are not using git, or doesn't have the proper git tags, you can set the version with

``` sh
echo 1.2.3.K > ./VERSION
```

# To make a deb package:

the depends need install:
    ubuntu24: debhelper-compat dh-dkms
    ubuntu22: debhelper-compat


the package can be built with

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

Simply run

``` sh
rpmbuild -bb --build-in-place ./package/rpm/gim-dkms.spec
```

This should create the package in ~/rpmbuild/RPMS/noarch/
