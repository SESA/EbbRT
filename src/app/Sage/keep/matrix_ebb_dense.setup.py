import os
from distutils.core import setup
from Cython.Build import cythonize
from distutils.extension import Extension

SAGE_ROOT = os.environ['SAGE_ROOT']

inc_dirs = [ SAGE_ROOT + "/devel/sage",
             SAGE_ROOT + "/devel/sage/sage/matrix",
             SAGE_ROOT + "/devel/sage/sage/ext/",
             SAGE_ROOT + "/devel/sage/c_lib/include" ]

extensions = [
    Extension("matrix_ebb_dense", ["matrix_ebb_dense.pyx"],
              include_dirs = inc_dirs,
              libraries = [],
              library_dirs = []),
]

setup(
    ext_modules = cythonize(extensions, include_path=inc_dirs)
)

