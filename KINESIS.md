# Setup Development Environment

- Download & install vscode
- Open code in remote container
- Install vscode extensions inside remote container: (1) C++ Extension Pack (2) CodeLLDB

## Configure you environment

If it is your first time checking out code you need to run `vscode-configure.sh` in remote container terminal.

## Debugging

1. Start from unit test identify section
2. Update `.vscode/launch.json`
3. Set breakpoints
4. Hit `F5`

## Build Docker Image

Use the following command to build a local docker image:

```bash
export TAG=kinesis-core:local
docker build -t $TAG . -f docker/Dockerfile.kinesis
```


## Run Tests inside dev container 

The PostgreSQL cluster should be running inside the dev container for the tests to establish the database connections 

Use the following command to check if main cluster is running.
```bash
pg_lsclusters 
```
Start the main cluster if not running already
```bash
sudo pg_ctlcluster 12 main start 
```
Run script file to create test db. 
```bash
./KINESIS_ci-build.sh
```
check if db is accepting connections
```bash
pg_isready 
```

Run stellar-core tests and generate report 'testReport'
```bash
./stellar-core test -r junit -o testReport.xml
```

