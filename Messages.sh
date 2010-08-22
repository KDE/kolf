#! /usr/bin/env bash
$EXTRACTRC *.rc *.ui >> rc.cpp
$XGETTEXT `find . -name \*.h -o -name \*.cpp` -o $podir/kolf.pot
