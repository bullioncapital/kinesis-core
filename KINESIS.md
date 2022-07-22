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

## Run Tests

Use the following command to run test inside docker built in previous section.

```bash
export TAG=kinesis-core:local
docker run --rm -it --entrypoint bash -v $PWD/_output/:/output $TAG
# in the container
./runTests.sh testReport.xml
```

You can also execute `./runTests.sh` from VSCode devcontainer, BUT make sure you first build the source code using `make -j $(nproc)`.
