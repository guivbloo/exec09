#!/bin/bash

lxterminal -t console_server -e /workspaces/exec09/tools/console_server &

/workspaces/exec09/m6809-run $*

val1=$(ps -ef | pgrep console_server)

kill $val1
exit 0
