import pexpect
import multiprocessing
import sys

cores = multiprocessing.cpu_count()
sys.stderr.write( "helloworldcpp on " + str(cores) + " cores [")
child = pexpect.spawn('src/app/HelloWorld/HelloWorld')
for i in range(cores):
    child.expect('Hello World', timeout=10)
print "]"
