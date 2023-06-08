# P4 DPDK Target End-to-End Testing

## Directory structure

```
suites/
    Collection of test suites.

    <p4_arch>/<test_suite_name>/
        Individual test suite.

        (generated) log/
            Log files generated during testing.
        (generated) p4c_gen/
            P4 compiler outputs generated from main.p4.
        cmds_bfshell.py:
            Commands to run with bfshell before testing script.
        cmds_shell.sh:
            Commands to run with normal shell before testing script.
        (generated) conf_bf_switchd.json:
            Config file needed by the bf_switchd binary.
        main.p4:
            The P4 pipeline code.
        README.md:
            A description of the test.
        test.py:
            The packet testing script.

tools/
    Tools used to run the tests.
```

## Testing environement

You can either build P4 DPDK target locally following the README at the root of this repo, or build a Docker image. Below we introduce the Docker approach.

All commands in this section are expected to be executed from the root of this repo.

### Docker image building

```console
./tools/docker_build.sh
```

### Convenient Docker scripts

To run a `bash` in a new container:

```console
./tools/docker_run_bash.sh
```

To run a new `bash` in the same running container:

```console
./tools/docker_exec_bash.sh
```

The last script is useful for providing multiple terminal shells in the same container.

## Simple automated smoke test

To run the smoke test in a container, at the **root of this repo**, do:

```console
./tools/docker_run_smoke_test.sh
```

To run it locally, at **this directory**, do:

```console
./tools/test.py suites/pna/basicfwd
```

## Manual testing workflow

In the previous section, all testing steps are executed with a single script. If for some reason (e.g. debugging) you would like to execute these steps separately, the details are described in this section.

All commands in this section are expected to be executed from this directory.

### Compile P4 code

```console
./tools/compile.py suites/pna/basicfwd
```

The following paths will be generated:

- `suites/pna/basicfwd/p4c_gen/`

### Run `bf_switchd`

In terminal 1, do:

```console
./tools/run_bf_switchd.py suites/pna/basicfwd
```

The following paths will be generated:

- `suites/pna/simple_l2_forwarding/log/bf_switchd/`
- `suites/pna/simple_l2_forwarding/conf_bf_switchd.json`

### Run `bfshell`

In terminal 2, do:

```console
./tools/run_bfshell.py suites/pna/basicfwd
```

The following paths will be generated:

- `suites/pna/simple_l2_forwarding/log/bfshell/`

### Run testing script

In terminal 3, do:

```console
./suites/pna/basicfwd/cmds_shell.sh
./suites/pna/basicfwd/test.py
```

### Clean up (optional)

```console
./tools/clean.py suites/pna/basicfwd
```
