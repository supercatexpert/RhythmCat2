#!/usr/bin/env python3
from gi.repository import GObject, Gtk
import gettext

RC2Py3PluginInfo = dict({
    'magic': 0x20120103,
    'major_version': 2,
    'minor_version': 0,
    'id': 'rc2-py3-gtk3-test',
    'name': 'Python3 Another Hello world plug-in',
    'version': '0.1',
    'description': 'A hello world plug-in in Python3',
    'author': 'SuperCat',
    'homepage': 'http://supercat-lab.org/'
})

def Probe():
    print('Gtk3Test: Probe!')
    return True
    
def Load():
    print('Gtk3Test: Load!')
    win = Gtk.Window()
    label = Gtk.Label('Hello world!')
    win.add(label)
    win.show_all()
    return True
    
def Unload():
    print('Gtk3Test: Unload!')
    return True
    
def Destroy():
    print('Gtk3Test: Destroy!')
    return


