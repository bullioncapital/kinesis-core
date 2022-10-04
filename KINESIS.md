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

Use the following command to build a local docker image with debug symbol and test suites:

```bash
export TAG=kinesis-core:local
docker build --build-arg NPROC=$(nproc) -t $TAG . -f docker/Dockerfile.kinesis
```

Build for release:
```bash
export TAG=kinesis-core:local
docker build --build-arg NPROC=$(nproc) --build-arg BUILD=release -t $TAG . -f docker/Dockerfile.kinesis
```

## Run Tests

Use the following command to run test inside docker built in previous section.

```bash
export TAG=kinesis-core:local
docker build  --build-arg NPROC=$(nproc) -t $TAG . -f docker/Dockerfile.kinesis --target buildstage
docker run --rm -it --entrypoint bash -v $PWD/_output/:/output $TAG
# in the container
./runTests.sh testReport.xml
```

You can also execute `./runTests.sh` from VSCode devcontainer, BUT make sure you first build the source code using `make -j $(nproc)`.

## Running and Updating TxMeta Checks

The units tests can be run into two special modes that has the TxMeta of each transaction executed. These two can increase the confidence that a change to kinesis-core does not alter the semantics of any transaction. The two modes are:

  * `--record-test-tx-meta <dirname>` which records TxMeta hashes into `<dirname>`
  * `--check-test-tx-meta <dirname>` which checks TxMeta hashes against `<dirname>`
  
You can run the `--check-test-tx-meta` mode against a pair of captured baseline directories stored in the repository, called `test-tx-meta-baseline-current` (for the current protocol) and `text-tx-meta-baseline-next` (for the next protocol). 
Use the following command to run the tests with `--check-test-tx-meta` inside the docker container mentioned in previous section:

```bash
# in the container
./stellar-core test [tx] --all-versions --rng-seed 12345 --check-test-tx-meta test-tx-meta-baseline-current -o testReport.xml
#for a build with only the current protocol enabled, and:

./stellar-core test [tx] --all-versions --rng-seed 12345 --check-test-tx-meta test-tx-meta-baseline-next -o testReport.xml
#for a build configured with `--enable-next-protocol-version-unsafe-for-production`.
```

If you make _intentional_ changes to the semantics of any transactions, or add any new transactions that need to have their hashes recorded, you can re-record the baseline using a command like:

```bash
# in the container
./stellar-core test [tx] --all-versions --rng-seed 12345 --record-test-tx-meta test-tx-meta-baseline-current -o testReport.xml
#for a build with only the current protocol enabled, and:

./stellar-core test [tx] --all-versions --rng-seed 12345 --record-test-tx-meta test-tx-meta-baseline-next -o testReport.xml
#for a build configured with `--enable-next-protocol-version-unsafe-for-production`.
```

These commands will rewrite the baseline files, which are human-readable JSON files. You should then inspect to see that only the transactions you expected to see change did so. If so, commit the changes as a new set of baselines for future tests.
