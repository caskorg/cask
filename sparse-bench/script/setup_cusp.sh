#!/usr/bin/env bash
rootDir=`pwd`
version=0.5.1
libDir=src/gpu/lib
workDir=setup

mkdir -p ${workDir}
cd ${workDir}
wget https://github.com/cusplibrary/cusplibrary/archive/v${version}.zip
unzip v${version}.zip

cd ${rootDir}
mkdir -p ${libDir}
rm -rf ${libDir}/cusp
mv ${workDir}/cusplibrary-${version}/cusp ${libDir}
rm -rf ${workDir}
