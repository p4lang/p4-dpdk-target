#!/usr/bin/expect -f
##
## Copyright(c) 2021 Intel Corporation.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
## http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##

set BFSHELL [lindex $argv 0];
set BFLAG [lindex $argv 1];
set TESTSCRIPT [lindex $argv 2];
set IFLAG [lindex $argv 3];
set P4WORKSPACE [lindex $argv 4];
set INSTALL_DIR [lindex $argv 5];

set force_conservative 0;
# set to 1 to force conservative mode even if
# script wasn't run conservatively originally

if {$force_conservative} {
	set send_slow {1 .1}
	proc send {ignore arg} {
		sleep .1
		exp_send -s -- $arg
	}
}

#
# 2) differing output - Some programs produce different output each time
# they run.  The "date" command is an obvious example.  Another is
# ftp, if it produces throughput statistics at the end of a file
# transfer.  If this causes a problem, delete these patterns or replace
# them with wildcards.  An alternative is to use the -p flag (for
# "prompt") which makes Expect only look for the last line of output
# (i.e., the prompt).  The -P flag allows you to define a character to
# toggle this mode off and on.
#
# Read the man page for more info.
#
# -Don


set timeout 10
send_user "bfshell script : $BFSHELL $BFLAG $TESTSCRIPT $IFLAG\n"
send_user "p4 workspace : $P4WORKSPACE\n"
send_user "install dir  : $INSTALL_DIR\n\n"
spawn $P4WORKSPACE/$BFSHELL $BFLAG $TESTSCRIPT $IFLAG
match_max 100000
expect {
    "Using workspace $P4WORKSPACE\r
Connecting to localhost port 7777 to check status on these devices: \[0\]\r
Waiting for device 0 to be ready\r
$INSTALL_DIR/bin/bfshell\r
\r\r\r
        ********************************************\r\r\r
        *      WARNING: Authorised Access Only     *\r\r\r
        ********************************************\r\r\r
    \r\r\r
bfshell> *"
    timeout { exit 1 }
}

expect {
    "*Error*" { exit 1 }
    "*Success*" { send -- "e" }
    timeout { exit 1 }
}

expect {
    "*e*" { send -- "x" }
    timeout { exit 1 }
}

expect {
    "*x*" { send -- "i" }
    timeout { exit 1 }
}

expect {
    "*i*" { send -- "t" }
    timeout { exit 1 }
}

expect {
    "*t*" { send -- "\r" }
    timeout { exit 1 }
}

expect {
    "*bfshell>*" { send -- "exit\r" }
    timeout { exit 1 }
}

expect eof
