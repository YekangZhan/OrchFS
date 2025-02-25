#!/bin/bash

sudo -v

libtoolize
aclocal
autoheader
automake --add-missing
autoconf

./configure
make
sudo make install

echo "----------------------------------------------------------------"
echo "Checking"

which filebench
ret=$?

if [ $ret -eq 0 ]
then
    echo "Filebench installed successfully!"
    echo "----------------------------------------------------------------"
    exit 0

else
    echo "Filebench *not* installed"
    echo "----------------------------------------------------------------"
    exit 1
fi

