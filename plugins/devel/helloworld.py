#!/usr/bin/env python3

RC2Py3PluginInfo = {
    'magic': 0x20120103,
    'major_version': 2,
    'minor_version': 0,
    'id': 'rc2-py3-helloworld',
    'name': 'Python3 Hello world plug-in',
    'version': '0.1',
    'description': 'A hello world plug-in in Python3',
    'author': 'SuperCat',
    'homepage': 'http://supercat-lab.org/'
}

def Probe():
    print('Hello World: Probe!')
    return True
    
def Load():
    print('Hello World: Load!')
    return True
    
def Unload():
    print('Hello World: Unload!')
    return True
    
def Destroy():
    print('Hello World: Destroy!')
    return


