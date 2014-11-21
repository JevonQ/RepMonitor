#!/usr/bin/ksh

pid=`ps -ef|grep repmon|grep -v grep|awk '{print $2}'`
kill -SIGTERM $pid
