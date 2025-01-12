#! /usr/bin/env bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2024 Luigi Toscano <luigi.toscano@tiscali.it>
#
# extract in a temporarily dummy cpp file
$EXTRACTRC `find . -name \*.kcfg` >>rc.cpp
# main code files
$XGETTEXT `find . -name \*.cpp -o -name \*.h -o -name \*.qml` -o $podir/karp.pot
# cleanup
rm -f rc.cpp
