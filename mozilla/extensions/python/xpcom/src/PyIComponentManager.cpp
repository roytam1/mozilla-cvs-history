/* Copyright (c) 2000-2001 ActiveState Tool Corporation.
   See the file LICENSE.txt for licensing information. */

//
// This code is part of the XPCOM extensions for Python.
//
// Written May 2000 by Mark Hammond.
//
// Based heavily on the Python COM support, which is
// (c) Mark Hammond and Greg Stein.
//
// (c) 2000, ActiveState corp.

#include "PyXPCOM_std.h"
#include <nsIComponentManager.h>

static nsIComponentManager *GetI(PyObject *self) {
	nsIID iid = NS_GET_IID(nsIComponentManager);

	if (!Py_nsISupports::Check(self, iid)) {
		PyErr_SetString(PyExc_TypeError, "This object is not the correct interface");
		return NULL;
	}
	return (nsIComponentManager *)Py_nsISupports::GetI(self);
}

static PyObject *PyCreateInstanceByContractID(PyObject *self, PyObject *args)
{
	char *pid, *notyet = NULL;
	PyObject *obIID = NULL;
	if (!PyArg_ParseTuple(args, "s|zO", &pid, &notyet, &obIID))
		return NULL;
	if (notyet != NULL) {
		PyErr_SetString(PyExc_ValueError, "2nd arg must be none");
		return NULL;
	}
	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID	iid;
	if (obIID==NULL)
		iid = NS_GET_IID(nsISupports);
	else
		if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
			return NULL;

	nsISupports *pis;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->CreateInstanceByContractID(pid, NULL, iid, (void **)&pis);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	/* Return a type based on the IID (with no extra ref) */
	return Py_nsISupports::PyObjectFromInterface(pis, iid, PR_FALSE);
}

static PyObject *PyContractIDToClassID(PyObject *self, PyObject *args)
{
	char *pid;
	if (!PyArg_ParseTuple(args, "s", &pid))
		return NULL;
	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID	iid;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->ContractIDToClassID(pid, &iid);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	return Py_nsIID::PyObjectFromIID(iid);
}

static PyObject *PyCLSIDToContractID(PyObject *self, PyObject *args)
{
	PyObject *obIID;
	if (!PyArg_ParseTuple(args, "O", &obIID))
		return NULL;

	nsIID iid;
	if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;
	char *ret_pid = nsnull;
	char *ret_class = nsnull;
	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->CLSIDToContractID(iid, &ret_class, &ret_pid);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	PyObject *ob_pid = PyString_FromString(ret_pid);
	PyObject *ob_class = PyString_FromString(ret_class);
	PyObject *ret = Py_BuildValue("OO", ob_pid, ob_class);
	nsAllocator::Free(ret_pid);
	nsAllocator::Free(ret_class);
	Py_XDECREF(ob_pid);
	Py_XDECREF(ob_class);
	return ret;
}

static PyObject *PyEnumerateCLSIDs(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIEnumerator *pRet;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->EnumerateCLSIDs(&pRet);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	return Py_nsISupports::PyObjectFromInterface(pRet, NS_GET_IID(nsIEnumerator), PR_FALSE);
}

static PyObject *PyEnumerateContractIDs(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIEnumerator *pRet;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->EnumerateContractIDs(&pRet);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	return Py_nsISupports::PyObjectFromInterface(pRet, NS_GET_IID(nsIEnumerator), PR_FALSE);
}

struct PyMethodDef 
PyMethods_IComponentManager[] =
{
	{ "CreateInstanceByContractID", PyCreateInstanceByContractID, 1},
	{ "createInstanceByContractID", PyCreateInstanceByContractID, 1},
	{ "EnumerateCLSIDs",        PyEnumerateCLSIDs, 1},
	{ "enumerateCLSIDs",        PyEnumerateCLSIDs, 1},
	{ "EnumerateContractIDs",       PyEnumerateContractIDs, 1},
	{ "enumerateContractIDs",       PyEnumerateContractIDs, 1},
	{ "ContractIDToClassID",        PyContractIDToClassID, 1},
	{ "contractIDToClassID",        PyContractIDToClassID, 1},
	{ "CLSIDToContractID",          PyCLSIDToContractID, 1},
	{NULL}
};
