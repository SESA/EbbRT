from sage.ext.mod_int cimport *    
from sage.matrix.matrix_dense cimport Matrix_dense

cdef class Matrix_ebb_dense(Matrix_dense):
    cdef _zero_out_matrix(self)
    cdef _new_unitialized_matrix(self, Py_ssize_t nrows, Py_ssize_t ncols)
    cdef _pickle_version0(self)
    cdef _unpickle_version0(self, data)
    cpdef _export_as_string(self, int base=?)

