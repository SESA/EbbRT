from distutils.core import setup, Extension

module1 = Extension('EbbModule',
                    libraries = [  'PrimitiveEbbManager.a','LocalConsole.a','SimpleEventManager.a','HelloEbb','ebbrt' ], 
                    libraries_dirs = [ '.' ],
                    sources = ['EbbModule.C'])

setup (name = 'EbbModule',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [module1])
