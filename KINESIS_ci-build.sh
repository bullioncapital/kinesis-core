# /bin/sh

# run 'pg_lsclusters' to check if main cluster is running.
# if not then run 'sudo pg_ctlcluster 12 main start' and check using 'pg_isready' if the service is accepting connections


NPROCS=$(getconf _NPROCESSORS_ONLN)

echo "Found $NPROCS processors"

# Create postgres databases
export PGUSER=postgres
psql -c "create database test;"
# we run NPROCS jobs in parallel
for j in $(seq 0 $((NPROCS-1))); do
    base_instance=$((j*50))
    for i in $(seq $base_instance $((base_instance+15))); do
        psql -c "create database test$i;"
    done
done