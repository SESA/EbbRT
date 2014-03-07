import sys
import subprocess

if len(sys.argv) != 3:
  print "Error: wrong amount of arguments."
  exit(1)

with open(sys.argv[2]) as f:
  depth = 0
  times = []
  cycles = []
  insts = []
  for line in f:
    data = line.split(' ')
    fname_s = "addr2line -s -f -C -e " + sys.argv[1]+' '+data[1]
    fname = subprocess.check_output(fname_s, shell=True).splitlines()
    if line.startswith("1"):
      depth += 1
      times.append(int(data[3]))
      cycles.append(int(data[4]))
      insts.append(int(data[5]))
      s = '  ' * depth
      cname_s = "addr2line -s -f -C -i -e "+ sys.argv[1]+' '+data[2]
      cname = subprocess.check_output(cname_s, shell=True).splitlines()
      print s,">>",fname[0],"from", cname[0],"in",cname[1]
    if line.startswith("0"):
      ttime = int(data[3]) - times.pop()
      tcycles = int(data[4]) - cycles.pop()
      tinst = int(data[5]) - insts.pop()
      s = '  ' * depth
      print s,"<< ",fname[0],"[",tcycles,"C,",tinst,"I,",ttime,"T]"
      depth -= 1
  print "finished."
