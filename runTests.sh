#!/usr/bin/env bash
XML_TESTS_REPORT=${1:-test-result.xml}

#--------Verify if cluster is running and create DB----------- 
./KINESIS_ci-build.sh

# -------Run tests------
run_stellar_tests() {
    set -e
    pushd src
    ./stellar-core test -r junit -o $XML_TESTS_REPORT
    popd
}

run_stellar_tests