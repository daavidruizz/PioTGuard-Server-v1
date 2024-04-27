#! /bin/bash

### BEGIN INIT INFO
# Provides:          rpi init sh
# Required-Start:    $local_fs 
# Required-Stop:     $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: init services
# Description:       
### END INIT INFO

# Carry out specific functions when asked to by the system
case "$1" in
  start)
    echo "Starting RPI init services"
    /home/pi/dev/blink
    ;;
  stop)
    echo "Stopping RPI init services"
    /home/pi/dev/blink
    ;;
  *)
    echo "Usage: /etc/init.d/foo {start|stop}"
    exit 1
    ;;
esac

exit 0
