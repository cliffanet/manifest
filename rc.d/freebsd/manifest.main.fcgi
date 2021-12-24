#!/bin/sh
#
# $FreeBSD: main (manifest), v1.0 2021/12/24 23:10:55 flood Exp 
#
# PROVIDE: manifest
# REQUIRE: NETWORKING
#
# Add the following line to /etc/rc.conf to enable manifest_main:
#
# manifest_main_enable="YES"
#

manifest_main_enable="${manifest_main_enable-NO}"
. /etc/rc.subr


name=manifest_main
rcvar=`set_rcvar`

prefix=/home/manifest
procname=fcgi-manifest-main
pidfile=/var/run/manifest/main.fcgi.pid
required_files="${prefix}/redefine.conf"
command="${prefix}/fcgi/main"

load_rc_config ${name}

run_rc_command "$1"
