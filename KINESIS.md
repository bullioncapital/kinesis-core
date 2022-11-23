# Development Environment Setup

- [Download & install vscode](https://code.visualstudio.com/download)
- Open the kinesis-core project in vscode and click on "Reopen in Container" when asked
![image](https://user-images.githubusercontent.com/29750/203445568-939211f6-126f-4150-8b7e-d2b3360effff.png)

- Install vscode extensions inside the remote container once it is opened, install the following extensions:
    * [C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
    * [CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)

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
