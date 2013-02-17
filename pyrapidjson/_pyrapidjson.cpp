#include <Python.h>
#include <string.h>

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#if PY_MAJOR_VERSION >= 3
#define PY3
#endif

#define DEFAULT_TOKEN_SIZE 1024

#ifdef DEBUG
#define _debug(format,...) printf("[DEBUG]" format,__VA_ARGS__)
#define _info(format,...) printf("[INFO]" format,__VA_ARGS__)
#endif

static PyObject* doc2pyobj(rapidjson::Document&);
static PyObject* _get_pyobj_from_object(
        rapidjson::Value::ConstMemberIterator& doc, PyObject *root,
        const char *key);

/* The module doc strings */
#ifndef PY3
PyDoc_STRVAR(pyrapidjson__doc__, "Python binding for rapidjson");
#endif
PyDoc_STRVAR(pyrapidjson_loads__doc__, "Decoding JSON");

static inline void
_set_pyobj(PyObject *parent, PyObject *child, const char *key)
{
    if (child && !parent) {
        return;
    }

    if (PyList_Check(parent)) {
        PyList_Append(parent, child);
        if (child && child != Py_None) {
            Py_XDECREF(child);
        }
    }
    else if (PyDict_Check(parent)) {
        PyDict_SetItemString(parent, key, child);
        if (child && child != Py_None) {
            Py_XDECREF(child);
        }
    }
}

static PyObject *
_get_pyobj_from_array(rapidjson::Value::ConstValueIterator& doc,
                      PyObject *root)
{
    PyObject *obj;

    switch (doc->GetType()) {
    case rapidjson::kObjectType:
        obj = PyDict_New();
        for (rapidjson::Value::ConstMemberIterator itr = doc->MemberBegin();
             itr != doc->MemberEnd(); ++itr) {
            _get_pyobj_from_object(itr, obj, itr->name.GetString());
        }
        break;
    case rapidjson::kArrayType:
        obj = PyList_New(0);
        for (rapidjson::Value::ConstValueIterator itr = doc->Begin();
             itr != doc->End(); ++itr) {
            _get_pyobj_from_array(itr, obj);
        }
        break;
    case rapidjson::kTrueType:
        obj = PyBool_FromLong(1);
        break;
    case rapidjson::kFalseType:
        obj = PyBool_FromLong(0);
        break;
    case rapidjson::kStringType:
        obj = PyString_FromStringAndSize(doc->GetString(),
                                         doc->GetStringLength());
        break;
    case rapidjson::kNumberType:
        if (doc->IsDouble()) {
            obj = PyFloat_FromDouble(doc->GetDouble());
        }
        else {
            obj = PyInt_FromLong(doc->GetInt());
        }
        break;
    case rapidjson::kNullType:
        Py_INCREF(Py_None);
        obj = Py_None;
        break;
    default:
        printf("not support. type:%d\n", doc->GetType());
        return NULL;
    }

    _set_pyobj(root, obj, NULL);

    return root;
}

static PyObject *
_get_pyobj_from_object(rapidjson::Value::ConstMemberIterator& doc,
                       PyObject *root,
                       const char *key)
{
    PyObject *obj;

    switch (doc->value.GetType()) {
    case rapidjson::kObjectType:
        obj = PyDict_New();
        for (rapidjson::Value::ConstMemberIterator itr = doc->value.MemberBegin();
             itr != doc->value.MemberEnd(); ++itr) {
            _get_pyobj_from_object(itr, obj, itr->name.GetString());
        }
        break;
    case rapidjson::kArrayType:
        obj = PyList_New(0);
        for (rapidjson::Value::ConstValueIterator itr = doc->value.Begin();
             itr != doc->value.End(); ++itr) {
            _get_pyobj_from_array(itr, obj);
        }
        break;
    case rapidjson::kTrueType:
        obj = PyBool_FromLong(1);
        break;
    case rapidjson::kFalseType:
        obj = PyBool_FromLong(0);
        break;
    case rapidjson::kStringType:
        obj = PyString_FromStringAndSize(doc->value.GetString(),
                                         doc->value.GetStringLength());
        break;
    case rapidjson::kNumberType:
        if (doc->value.IsDouble()) {
            obj = PyFloat_FromDouble(doc->value.GetDouble());
        }
        else {
            obj = PyInt_FromLong(doc->value.GetInt());
        }
        break;
    case rapidjson::kNullType:
        Py_INCREF(Py_None);
        obj = Py_None;
        break;
    default:
        printf("not support. type:%d\n", doc->value.GetType());
        return NULL;
    }

    _set_pyobj(root, obj, key);
    return obj;
}

static PyObject *
doc2pyobj(rapidjson::Document& doc)
{
    PyObject *root;

    if (doc.IsArray()) {
        root = PyList_New(0);
        for (rapidjson::Value::ConstValueIterator itr = doc.Begin();
             itr != doc.End(); ++itr) {
            _get_pyobj_from_array(itr, root);
        }
    }
    else {
        // Object
        root = PyDict_New();
        for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin();
             itr != doc.MemberEnd(); ++itr) {
            _get_pyobj_from_object(itr, root, itr->name.GetString());
        }
    }

    return root;
}

static PyObject *
pyrapidjson_loads(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"text", NULL};
    char *text;
    PyObject *pyjson;
    rapidjson::Document doc;
    int is_float;
    unsigned int offset;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &text))
        return NULL;

    doc.Parse<0>(text);

    if (!(doc.IsArray() || doc.IsObject())) {
        switch (text[0]) {
        case 't':
            return PyBool_FromLong(1);
        case 'f':
            return PyBool_FromLong(0);
        case 'n':
            Py_RETURN_NONE;
        default:
            is_float = 0;
            for (offset = 0; offset < strlen(text); offset++) {
                switch (text[offset]) {
                    case '.':
                    case 'e':
                    case 'E':
                        is_float = 1;
                        break;
                }
                if (is_float) break;
            }
            if (is_float) {
                pyjson = PyFloat_FromDouble(atof(text));
            }
            else {
                pyjson = PyLong_FromLong(atol(text));
            }
            return pyjson;
        }
    }

    return doc2pyobj(doc);
}


static PyMethodDef PyrapidjsonMethods[] = {
    {"loads", (PyCFunction)pyrapidjson_loads, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_loads__doc__},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

#ifdef PY3
static struct PyModuleDef pyrapidjson_module_def = {
    PyModuleDef_HEAD_INIT,
    "rapidjson",
    "Python binding for rapidjson",
    -1,
    PyrapidjsonMethods,
};
PyObject *
PyInit_rapidjson(void)
#else

PyMODINIT_FUNC
initrapidjson(void)
#endif
{
    PyObject *module;

#ifdef PY3
    module = PyModule_Create(&pyrapidjson_module_def);
    return module;
#else
    /* The module */
    module = Py_InitModule3("rapidjson", PyrapidjsonMethods, pyrapidjson__doc__);
    if (module == NULL)
        return;
#endif
}
