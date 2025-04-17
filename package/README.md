# To make a deb package:

``` sh
# go to the project root
cd ./gim
# copy the package files to the project root
cp -r ./package/deb ./debian
# update the change log
debchange --newversion=$(./dkms/get-version)
# build the package,
dpkg-buildpackage -tc
```

This should create a package at the parent directory, i.e. in the same directory as gim.

# To make a rpm package:

``` sh
# go to the project root and run
cd ./gim
rpmbuild -bb --build-in-place ./package/rpm/gim-dkms.spec
```

This should create the package in ~/rpmbuild/RPMS/noarch/
