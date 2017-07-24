# Jake Mayeux
# July 2017
# mixing-v2.py

import re
import sys
import getopt
from os import walk

absolute = -1

infile = 'ZMixingIn.gcode'

outfile = 'ZMixingOut.gcode'

colorfiles = 'color-codes'

maxZ = float(-1.0)

zhop = float(1.0) # needed to properly detect layer changes

zpos = float(0.0)

#lastz = float(-1.0)

numColors = -1 # -1 to use all colors from the color code file

numExtruders = 16 

opts, extraparams = getopt.getopt(
   sys.argv[1:],
   'z:f:o:',
   ['zhop', 'infile', 'outfile'])
for o, p in opts:
   if o in ['-z', '--zhop']:
      zhop = p
   elif o in ['-f', '--infile']:
      infile = p
   elif o in ['-o', '--outfile']:
      outfile = p

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
        return float(m.group(0))
    except ValueError:
        return default

# iterate through lines from last to first
for x in reversed(range(len(lines))):
   l = lines[x]
   i = l.find(' ')
   if (i == -1):
      gc = l
   else:
      gc = l[0:i]
   
   # remove comments
   if (gc.find(';') != -1):
      gc = gc[:gc.find(';')]

   if (gc == 'G1'):
      # if we got a Z, its the last z, so its the max
      maxZ = getValue(l, 'Z', -1)
      if (maxZ != -1):
         break

swapHeight = maxZ / numExtruders
nextSwap = swapHeight
print(maxZ)
print(swapHeight)

ct = 0 # current T
ouf.write('T'+str(ct)+'\n')
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
      newz = getValue(l, 'Z', -1)
      #print(newz)
      if (newz != -1 and newz - zpos < zhop - 0.01 and newz != zpos):
         if (zpos == 0):
            zpos = newz
         elif (zpos > 0):
            zpos = newz
            # new layer here
            #print(zpos)
            if (zpos >= nextSwap):
               nextSwap += swapHeight
               print(nextSwap)
               ct += 1
               ouf.write('T'+str(ct)+'\n')
   elif (gc[0:1] == 'T'):
      tn = int(gc[1:])
      if (ltn == -1):
         ltn = tn
      elif (tn != ltn):
         # swap here
         ltn = tn
         if (zpos > 0):
            drawNextTower()
   ouf.write(l)
