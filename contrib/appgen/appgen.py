#!/usr/bin/python
from jinja2 import Environment, FileSystemLoader
import os
import sys
import argparse
 
parser = argparse.ArgumentParser(description='Generate EbbRT application', 
	prog=sys.argv[0], usage='%(prog)s [options]')
parser.add_argument('-n', metavar="NAME", default="app", help='new application name')
parser.add_argument('-t', default="default", choices=['default', 'native'], help='application template')
args = parser.parse_args()

template = args.t
appname = args.n 

# Capture the script (source) directory
source_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "template/"+template) 
target_path = os.getcwd()
CONTEXT = { "title" : appname, "CAP_TITLE": appname.upper() }

def apply_template(source, target):
    if not os.path.exists(target):
        os.makedirs(target)
    print "Building new application: "+target
    #print "Using template: "+source
    for path,sd,files in os.walk(source):
        relative_root = path[len(source)+1:]
        target_root = os.path.join(target, relative_root)
        for s in sd:
            target_root = os.path.join(target, relative_root)
            subdir = os.path.join(target_root, s)
            if not os.path.exists(subdir):
                os.makedirs(subdir)
        for name in files:
            relative_path = os.path.join(relative_root, name)
            full_source_path = os.path.join(path, name)
            if name == "title.cc":
                full_target_path = os.path.join(target_root, appname+".cc")
            else:
                full_target_path = os.path.join(target_root, name)
            print "writing "+full_target_path
            with open(full_target_path, 'w') as f:
                new = render_template(source_path, relative_path, CONTEXT)
                f.write(new)
                f.write(os.linesep)

def render_template(fs, filename, context):
        return Environment(loader=FileSystemLoader(fs),
                         trim_blocks=True).get_template(filename).render(context)

if __name__ == '__main__':
    apply_template(source_path, target_path)
    print "New EbbRT application ("+appname+") generated in current working directory"
