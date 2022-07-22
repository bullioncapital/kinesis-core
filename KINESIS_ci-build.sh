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
    for times in 1 2 3 4 5
    do
        ready=$(pg_isready)
        accept=$(echo $ready | cut -d" " -f3)

        if [ $accept = "accepting" ] 
        then
            echo "DB is accepting connections"
            CreateDb
            return
        else
            echo "DB not ready  $ready"
            sleep 1s
        fi
    done

}

###############################

echo "user=`whoami`" 
RunCluster
CheckStartDb