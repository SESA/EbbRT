# distutils: language = c++
# distutils: sources = Rectangle.cpp

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


from sage.rings.integer cimport Integer
from sage.rings.rational_field import QQ
from sage.rings.real_double import RDF
from sage.rings.integer_ring import ZZ, IntegerRing_class
from sage.rings.integer_ring cimport IntegerRing_class

from sage.matrix.matrix cimport Matrix
from sage.matrix.matrix_dense cimport Matrix_dense 

cdef class Matrix_ebb_dense(Matrix_dense):   # dense or sparse
    ########################################################################
    # LEVEL 1 functionality
    # x * __cinit__  
    # x * __dealloc__   
    # x * __init__      
    # x * set_unsafe
    # x * get_unsafe
    # x * def _pickle
    # x * def _unpickle
    ########################################################################
    def __cinit__(self, parent, entries, coerce, copy):
        self._parent = parent
        self._base_ring = ZZ
        self._nrows = parent.nrows()
        self._ncols = parent.ncols()
	# allocate ebb
        raise RuntimeError("__cinit__")	
  
    def __copy__(self):
        raise RuntimeError("__copy__")

    def __hash__(self):
        raise RuntimeError("__hash__")

    def __dealloc__(self):
        raise RuntimeError("__dealloc__")

    def __init__(self, parent, entries, copy, coerce):
        raise RuntimeError("__init__")

    cdef set_unsafe(self, Py_ssize_t i, Py_ssize_t j, value):
        raise RuntimeError("set_unsafe")

    cdef get_unsafe(self, Py_ssize_t i, Py_ssize_t j):
        raise RuntimeError("get_unsafe")

    def _pickle(self):
        raise RuntimeError("_pickle")

    cdef _pickle_version0(self):
        raise RuntimeError("_pickle_version0")

    cpdef _export_as_string(self, int base=10):
        raise RuntimeError("_export_as_string")

    def _unpickle(self, data, int version):
        raise RuntimeError("_unpickle")

    cdef _unpickle_version0(self, data):
        raise RuntimeError("_unpickle_version0")

    def __richcmp__(Matrix self, right, int op):  # always need for mysterious reasons.
        return self._richcmp(right, op)

    ########################################################################
    # LEVEL 1 helpers:
    #   These function support the implementation of the level 1 functionality.
    ########################################################################    
    cdef _zero_out_matrix(self):
        raise RuntimeError("_zero_out_matrix")

    cdef _new_unitialized_matrix(self, Py_ssize_t nrows, Py_ssize_t ncols):
        """
        Return a new matrix over the integers with the given number of rows
        and columns. All memory is allocated for this matrix, but its
        entries have not yet been filled in.
        """
        P = self._parent.matrix_space(nrows, ncols)
        return Matrix_ebb_dense.__new__(Matrix_ebb_dense, P, None, None, None)

    ########################################################################
    # LEVEL 2 functionality
    # x * cdef _add_         
    # x * cdef _sub_         
    # x * cdef _mul_
    # x * cdef _cmp_c_impl
    #   * __neg__
    #   * __invert__
    #   * __copy__
    # x * _multiply_classical
    #   * _list -- list of underlying elements (need not be a copy)
    #   * _dict -- sparse dictionary of underlying elements (need not be a copy)
    ########################################################################
    # cdef _mul_(self, Matrix right):
    # def __neg__(self):
    # def __invert__(self):
    # def __copy__(self):
    # def _multiply_classical(left, matrix.Matrix _right):
    # def _list(self):
    # def _dict(self):
    # def __nonzero__(self):
    #     cpdef ModuleElement _lmul_(self, RingElement right):
    #     def _multiply_classical(self, Matrix right):
    #     cpdef ModuleElement _add_(self, ModuleElement right):
    #     cpdef ModuleElement _sub_(self, ModuleElement right):
    #     cdef int _cmp_c_impl(self, Element right) except -2:
    #    cdef Vector _vector_times_matrix_(self, Vector v):

    ########################################################################
    # LEVEL 3 functionality (Optional)
    #    * __deepcopy__
    #    * __invert__
    #    * Matrix windows -- only if you need strassen for that base
    #    * Other functions (list them here):
    #    * Specialized echelon form
    ########################################################################
