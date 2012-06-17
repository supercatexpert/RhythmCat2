#!/bin/sh
gcc -o db-converter db-converter.c `pkg-config --cflags --libs glib-2.0 gobject-2.0 json-glib-1.0`
