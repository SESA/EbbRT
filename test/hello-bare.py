import pexpect
import multiprocessing
import sys

cores = multiprocessing.cpu_count()
for z in range(1, cores+1):
    sys.stdout.write( "HelloWorld bare metal on " + str(z) + " cores [")
    sys.stdout.flush()
    child = pexpect.spawn ('qemu-system-x86_64 -nographic -smp ' + str(z) + ' src/app/HelloWorld/HelloWorld.iso')
    # uncomment following line to see results of experiment
    #   child.logfile = sys.stdout
    for j in range(z):
        child.expect('Hello World', timeout=100)
    # child.kill(9)
    child.sendcontrol('a')
    child.send('x')
    child.wait()
    print "]"
