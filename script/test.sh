#!/usr/bin/env bash
txtblk='\e[0;30m' # Black - Regular
txtred='\e[0;31m' # Red
txtgrn='\e[0;32m' # Green
txtylw='\e[0;33m' # Yellow
txtblu='\e[0;34m' # Blue
txtpur='\e[0;35m' # Purple
txtcyn='\e[0;36m' # Cyan
txtwht='\e[0;37m' # White
txtrst='\e[0m'    # Text Reset

rootDir=$(readlink -f .)
binaryDir=${rootDir}/build

echo "Running test => ${rootDir}"
echo "Binary dir   => ${binaryDir}"

binaries="CgEigen BicgStabEigen" # CgMklExplicit CgMklRci CgMklThreeTerm"

matrices="test/systems/tiny test/systems/tinysym"

for b in ${binaries}; do
  fullBinaryPath=${binaryDir}/$b
  echo -e "${txtpur}Testing ${fullBinaryPath}${txtrst}\n"

  for m in ${matrices}; do
    echo -e "${txtcyn}Running on ${m}${txtrst}\n"
    matrix=$(readlink -f ${m}.mtx)
    lhs=$(readlink -f ${m}_sol.mtx)
    rhs=$(readlink -f ${m}_b.mtx)
    ${fullBinaryPath} -mat ${matrix}  -rhs ${rhs}  -lhs ${lhs}
  done

done
