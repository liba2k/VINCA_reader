#!/bin/env python
import os
import shutil
import subprocess

site_files = ['index.html', 'favicon.png', 'csv.png', 'reconnecting_websocket.js', 'LCD.woff', 'jexcel.css', 'jexcel.js', 'jexcel.themes.css', 'jsuites.css', 'jsuites.js']
\
with open(os.path.join('include','static.h'), 'w') as f_out:
    for file_name in site_files:
            gz = output = subprocess.Popen(["gzip", "-k", "-c", os.path.join('web',file_name)], stdout=subprocess.PIPE).communicate()[0]
            fn = file_name.replace('.', '_').replace('-', '_')
            f_out.write('const char PROGMEM {}[{}] = {{'.format(fn, len(gz)))
            for b in gz:
                f_out.write('{}, '.format(hex(b)))
            f_out.write('};\n')

