#!/bin/bash

lxterminal -t console_server -e /home/pi/6809/exec09/branches/mc6840/console_server/console_server /home/pi/6809/MoOS/branches/mc6840/csw/csw.bin &
lxterminal -t debug_server -e /home/pi/6809/exec09/branches/mc6840/debug_server/debug_server &

/home/pi/6809/exec09/branches/mc6840/m6809-run $*

val1=$(ps -ef | pgrep console_server)
val2=$(ps -ef | pgrep debug_server)

kill $val1
kill $val2
exit 0
