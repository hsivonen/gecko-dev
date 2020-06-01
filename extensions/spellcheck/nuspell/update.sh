#!/bin/sh

if [ $# -lt 1 ]; then
  echo update.sh "<release tag name>"
  exit 1
fi

nuspell_dir=`dirname $0`

tmpclonedir=$(mktemp -d)
git clone https://github.com/nuspell/nuspell --depth 1 --branch $1 ${tmpclonedir}
# Back up mozilla files
cp ${nuspell_dir}/src/moz.build ${tmpclonedir}/src/nuspell
cp ${nuspell_dir}/src/moz.yaml ${tmpclonedir}/src/nuspell

rm -rf ${nuspell_dir}/src
cp -r ${tmpclonedir}/src/nuspell/ ${nuspell_dir}/src
cp ${tmpclonedir}/COPYING.LESSER ${nuspell_dir}/src
cp ${tmpclonedir}/COPYING ${nuspell_dir}/src
cp ${tmpclonedir}/AUTHORS ${nuspell_dir}/src
cp ${tmpclonedir}/README.md ${nuspell_dir}/src
rm ${nuspell_dir}/src/clang-format.sh
rm ${nuspell_dir}/src/CMakeLists.txt
rm ${nuspell_dir}/src/main.cpp
rm -rf ${tmpclonedir}

cd ${nuspell_dir}/src
