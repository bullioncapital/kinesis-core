# Development Environment Setup

The Kinesis changes of the fork are enabled using _KINESIS C++ macro and makefile flag in the fork of stellar-core.

## 1. With Dev Container

### a. Prerequisites
- [Download & install vscode](https://code.visualstudio.com/download)
- Open the kinesis-core project in vscode and click on "Reopen in Container" when asked. 
![image](https://user-images.githubusercontent.com/29750/203445568-939211f6-126f-4150-8b7e-d2b3360effff.png)

`Troubleshooting note` : If the devcontainer is not booting up with package errors, try using a different mirror to download the packages : Run `wget -qO - mirrors.ubuntu.com/mirrors.txt` on terminal, pick one of the mirrors and update `APT_MIRROR` build argument in [devcontainer.json](.devcontainer/devcontainer.json) with your mirror and then rebuild container.

- Install vscode extensions inside the remote container once it is opened, install the following extensions if not installed on their own:
    * [C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
    * [CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)

### b. Configure you environment

If it is your first time checking out this repository then, before doing anything, you need to run `vscode-configure.sh` in the remote container terminal within VSCode.

### c. Debugging

1. Start stepping through the unit test identify section in project root->test folder
2. Update the `.vscode/launch.json` file with accurate values if necessary. ie: compiler and debugger paths.
3. Set breakpoints
4. Hit `F5`

## 2. Without dev container 
Follow steps from [INSTALL.md](INSTALL.md) for steps specific to your OS


## Run Tests

### 1. Using Docker 
Use the following command to run tests inside docker 

```bash
export TAG=kinesis-core:local
docker build  --build-arg NPROC=$(nproc) -t $TAG . -f docker/Dockerfile.kinesis --target buildstage
docker run --rm -it --entrypoint bash -v $PWD/_output/:/output $TAG
# in the container
./runTests.sh /output/testReport.xml
```
Note: The default location of test report is src folder of the container itself, unless the output path is provided as argument.

### 2. Using Devcontainer
Inside the dev container terminal,
```bash 
#build the source code 
make -j $(nproc)

# run tests
./runTests.sh
```
Note: The default name of the report is `test-result.xml` and location of test report is src folder, unless a path with name is provided as argument.

To generate the html from the report, just run following inside devcontainer
```bash
#XML_TESTS_REPORT = filename of the report (default filename = test-result)
xunit-viewer -r <XML_TESTS_REPORT>.xml -o <XML_TESTS_REPORT>.html
```
The html report provides the passed and failed catch2 unit test cases.

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
