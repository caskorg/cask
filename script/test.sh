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

binaries="CgMklExplicit" # CgMklRci CgEigen BicgStabEigen" # CgMklThreeTerm"

matrices="test/systems/tiny test/systems/tinysym"
matrices=$(find test/all-systems -name "*_SPD.mtx" | sed 's/.mtx//g' | xargs -If basename f)

matDir="test/systems"
solDir="test/systems"

if [ -e test/all-systems ]; then
  matDir="test/all-systems"
fi

if [ -e test/solutions ]; then
  solDir="test/solutions"
fi

jsonOutput="[\n"
first=true

for b in ${binaries}; do
  fullBinaryPath=${binaryDir}/$b
  echo -e "${txtpur}==> Testing ${fullBinaryPath}${txtrst}"

  for m in ${matrices}; do
    echo -e "${txtcyn}---> Running on ${m}${txtrst}"
    matrix=$(readlink -f ${matDir}/${m}.mtx)
    rhs=$(readlink -f ${matDir}/${m}_b.mtx)
    lhs=$(readlink -f ${solDir}/${m}_x.mtx)

    if [ "$first" != true ]; then
      jsonOutput+=","
    fi

    first=false
    jsonOutput+="{\n"
    jsonOutput+="\"command\":\"${fullBinaryPath} -mat ${matrix}  -rhs ${rhs}  -lhs ${lhs})\",\n"
    jsonOutput+="\"matrix\":\"${m}\",\n"
    jsonOutput+="\"solver\":\"${b}\",\n"
    jsonOutput+=$(${fullBinaryPath} -mat ${matrix}  -rhs ${rhs}  -lhs ${lhs}) # | sed 's/^/     /'
    jsonOutput+="}\n"

  done
done

jsonOutput+="]\n"

echo -e ${jsonOutput} > out.json
