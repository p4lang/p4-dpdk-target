# Simple L2 Forwarding Test

## Environment setup

If you have set up a local environment already, you can skip this part. Otherwise, you can also use a Docker container to run this test.

The following commands need to run at the root of this repo where the `Dockerfile` resides.

First build a Docker image:
```
./docker-build.sh
```

Then run a Docker container from this image and enter an interactive shell:
```
./docker-run.sh
```

## Manual testing steps

Switch to this directory and run the following commands:
```
./cmd/build.sh
./cmd/run_bf_switchd.sh
```

Then you shall see the `bfshell>` prompt.
