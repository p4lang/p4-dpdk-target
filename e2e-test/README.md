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
        (generated) bf_switchd_config.json:
            Config file needed by the bf_switchd binary.
        bfshell_cmds.py:
            Commands to run with bfshell.
        main.p4:
            The P4 pipeline code.

tools/
    Tools used to run the tests.
```

## Example testing workflow

### Compile P4 code

```console
./tools/cmd.py compile suites/pna/simple_l2_forwarding
```

The following paths will be generated:

- `suites/pna/simple_l2_forwarding/build/`

### Run `bf_switchd`

```console
./tools/cmd.py bf_switchd suites/pna/simple_l2_forwarding
```

The following paths will be generated:

- `suites/pna/simple_l2_forwarding/log/bf_switchd/`
- `suites/pna/simple_l2_forwarding/bf_switchd_conf.json`

### Run `bfshell`

Keep the `bf_switchd` terminal open. In another terminal, run:

```console
./tools/cmd.py bfshell suites/pna/simple_l2_forwarding
```

### Clean up (optional)

```console
./tools/cmd.py clean suites/pna/simple_l2_forwarding
```
