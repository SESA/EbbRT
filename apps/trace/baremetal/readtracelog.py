import sys
import subprocess

if len(sys.argv) != 3:
  print "Error: wrong amount of arguments."
  exit(1)

def addr2line(inst):
    try:
        return traces[inst]
    except KeyError:
        cmd = "addr2line -s -f -C -e " + sys.argv[1]+' '+inst
        fname = subprocess.check_output(cmd, shell=True).splitlines()
        traces[inst] = fname[0]
        return fname[0]

#vars
depth = 0
#stacks
times = []
cycles = []
insts = []
#hashmap
traces = {}

with open(sys.argv[2]) as f:
  for line in f:
    if line.startswith("1"):
      depth += 1
      data = line.split(' ')
      fname = addr2line(data[1])
      cname = addr2line(data[2])
      times.append(int(data[3]))
      cycles.append(int(data[4]))
      insts.append(int(data[5]))
      print "%s,>,%s,%s" % (depth,fname,cname)
    elif line.startswith("0"):
      data = line.split(' ')
      fname = addr2line(data[1])
      if depth > 0:
        ttime = int(data[3]) - times.pop()
        tcycles = int(data[4]) - cycles.pop()
        tinst = int(data[5]) - insts.pop()
        print "%s,<,%s,%s,%s,%s" % (depth,fname,tcycles,tinst,ttime)
      else:
        print "%s,<,%s,?,?,?" % (depth,fname)
      depth -= 1
    else:
      # non-trace line
      depth = 0
      times = []
      cycles = []
      insts = []
      print line
  print "Trace print completed."
