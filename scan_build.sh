#!/bin/bash
cd /sw/src/modules/mod_com_g729
apt-get update && apt-get install -yq apt-transport-https ruby jq curl openssh-client gnupg2 wget build-essential autotools-dev lsb-release pkg-config automake autoconf libtool-bin clang-tools-7 uncrustify
./check-style.sh
echo "machine fsa.freeswitch.com login $FSA_USERNAME password $FSA_PASSWORD" > /etc/apt/auth.conf
echo "deb [trusted=yes] https://fsa.freeswitch.com/repo/deb/fsa/ $(lsb_release -sc) unstable" > /etc/apt/sources.list.d/freeswitch.list
apt-get update && apt-get install -yq libfreeswitch-dev libcurl4-openssl-dev librdkafka-dev
./bootstrap.sh
PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/2.0/lib/pkgconfig ./configure
mkdir -p scan-build
scan-build-7 -o ./scan-build/ make -j`nproc --all` |& tee ./scan-build-result.txt
exitstatus=${PIPESTATUS[0]}
echo "*** Exit status is $exitstatus";
export SubString="scan-build: No bugs found";
export COMPILATION_FAILED=false;
export BUGS_FOUND=false;
if [ "0" -ne $exitstatus ] ; then
  export COMPILATION_FAILED=true;
  echo MESSAGE="compilation failed" >> $GITHUB_OUTPUT;
fi
export RESULTFILE="/sw/src/modules/mod_com_g729/scan-build-result.txt";
if ! grep -sq "$SubString" $RESULTFILE; then
  export BUGS_FOUND=true;
  echo MESSAGE="found bugs" >> $GITHUB_OUTPUT;
fi
echo "COMPILATION_FAILED=$COMPILATION_FAILED" >> $GITHUB_OUTPUT;
echo "BUGS_FOUND=$BUGS_FOUND" >> $GITHUB_OUTPUT;
if [ "0" -ne $exitstatus ] || ! grep -sq "$SubString" $RESULTFILE; then
  exit 1;
fi
exit 0;