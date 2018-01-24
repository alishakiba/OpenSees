/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */

// Written: Minjie

// Description: This file contains the class definition for Python Module
// PythonModule implements a DL_Interpreter for the python language
//

#include "PythonModule.h"


PythonModule::PythonModule()
    :wrapper(), cmds(this)
{
    // does nothing
}

PythonModule::~PythonModule()
{
    // does nothing
}

int
PythonModule::run()
{
    return 0;
}

int
PythonModule::addCommand(const char *, Command &)
{
    return -1;
}

int
PythonModule::removeCommand(const char *)
{
    return -1;
}

int
PythonModule::getNumRemainingInputArgs(void)
{
    return wrapper.getNumberArgs() - wrapper.getCurrentArg();
}

int
PythonModule::getInt(int *data, int numArgs)
{
    if ((wrapper.getNumberArgs() - wrapper.getCurrentArg()) < numArgs) {
        return -1;
    }
    
    for (int i = 0; i < numArgs; i++) {
        PyObject *o = PyTuple_GetItem(wrapper.getCurrentArgv(), wrapper.getCurrentArg());
        wrapper.incrCurrentArg();
#if PY_MAJOR_VERSION >= 3
        if (!PyLong_Check(o)) {
            return -1;
        }
        data[i] = PyLong_AS_LONG(o);
#else
        if (!PyInt_Check(o)) {
            return -1;
        }
        data[i] = PyInt_AS_LONG(o);
#endif
    }
    
    return 0;
}

int
PythonModule::getDouble(double *data, int numArgs)
{
    if ((wrapper.getNumberArgs() - wrapper.getCurrentArg()) < numArgs) {
        return -1;
    }
    
    for (int i = 0; i < numArgs; i++) {
        PyObject *o = PyTuple_GetItem(wrapper.getCurrentArgv(), wrapper.getCurrentArg());
        wrapper.incrCurrentArg();
        if (!PyFloat_Check(o)) {
            return -1;
        }
        data[i] = PyFloat_AS_DOUBLE(o);
    }
    
    return 0;
}

const char*
PythonModule::getString()
{
    if (wrapper.getCurrentArg() >= wrapper.getNumberArgs()) {
        return 0;
    }
    
    PyObject *o = PyTuple_GetItem(wrapper.getCurrentArgv(), wrapper.getCurrentArg());
    wrapper.incrCurrentArg();
#if PY_MAJOR_VERSION >= 3
    if (!PyUnicode_Check(o)) {
        return 0;
    }
    
    return PyUnicode_AsUTF8(o);
#else
    if (!PyString_Check(o)) {
        return 0;
    }
    
    return PyString_AS_STRING(o);
#endif
}

int
PythonModule::getStringCopy(char **stringPtr)
{
    return -1;
}

void
PythonModule::resetInput(int cArg)
{
    wrapper.resetCommandLine(cArg);
}

int
PythonModule::setInt(int* data, int numArgs)
{
    wrapper.setOutputs(data, numArgs);
    
    return 0;
}

int
PythonModule::setDouble(double* data, int numArgs)
{
    wrapper.setOutputs(data, numArgs);
    
    return 0;
}

int
PythonModule::setString(const char* str)
{
    wrapper.setOutputs(str);
    
    return 0;
}

int
PythonModule::runCommand(const char* cmd)
{
    return PyRun_SimpleString(cmd);
}


static PythonModule* module = 0;

PyMethodDef* getmethodsFunc()
{
    module = new PythonModule();
    PythonWrapper* wrapper = module->getWrapper();
    wrapper->addOpenSeesCommands();
    
    return wrapper->getMethods();
}

void cleanupFunc()
{
    if (module != 0) {
        delete module;
    }
}

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyObject *
error_out(PyObject *m)
{
    struct module_state *st = GETSTATE(m);
    PyErr_SetString(st->error, "something bad happened");
    
    return NULL;
}

#if PY_MAJOR_VERSION >= 3

static int opensees_traverse(PyObject *m, visitproc visit, void *arg)
{
    Py_VISIT(GETSTATE(m)->error);
    
    return 0;
}

static int opensees_clear(PyObject *m)
{
    Py_CLEAR(GETSTATE(m)->error);
    
    return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "opensees",
    NULL,
    sizeof(struct module_state),
    getmethodsFunc(),
    NULL,
    opensees_traverse,
    opensees_clear,
    NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_opensees(void)

#else
#define INITERROR return

//void
PyMODINIT_FUNC
initopensees(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("opensees", getmethodsFunc());
#endif
    
    if (module == NULL)
        INITERROR;
    struct module_state *st = GETSTATE(module);
    
    st->error = PyErr_NewException("opensees.Error", NULL, NULL);
    if (st->error == NULL) {
        Py_DECREF(module);
        INITERROR;
    }
    
    Py_AtExit(cleanupFunc);
    
#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
