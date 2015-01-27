#!/bin/sh
#
# This script finds all of the dependecies from Fink and included them
# into current folder so that it becomes a libdir to be installed into
# ~/Library/Pd or /Library/Pd <hans@eds.org>

if [ -z "$1" ]; then
    PD_APP_LIB=`pwd`
else
    PD_APP_LIB=$1
fi

echo " "

for pd_darwin in `find . -name '*.pd_darwin'`; do
    LIBS=`otool -L $pd_darwin | sed -n 's|.*/sw/lib/\(.*\.dylib\).*|\1|p'`
    if [ "x$LIBS" != "x" ]; then
        echo "`echo $pd_darwin | sed 's|.*/\(.*\.pd_darwin$\)|\1|'` is using:"
        for lib in $LIBS; do
            echo "    $lib"
            install -vp /sw/lib/$lib $PD_APP_LIB
            new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
            install_name_tool -id @loader_path/$new_lib $PD_APP_LIB/$new_lib
            install_name_tool -change /sw/lib/$lib @loader_path/$new_lib $pd_darwin
        done
        echo " "
    fi
done

# needs to run 3 times:
# - once to start the process
# - once to catch dylibs that depend on dylibs
# - once again because ... three is a magic number, or something like that
for n in {1..3}; do
    for dylib in $PD_APP_LIB/*.dylib; do
        LIBS=`otool -L $dylib | sed -n 's|.*/sw/lib/\(.*\.dylib\).*|\1|p'`
        if [ "x$LIBS" != "x" ]; then
            echo "`echo $dylib | sed 's|.*/\(.*\.dylib\)|\1|'` is using:"
            for lib in $LIBS; do
                echo "    $lib"
                new_lib=`echo $lib | sed 's|.*/\(.*\.dylib\)|\1|'`
                if [ -e  $PD_APP_LIB/$new_lib ]; then
                    echo "$PD_APP_LIB/$new_lib already exists, skipping copy."
                else
                    install -vp /sw/lib/$lib $PD_APP_LIB
                fi
                install_name_tool -id @loader_path/$new_lib $PD_APP_LIB/$new_lib
                install_name_tool -change /sw/lib/$lib @loader_path/$new_lib $dylib
            done
            echo " "
        fi
    done
done
