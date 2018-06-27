#!/bin/sh
rc -o resource.rsrc $1/resource.rdef
xres -o "$2" resource.rsrc
mimeset -f "$2"
