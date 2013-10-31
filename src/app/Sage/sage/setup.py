import sys
if len(sys.argv) == 1:
    sys.argv += ['build_ext','--inplace']

import os
from distutils.core import setup
from Cython.Build import cythonize
from distutils.extension import Extension


SAGE_ROOT = os.environ['SAGE_ROOT']
SAGE_LOCAL = SAGE_ROOT + "/local"
numpy_depends = [SAGE_LOCAL + '/lib/python/site-packages/numpy/core/include/numpy/_numpyconfig.h']

numpy_include_dir =  SAGE_LOCAL + '/lib/python/site-packages/numpy/core/include'

sage_dirs = [ SAGE_ROOT + "/devel/sage",
             SAGE_ROOT + "/devel/sage/sage/matrix",
             SAGE_ROOT + "/devel/sage/sage/ext/",
             SAGE_ROOT + "/devel/sage/c_lib/include",
             numpy_include_dir ]

extensions = [
    Extension("matrix_ebb_dense", ["matrix_ebb_dense.pyx"],
              include_dirs = sage_dirs,
              libraries = [],
              library_dirs = []),
    Extension("matrix_ebb_real_double_dense", ["matrix_ebb_real_double_dense.pyx"],
              include_dirs = sage_dirs,
              library_dirs = [],
              libraries = [],
              depends = numpy_depends)
#    Extension("hello_ebb", ["hello_ebb.pyx"],
#              library_dirs = ["/home/dschatz/Work/SESA/EbbRT/build/linux/udp/src/app/Sage/.libs"],
#              libraries = ["HelloEbb"] )

]

setup(name='EbbRTSage',
      version='0.01',
      description='EbbRT sage objects',
      author='BU SESA Group',
      ext_modules = cythonize(extensions, include_path=sage_dirs)
)
