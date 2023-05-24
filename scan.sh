#!/bin/bash
scan-build-7 -o ./scan-build/ make -j`nproc --all` |& tee ./scan-build-result.txt
exitstatus=$${PIPESTATUS[0]}
echo $$exitstatus > ./scan-build-status.txt

