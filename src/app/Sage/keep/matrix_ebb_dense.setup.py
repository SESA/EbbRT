from distutils.core import setup
from Cython.Build import cythonize
from distutils.extension import Extension

inc_dirs = [ "/home/jappavoo/Work/sage/sage-5.12/devel/sage",
             "/home/jappavoo/Work/sage/sage-5.12/devel/sage/sage/matrix",
             "/home/jappavoo/Work/sage/sage-5.12/devel/sage/sage/ext/",
             "/home/jappavoo/Work/sage/sage-5.12/devel/sage/c_lib/include" ]

extensions = [
    Extension("matrix_ebb_dense", ["matrix_ebb_dense.pyx"],
              include_dirs = inc_dirs,
              libraries = [],
              library_dirs = []),
]

setup(
    ext_modules = cythonize(extensions, include_path=inc_dirs)
)

