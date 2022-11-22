# Development Environment Setup

- Download & install vscode
    ```bash
    'snap install code' (Linux)
    ```
- Open the kinesis-core project in a remote container via VSCode
    `https://code.visualstudio.com/docs/devcontainers/create-dev-container#_create-a-devcontainerjson-file`
    
- Install vscode extensions inside the remote container once it is opened, install the following extensions: (1) C++ Extension Pack (2) CodeLLDB

## Configure you environment

If it is your first time checking out this repository then, before doing anything, you need to run `vscode-configure.sh` in the remote container terminal within VSCode.

## Debugging

1. Start stepping through the unit test identify section in project root->test folder
2. Update the `.vscode/launch.json` file with accurate values if necessary. ie: compiler and debugger paths.
3. Set breakpoints
4. Hit `F5`

## Building a Docker Image

Use the following command to build a local docker image with debug symbol and test suites:

```bash
export TAG=kinesis-core:local
docker build --build-arg NPROC=$(nproc) -t $TAG . -f docker/Dockerfile.kinesis
```

Building for release:
```bash
export TAG=kinesis-core:local
docker build --build-arg NPROC=$(nproc) --build-arg BUILD=release -t $TAG . -f docker/Dockerfile.kinesis
```

## Run Tests

Use the following command to run tests inside docker as built in the previous section.

```bash
export TAG=kinesis-core:local
docker build  --build-arg NPROC=$(nproc) -t $TAG . -f docker/Dockerfile.kinesis --target buildstage
docker run --rm -it --entrypoint bash -v $PWD/_output/:/output $TAG
# in the container
./runTests.sh testReport.xml
```

You can also execute `./runTests.sh` from VSCode devcontainer, BUT make sure you first build the source code using `make -j $(nproc)`.
