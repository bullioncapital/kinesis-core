# /bin/sh

# Create Postgres DB
CreateDb() 
{
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
}

# Run Postgres main cluster
RunCluster() 
{
  state=$(pg_lsclusters | cut -d" " -f8)
  if [ $state = "down" ]
  then
    echo `pg_ctlcluster 12 main start`
    echo "Cluster is `pg_lsclusters | cut -d" " -f8`"
  else
    echo "Clusters are already online"
  fi
}

#Verify DB and Create Db 
CheckStartDb()
{
  local RETRY=0
  while ($(pg_isready) != "accepting")
  do
    if (( $RETRY -gt 5 )); then
      echo "PG server connection refused"
      exit 1 # fail fast 
    fi
    echo "PG is not accepting connection .. retry ${RETRY}"
    sleep 1s
    ((RETRY=RETRY + 1))
  done
}

###############################

echo "user=`whoami`" 
RunCluster
CheckStartDb
