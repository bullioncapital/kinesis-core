#/bin/sh
XML_TESTS_REPORT=${1:-test-result.xml}

#--------Verify if cluster is running and create DB----------- 
./KINESIS_ci-build.sh

# -------Run tests------
cd src
./stellar-core test -r junit -o $XML_TESTS_REPORT