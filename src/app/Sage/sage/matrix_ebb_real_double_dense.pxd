# distutils: language = c++
# distutils: sources = Rectangle.cpp

cimport sage.matrix.matrix_double_dense 

cdef class Matrix_ebb_real_double_dense(sage.matrix.matrix_double_dense.Matrix_double_dense):
    cdef void *therect
    cdef set_unsafe_double(self, Py_ssize_t i, Py_ssize_t j, double value)
    cdef double get_unsafe_double(self, Py_ssize_t i, Py_ssize_t j)

