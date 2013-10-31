# distutils: language = c++
# distutils: sources = Rectangle.cpp

"""
Dense matrices over the Real Double Field using NumPy

EXAMPLES::

    sage: b=Mat(RDF,2,3).basis()
    sage: b[0]
    [1.0 0.0 0.0]
    [0.0 0.0 0.0]

We deal with the case of zero rows or zero columns::

    sage: m = MatrixSpace(RDF,0,3)
    sage: m.zero_matrix()
    []

TESTS::

    sage: a = matrix(RDF,2,range(4), sparse=False)
    sage: TestSuite(a).run()
    sage: MatrixSpace(RDF,0,0).zero_matrix().inverse()
    []
    
    
AUTHORS:

- Jason Grout (2008-09): switch to NumPy backend, factored out the
  Matrix_double_dense class

- Josh Kantor

- William Stein: many bug fixes and touch ups.
"""

##############################################################################
#       Copyright (C) 2004,2005,2006 Joshua Kantor <kantor.jm@gmail.com>
#  Distributed under the terms of the GNU General Public License (GPL)
#  The full text of the GPL is available at:
#                  http://www.gnu.org/licenses/
##############################################################################
cdef extern from "Rectangle.h" namespace "shapes":
    cdef cppclass Rectangle:
        Rectangle(int, int, int, int) except +
        int x0, y0, x1, y1
        int getLength()
        int getHeight()
        int getArea()
        void move(int, int)

cdef class PyRectangle:
    cdef Rectangle *thisptr      # hold a C++ instance which we're wrapping
    def __cinit__(self, int x0, int y0, int x1, int y1):
        self.thisptr = new Rectangle(x0, y0, x1, y1)
    def __dealloc__(self):
        del self.thisptr
    def getLength(self):
        return self.thisptr.getLength()
    def getHeight(self):
        return self.thisptr.getHeight()
    def getArea(self):
        return self.thisptr.getArea()
    def move(self, dx, dy):
        self.thisptr.move(dx, dy)


from sage.rings.real_double import RDF
from sage.matrix.matrix_dense import Matrix_dense

cimport numpy as cnumpy

numpy=None
scipy=None

cdef class Matrix_ebb_real_double_dense(sage.matrix.matrix_double_dense.Matrix_double_dense):
    """
    Class that implements matrices over the real double field. These
    are supposed to be fast matrix operations using C doubles. Most
    operations are implemented using numpy which will call the
    underlying BLAS on the system.
    
    EXAMPLES::
    
        sage: m = Matrix(RDF, [[1,2],[3,4]])
        sage: m**2
        [ 7.0 10.0]
        [15.0 22.0]
        sage: n= m^(-1); n
        [-2.0  1.0]
        [ 1.5 -0.5]
    
    To compute eigenvalues the use the functions left_eigenvectors or
    right_eigenvectors
    
    ::
    
        sage: p,e = m.right_eigenvectors()
    
    the result of eigen is a pair (p,e), where p is a list of
    eigenvalues and the e is a matrix whose columns are the
    eigenvectors.
    
    To solve a linear system Ax = b where A = [[1,2],[3,4]] and
    b = [5,6].
    
    ::
    
        sage: b = vector(RDF,[5,6])
        sage: m.solve_right(b)
        (-4.0, 4.5)
    
    See the commands qr, lu, and svd for QR, LU, and singular value
    decomposition.
    """
    
    ########################################################################
    # LEVEL 1 functionality
    #   * __cinit__  
    #   * __dealloc__   
    #   * __init__      
    #   * set_unsafe
    #   * get_unsafe
    #   * __richcmp__    -- always the same
    #   * __hash__       -- always simple
    ########################################################################
    def __cinit__(self, parent, entries, copy, coerce):
        print "ebb:__cinit__"
        global numpy
        if numpy is None:
            import numpy
        self._numpy_dtype = numpy.dtype('float64')
        self._numpy_dtypeint = cnumpy.NPY_DOUBLE
        self._python_dtype = float
        # TODO: Make RealDoubleElement instead of RDF for speed
        self._sage_dtype = RDF
        # this is kludge for the moment we preallocate a numpy array for interfacing
        # to local computation if needed.
        self.__create_matrix__()
        self.therect = <void *>new Rectangle(1,2,3,4)
        return

    def __init__(self, parent, entries, copy, coerce):
        print "ebb: __init__: initilize values if you feel like it ;-)"

    cdef set_unsafe(self, Py_ssize_t i, Py_ssize_t j, object value):
        print "set_unsafe called"

    cdef set_unsafe_double(self, Py_ssize_t i, Py_ssize_t j, double value):
        """
        Set the (i,j) entry to value without any type checking or
        bound checking.
        
        This currently isn't faster than calling self.set_unsafe; should
        we speed it up or is it just a convenience function that has the
        right headers?
        """
        self.set_unsafe(i,j,value)

    cdef get_unsafe(self, Py_ssize_t i, Py_ssize_t j):
       print "get_unsafe"	     
       return self._sage_dtype(1)    
 
    cdef double get_unsafe_double(self, Py_ssize_t i, Py_ssize_t j):
        """
        Get the (i,j) entry without any type checking or bound checking.
        
        This currently isn't faster than calling self.get_unsafe; should
        we speed it up or is it just a convenience function that has the
        right headers?
        """
        return self.get_unsafe(i,j)
        
    def rectGetLength(self):
        cdef Rectangle *rp = <Rectangle *>self.therect
        return rp.getLength()

    def __dealloc__(self):
        cdef Rectangle *rp = <Rectangle *>self.therect
        del rp

    def change_ring(self, ring):
        print "ebb: change_ring called" 
        print ring
        return self

    def numpy(self, dtype=None):
        import numpy as np
        print "ebb: numpy: this is a kludge right now need to GATHER values into our local numpy cache"
        print dtype

        if dtype is None or self._numpy_dtype == np.dtype(dtype):
            return self._matrix_numpy.copy()
        else:
            return Matrix_dense.numpy(self, dtype=dtype)
