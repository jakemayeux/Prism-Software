# Jake Mayeux
# July 2017
# gcode-to-color.py

import re
import sys
import getopt
from os import walk

infile = ''

outfile = ''

opts, extraparams = getopt.getopt(
   sys.argv[1:],
   'f:')

for o, p in opts:
   if o == '-f':
      infile = p


if infile[infile.find('.'):] != '.gcode':
   print('.gcode files only')
   sys.exit()

outfile = infile[:infile.find('.')] + '.txt'

inf = open(infile, 'r')
ouf = open(outfile, 'w')

lines = inf.readlines()

def getValue(line, key, default=None):
    if (key not in line) or (';' in line and line.find(key) > line.find(';')):
        return default
    sub_part = line[line.find(key) + 1:]
    m = re.search('^[0-9]+\.?[0-9]*', sub_part)
    if m is None:
        return default
    try:
        return m.group(0)
    except ValueError:
        return default

for l in lines:
   if l[0:4] == 'M163':
      ouf.write(getValue(l,'P','x'))
      if getValue(l, 'S', 'x') != '4':
         ouf.write(',')
   elif l[0:4] == 'M164':
      ouf.write('\n')
