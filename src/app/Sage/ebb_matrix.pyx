from cython.operator cimport dereference as deref

cdef extern from "ebb/EventManager/Future.hpp" namespace "ebbrt":
    cdef cppclass Future[T]:
        pass

cdef extern from "app/Sage/ebb_matrix_helper.hpp":
    void activate_context()
    void deactivate_context()
    void wait_for_future_void "wait_for_future<void>"(Future[void]*)
    double wait_for_future_double "wait_for_future<double>"(Future[double]*)

cdef extern from "inttypes.h":
    ctypedef unsigned int uint32_t

cdef extern from "lrt/EbbId.hpp" namespace "ebbrt::lrt::trans":
    ctypedef uint32_t EbbId

cdef extern from "lrt/EbbRef.hpp" namespace "ebbrt::lrt::trans":
    cdef cppclass EbbRef[T]:
        EbbRef()
        EbbRef(EbbId id)
        T* operator*()
        EbbId operator()

cdef extern from "lrt/EbbRoot.hpp" namespace "ebbrt::lrt::trans":
    cdef cppclass EbbRoot

cdef extern from "ebb/EbbManager/EbbManager.hpp" namespace "ebbrt":
    cdef cppclass EbbManager:
        EbbId AllocateId()
        void Bind(EbbRoot* (*factory)(), EbbId id)
    EbbRef[EbbManager] ebb_manager

cdef extern from "app/Sage/Matrix.hpp" namespace "ebbrt":
    cdef cppclass Matrix:
        Future[void] Randomize()
        Future[double] Get(int, int)
        Future[void] Set(int, int, double)
        Future[double] Sum()

cdef extern from "app/Sage/Matrix.hpp" namespace "ebbrt::Matrix":
    EbbRoot* ConstructRoot()
    void SetSize(EbbId, int)

cdef extern from "ebbrt.hpp" namespace "ebbrt":
    cdef cppclass EbbRT:
        EbbRT()
    cdef cppclass Context:
        Context(EbbRT& instance)
        void Activate()
        void Deactivate()

cdef class EbbMatrix:
    cdef EbbRef[Matrix] matrix
    cdef int size
    def __cinit__(self, size):
        activate_context()
        cdef EbbManager* manager = deref(ebb_manager)
        self.matrix = <EbbRef[Matrix]>manager.AllocateId()
        manager.Bind(ConstructRoot, <EbbId>self.matrix)
        SetSize(<EbbId>self.matrix, size)
        deactivate_context()
    def __init__(self, size):
        self.size = size
    def randomize(self):
        activate_context()
        cdef Matrix* ref = deref(self.matrix)
        cdef Future[void] fut = ref.Randomize()
        wait_for_future_void(&fut)
        deactivate_context()
    def sum(self):
        activate_context()
        cdef Matrix* ref = deref(self.matrix)
        cdef Future[double] fut = ref.Sum()
        cdef double ret = wait_for_future_double(&fut)
        deactivate_context()
        return ret
    def __getitem__(self, pos):
        row,column = pos
        activate_context()
        cdef Matrix* ref = deref(self.matrix)
        cdef Future[double] fut = ref.Get(row, column)
        cdef double ret = wait_for_future_double(&fut)
        deactivate_context()
        return ret
    def __setitem__(self, pos, value):
        row,column = pos
        activate_context()
        cdef Matrix* ref = deref(self.matrix)
        cdef Future[void] fut = ref.Set(row, column, value)
        wait_for_future_void(&fut)
        deactivate_context()
    def __iter__(self):
        for row in range(self.size):
            for column in range(self.size):
                yield self.__getitem__((row, column))
