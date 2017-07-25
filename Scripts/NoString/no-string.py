# Jake Mayeux
# July 2017
# no-string.py

import re
import sys
import getopt
from os import walk

absolute = -1

infile = 'eiffel-in.gcode'

outfile = 'eiffel-out.gcode'

inf = open(infile, 'r')
ouf = open(outfile, 'w')

newe = float(0.0)

epos = float(0.0)

edir = True # true = increasing, false = decreasing

rdist = float(0.0) # retraction distance

edist = float(0.0) # extrusion distance

firstG92 = True

lines = inf.readlines()

def getValue(line, key, default=None):
    if (key not in line) or (';' in line and line.find(key) > line.find(';')):
        return default
    sub_part = line[line.find(key) + 1:]
    m = re.search('^[0-9]+\.?[0-9]*', sub_part)
    if m is None:
        return default
    try:
        return float(m.group(0))
    except ValueError:
        return default

for l in lines:
   i = l.find(' ')
   if (i == -1):
      gc = l
   else:
      gc = l[0:i]
   
   # remove comments
   if (gc.find(';') != -1):
      gc = gc[:gc.find(';')]

   if (gc == 'G1'):
      # check if Z has changed, check if it is a zhop movement
      newe = getValue(l, 'E', float(-1.0))
      #print(newz)
      if (newe != -1):
         if (edir and newe - epos < 0): # begins retracting
            rdist = 0
            edir = False
            ouf.write('T0\n')
         elif (not edir and newe - epos > 0): # begins extruding
            edist = 0
            edir = True
         if (not edir):
            rdist += epos - newe
         elif (rdist > 0 and edir):
            edist += newe - epos
         if (edir and rdist > 0 and edist >= rdist):
            print(rdist, edist)
            rdist = 0
            edist = 0
            ouf.write('T1\n')
         epos = newe
         
   # set a dedicated virtual extruder for retraction
   ouf.write(l)
   if (gc == 'G92' and firstG92):
      firstG92 = False
      ouf.write('M163 S0 P1\n')
      ouf.write('M163 S1 P1\n')
      ouf.write('M163 S2 P1\n')
      ouf.write('M163 S3 P1\n')
      ouf.write('M163 S4 P1\n')
      ouf.write('M164 S0\n')
      ouf.write('M163 S0 P0\n')
      ouf.write('M163 S1 P0\n')
      ouf.write('M163 S2 P1\n')
      ouf.write('M163 S3 P1\n')
      ouf.write('M163 S4 P0\n')
      ouf.write('M164 S1\n')
      ouf.write('T1\n')
