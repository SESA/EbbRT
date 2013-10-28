from cython.operator cimport dereference as deref

cdef extern from "ebb/EventManager/Future.hpp" namespace "ebbrt":
    cdef cppclass Future[T]:
        pass

cdef extern from "app/Sage/ebb_matrix_helper.hpp":
    void activate_context()
    void deactivate_context()
    void wait_for_future(Future[void]*)

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
    def __cinit__(self, size):
        activate_context()
        cdef EbbManager* manager = deref(ebb_manager)
        self.matrix = <EbbRef[Matrix]>manager.AllocateId()
        manager.Bind(ConstructRoot, <EbbId>self.matrix)
        SetSize(<EbbId>self.matrix, size)
        deactivate_context()
    def randomize(self):
        activate_context()
        cdef Matrix* ref = deref(self.matrix)
        cdef Future[void] fut = ref.Randomize()
        wait_for_future(&fut)
        deactivate_context()
