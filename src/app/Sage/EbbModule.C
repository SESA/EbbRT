#include <python2.7/Python.h>
#include <stdio.h>

static PyObject *EbbModuleError;

static PyObject *
EbbModule_system(PyObject *self, PyObject *args)
{
    const char *command;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;
    
    sts = fprintf(stderr, "%s", command);

    if (sts < 0) {
        PyErr_SetString(EbbModuleError, "fprintf failed");
        return NULL;
    }

    return PyLong_FromLong(sts);
}

static PyMethodDef EbbModuleMethods[] = {
    {"system",  EbbModule_system, METH_VARARGS,
     "Execute a shell command."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

extern int initEbbRT();

PyMODINIT_FUNC
initEbbModule(void)
{
    PyObject *m;

    m = Py_InitModule("EbbModule", EbbModuleMethods);
    if (m == NULL)
        return;

    initEbbRT();

    EbbModuleError = PyErr_NewException((char *)"EbbModule.error", NULL, NULL);
    Py_INCREF(EbbModuleError);
    PyModule_AddObject(m, "error", EbbModuleError);
}

