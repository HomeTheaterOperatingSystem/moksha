#!/bin/sh

rm -rf autom4te.cache
rm -f aclocal.m4 ltmain.sh

touch README
touch ABOUT-NLS

# no more autopoint. cuases too many hassles. too bad gentoo users - go back
# to automake 1.9 :(
#echo "Running autopoint..." ; autopoint -f || exit 1
echo "Running aclocal..." ; aclocal $ACLOCAL_FLAGS -I m4 || exit 1
echo "Running autoconf..." ; autoconf || exit 1
echo "Running autoheader..." ; autoheader || exit 1
echo "Running libtoolize..." ; (libtoolize --copy --automake || glibtoolize --automake) || exit 1
#echo "Running gettextize..." ; gettextize -f --no-changelog&
# hack - gettextize is interactive and demands input from a user. "screw it".
#sleep 20
#kill %1
echo "Running automake..." ; automake --add-missing --copy --gnu || exit 1

if [ -z "$NOCONFIGURE" ]; then
	./configure "$@"
fi
