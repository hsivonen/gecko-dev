#!/bin/sh

# Copyright 2020 Sander van Geloven for the Nuspell integration

if [ $# -lt 1 ]; then
  echo update.sh "<release tag name, probably nothrows>"
  exit 1
fi

nuspell_dir=`dirname $0`

tmpclonedir=$(mktemp -d)
git clone https://github.com/nuspell/nuspell --depth 1 --branch $1 ${tmpclonedir}
# Back up mozilla files
cp ${nuspell_dir}/src/moz.build ${tmpclonedir}/src/nuspell/
cp ${nuspell_dir}/src/moz.yaml ${tmpclonedir}/src/nuspell/

rm -rf ${nuspell_dir}/src/
cp -r ${tmpclonedir}/src/nuspell/ ${nuspell_dir}/src/
cp ${tmpclonedir}/COPYING.LESSER ${nuspell_dir}/src/
cp ${tmpclonedir}/COPYING ${nuspell_dir}/src/
cp ${tmpclonedir}/AUTHORS ${nuspell_dir}/src/
cp ${tmpclonedir}/README.md ${nuspell_dir}/src/
rm ${nuspell_dir}/src/clang-format.sh
rm ${nuspell_dir}/src/CMakeLists.txt
rm ${nuspell_dir}/src/main.cxx

rm -rf ${nuspell_dir}/tests/unit/data/*
cp ${tmpclonedir}/tests/v1cmdline/* ${nuspell_dir}/tests/unit/data/
rm -rf ${tmpclonedir}

# Rename test files so that underscores become hyphens as is convention here.
for i in $(ls ${nuspell_dir}/tests/unit/data/*_*); do
  mv -f $i $(echo $i|sed -e 's/_/-/g')
done

cd ${nuspell_dir}/src/
patch -p5 < ../patches/symbolexposure.patch
#patch -p5 < ../patches/encodingconverter.patch
