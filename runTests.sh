#/bin/sh
#$1= path test report is expected

#--------Verify if cluster is running and create DB----------- 
./KINESIS_ci-build.sh

# -------Run tests------
cd src
./stellar-core test -r junit -o $1
return