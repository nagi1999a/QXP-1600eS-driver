#!/bin/bash
#
case "$1" in
 'start')
   insmod /lib/modules/`uname -r`/kernel/drivers/mv14xx/mv14xx.ko
   ;;
 'stop')
   ;;
esac

