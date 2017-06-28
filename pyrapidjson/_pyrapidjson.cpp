#include <Python.h>
#include <string.h>
#include <cstdio>
#include <sstream>

#include "rapidjson/rapidjson.h"
#include "rapidjson/error/en.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/encodedstream.h"

#if PY_MAJOR_VERSION >= 3
#define PY3
#define PyInt_FromLong PyLong_FromLong
#define PyInt_FromString PyLong_FromString
#define PyInt_Check PyLong_Check
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_Check PyBytes_Check
#define PyString_AsString PyBytes_AsString
#define PyString_GET_SIZE PyUnicode_GET_SIZE
#endif

#ifdef DEBUG
#define _debug(format,...) printf("[DEBUG]" format,__VA_ARGS__)
#define _info(format,...) printf("[INFO]" format,__VA_ARGS__)
#endif

// for Python2.6
#ifndef _PyVerify_fd
#define _PyVerify_fd(FD) (1)
#endif

static bool pyobj2doc(PyObject *object, rapidjson::Document& doc);
static bool pyobj2doc(PyObject *object, rapidjson::Value& doc, rapidjson::Document& root);

static PyObject* _doc2pyobj(rapidjson::Document&, char *);
static PyObject* doc2pyobj(rapidjson::Document&);
static PyObject* _get_pyobj_from_object(
        rapidjson::Value::ConstMemberIterator& doc, PyObject *root,
        const char *key);

/* The module doc strings */
PyDoc_STRVAR(pyrapidjson__doc__, "Python binding for rapidjson");
PyDoc_STRVAR(pyrapidjson_loads__doc__, "Decoding JSON");
PyDoc_STRVAR(pyrapidjson_load__doc__, "Decoding JSON file like object");
PyDoc_STRVAR(pyrapidjson_dumps__doc__, "Encoding JSON");
PyDoc_STRVAR(pyrapidjson_dump__doc__, "Encoding JSON file like object");


static inline bool
pyobj2doc_pair(PyObject *key, PyObject *value,
               rapidjson::Value& doc, rapidjson::Document& root)
{
    const char *key_string;
#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(key);
    key_string = PyBytes_AsString(utf8_item);
#else
    key_string = PyString_AsString(key);
#endif
    rapidjson::Value s;
    s.SetString(key_string, root.GetAllocator());
    rapidjson::Value _v;
    if (false == pyobj2doc(value, _v, root)) {
        return false;
    }
    doc.AddMember(s, _v, root.GetAllocator());
    return true;
}
static inline bool
pyobj2doc_pair(PyObject *key, PyObject *value, rapidjson::Document& doc)
{
    const char *key_string;
    PyObject *utf8_item = NULL;
    PyObject *pyobj = NULL;
#ifdef PY3
    if (!PyUnicode_Check(key)) {
        pyobj = PyObject_Str(key);
        if (pyobj == NULL) {
            PyErr_SetString(PyExc_TypeError, "not support key type");
            return false;
        }
        utf8_item = PyUnicode_AsUTF8String(pyobj);
    } else {
        utf8_item = PyUnicode_AsUTF8String(key);
    }
    key_string = PyBytes_AsString(utf8_item);
#else
    if (PyString_Check(key)) {
        key_string = PyString_AsString(key);
    } else if (PyUnicode_Check(key)) {
        utf8_item = PyUnicode_AsUTF8String(key);
        key_string = PyBytes_AsString(utf8_item);
    } else {
        PyObject *pyobj = PyObject_Str(key);
        if (pyobj == NULL) {
            PyErr_SetString(PyExc_TypeError, "not support key type");
            return false;
        }
        Py_DECREF(key);
        key_string = PyString_AsString(pyobj);
    }
#endif
    rapidjson::Value s;
    s.SetString(key_string, doc.GetAllocator());

    Py_XDECREF(pyobj);
    Py_XDECREF(utf8_item);

    rapidjson::Value _v;
    if (false == pyobj2doc(value, _v, doc)) {
        return false;
    }
    doc.AddMember(s, _v, doc.GetAllocator());
    return true;
}

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
    PyObject *obj, *utf8item;

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
#ifdef PY3
        obj = PyString_FromStringAndSize(doc->GetString(),
                                         doc->GetStringLength());
#else
        utf8item = PyUnicode_FromStringAndSize(doc->GetString(),
                                               doc->GetStringLength());
        if (utf8item) {
            obj = utf8item;
        } else {
            obj = PyString_FromStringAndSize(doc->GetString(),
                                         doc->GetStringLength());
        }
#endif
        break;
    case rapidjson::kNumberType:
        if (doc->IsDouble()) {
            obj = PyFloat_FromDouble(doc->GetDouble());
        }
        else {
            obj = PyInt_FromLong(doc->GetInt64());
        }
        break;
    case rapidjson::kNullType:
        Py_INCREF(Py_None);
        obj = Py_None;
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "not support type");
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
    PyObject *obj, *utf8item;

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
#ifdef PY3
        obj = PyString_FromStringAndSize(doc->value.GetString(),
                                         doc->value.GetStringLength());
#else
        utf8item = PyUnicode_FromStringAndSize(doc->value.GetString(),
                                               doc->value.GetStringLength());
        if (utf8item) {
            obj = utf8item;
        } else {
            obj = PyString_FromStringAndSize(doc->value.GetString(),
                                         doc->value.GetStringLength());
        }
#endif
        break;
    case rapidjson::kNumberType:
        if (doc->value.IsDouble()) {
            obj = PyFloat_FromDouble(doc->value.GetDouble());
        }
        else {
            obj = PyInt_FromLong(doc->value.GetInt64());
        }
        break;
    case rapidjson::kNullType:
        Py_INCREF(Py_None);
        obj = Py_None;
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "not support type");
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

static bool
pyobj2doc(PyObject *object, rapidjson::Value& doc, rapidjson::Document& root)
{
    if (PyBool_Check(object)) {
        if (Py_True == object) {
	        doc.SetBool(true);
        }
        else {
	        doc.SetBool(false);
        }
    }
    else if (Py_None == object) {
        doc.SetNull();
    }
    else if (PyFloat_Check(object)) {
        doc.SetDouble(PyFloat_AsDouble(object));
    }
    else if (PyInt_Check(object)) {
        doc.SetInt64(PyLong_AsLong(object));
    }
    else if (PyString_Check(object)) {
        doc.SetString(PyString_AsString(object), PyString_GET_SIZE(object));
    }
    else if (PyUnicode_Check(object)) {
        PyObject *utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            PyErr_SetString(PyExc_RuntimeError, "codec error.");
            return false;
        }
#ifdef PY3
        doc.SetString(PyBytes_AsString(utf8_item), PyBytes_GET_SIZE(utf8_item), root.GetAllocator());
#else
        doc.SetString(PyString_AsString(utf8_item), PyString_GET_SIZE(utf8_item), root.GetAllocator());
#endif
        Py_XDECREF(utf8_item);
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;

        doc.SetArray();
        rapidjson::Value _v;
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyTuple_GetItem(object, i);
            if (false == pyobj2doc(elm, _v, root)) {
                return false;
            }
            doc.PushBack(_v, root.GetAllocator());
        }
    }
    else if (PyList_Check(object)) {
        int len = PyList_Size(object), i;

        doc.SetArray();
        rapidjson::Value _v;
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            if (false == pyobj2doc(elm, _v, root)) {
                return false;
            }
            doc.PushBack(_v, root.GetAllocator());
        }
    }
    else if (PyDict_Check(object)) {
        doc.SetObject();
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(object, &pos, &key, &value)) {
            if (false == pyobj2doc_pair(key, value, doc, root)) {
                return false;
            }
        }
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "invalid python object");
        return false;
    }

    return true;
}

static bool
pyobj2doc(PyObject *object, rapidjson::Document& doc)
{
    if (PyBool_Check(object)) {
        if (Py_True == object) {
	        doc.SetBool(true);
        }
        else {
	        doc.SetBool(false);
        }
    }
    else if (Py_None == object) {
        doc.SetNull();
    }
    else if (PyFloat_Check(object)) {
        doc.SetDouble(PyFloat_AsDouble(object));
    }
    else if (PyInt_Check(object)) {
        doc.SetInt64(PyLong_AsLong(object));
    }
    else if (PyString_Check(object)) {
        doc.SetString(PyString_AsString(object), PyString_GET_SIZE(object));
    }
    else if (PyUnicode_Check(object)) {
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            PyErr_SetString(PyExc_RuntimeError, "codec error.");
            return false;
        }
#ifdef PY3
        doc.SetString(PyBytes_AsString(utf8_item), PyBytes_GET_SIZE(utf8_item), doc.GetAllocator());
#else
        doc.SetString(PyString_AsString(utf8_item), PyString_GET_SIZE(utf8_item), doc.GetAllocator());
#endif
        Py_XDECREF(utf8_item);
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;
        doc.SetArray();
        rapidjson::Value _v;
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyTuple_GetItem(object, i);
            if (false == pyobj2doc(elm, _v, doc)) {
                return false;
            }
            doc.PushBack(_v, doc.GetAllocator());
        }
    }
    else if (PyList_Check(object)) {
        int len = PyList_Size(object), i;
        doc.SetArray();
        rapidjson::Value _v;
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            if (false == pyobj2doc(elm, _v, doc)) {
                return false;
            }
            doc.PushBack(_v, doc.GetAllocator());
        }
    }
    else if (PyDict_Check(object)) {
        doc.SetObject();
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(object, &pos, &key, &value)) {
            if (false == pyobj2doc_pair(key, value, doc)) {
                return false;
            }
        }
    }
    else {
        PyErr_SetString(PyExc_RuntimeError, "invalid python object");
        return false;
    }

    return true;
}

static PyObject *
pyobj2pystring(PyObject *pyjson)
{
    rapidjson::Document doc;

    if (false == pyobj2doc(pyjson, doc)) {
        return NULL;
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer, rapidjson::Document::EncodingType, rapidjson::ASCII<> > writer(buffer);
    doc.Accept(writer);
    std::string s = buffer.GetString();

    return PyString_FromStringAndSize(s.data(), s.length());
}

static PyObject *
_doc2pyobj(rapidjson::Document& doc, char *text)
{
    int is_float;
    unsigned int offset;
    PyObject *pyjson;

    if (!(doc.IsArray() || doc.IsObject())) {
        switch (text[0]) {
        case 't':
            return PyBool_FromLong(1);
        case 'f':
            return PyBool_FromLong(0);
        case 'n':
            Py_RETURN_NONE;
        case '"':
            return PyString_FromStringAndSize(text + 1, strlen(text) - 2);
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
                pyjson = PyInt_FromString(text, NULL, 10);
            }
            return pyjson;
        }
    }

    return doc2pyobj(doc);
}

static PyObject *
pyrapidjson_loads(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {(char *)"text", NULL};
    char *text;
    rapidjson::Document doc;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &text))
        return NULL;

    doc.Parse(text);
    if (doc.HasParseError()) {
        PyErr_SetString(PyExc_ValueError, GetParseError_En(doc.GetParseError()));
        return NULL;
    }

    return _doc2pyobj(doc, text);
}

static PyObject *
pyrapidjson_load(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {(char *)"text", NULL};
    PyObject *py_file, *py_string, *read_method;
    char *text;
    rapidjson::Document doc;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &py_file))
        return NULL;

    if (!PyObject_HasAttrString(py_file, "read")) {
        PyErr_Format(PyExc_TypeError, "expected file object. has not read() method.");
        return NULL;
    }
    read_method = PyObject_GetAttrString(py_file, "read");
    if (!PyCallable_Check(read_method)) {
        Py_XDECREF(read_method);
        PyErr_Format(PyExc_TypeError, "expected file object. read() method is not callable.");
        return NULL;
    }

    py_string = PyObject_CallObject(read_method, NULL);
    if (py_string == NULL) {
        Py_XDECREF(read_method);
        return NULL;
    }

#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(py_string);
    text = PyBytes_AsString(utf8_item);
#else
    text = PyString_AsString(py_string);
#endif
    doc.Parse(text);

    if (doc.HasParseError()) {
        Py_XDECREF(read_method);
        Py_XDECREF(py_string);
        PyErr_SetString(PyExc_ValueError, GetParseError_En(doc.GetParseError()));
        return NULL;
    }

    Py_XDECREF(read_method);
    Py_XDECREF(py_string);

    PyObject *ret = _doc2pyobj(doc, text);
#ifdef PY3
    Py_XDECREF(utf8_item);
#endif
    return ret;
}


static PyObject *
pyrapidjson_dumps(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {(char *)"obj", NULL};
    PyObject *pyjson;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &pyjson))
        return NULL;

    return pyobj2pystring(pyjson);
}


static PyObject *
pyrapidjson_dump(PyObject *self, PyObject *args, PyObject *kwargs)
{
    // TODO: not support kwargs like json.dump() (encoding, etc...)
    static char *kwlist[] = {(char *)"obj", (char *)"fp", NULL};
    PyObject *py_file, *py_json, *py_string, *write_method, *write_arg, *write_ret;
    rapidjson::Document doc;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &py_json, &py_file))
        return NULL;

    if (!PyObject_HasAttrString(py_file, "write")) {
        PyErr_Format(PyExc_TypeError, "expected file object. has not write() method.");
        return NULL;
    }
    write_method = PyObject_GetAttrString(py_file, "write");
    if (!PyCallable_Check(write_method)) {
        Py_XDECREF(write_method);
        PyErr_Format(PyExc_TypeError, "expected file object. write() method is not callable.");
        return NULL;
    }

    py_string = pyobj2pystring(py_json);
    if (py_string == NULL) {
        Py_XDECREF(write_method);
        PyErr_SetString(PyExc_RuntimeError, "pyobj2pystring() error.");
        return NULL;
    }

    write_arg = PyTuple_Pack(1, py_string);
    if (write_arg == NULL) {
        Py_XDECREF(write_method);
        return NULL;
    }

    write_ret = PyObject_CallObject(write_method, write_arg);
    if (write_ret == NULL) {
        Py_XDECREF(write_method);
        Py_XDECREF(write_arg);
        Py_XDECREF(py_string);
        return NULL;
    }

    Py_XDECREF(write_method);
    Py_XDECREF(write_arg);
    Py_XDECREF(write_ret);
    Py_XDECREF(py_string);

    Py_RETURN_NONE;
}

static PyMethodDef PyrapidjsonMethods[] = {
    {"loads", (PyCFunction)pyrapidjson_loads, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_loads__doc__},
    {"load", (PyCFunction)pyrapidjson_load, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_load__doc__},
    {"dumps", (PyCFunction)pyrapidjson_dumps, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_dumps__doc__},
    {"dump", (PyCFunction)pyrapidjson_dump, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_dump__doc__},
    {NULL, NULL, 0, NULL} /* Sentinel */
};

#ifdef PY3
static struct PyModuleDef pyrapidjson_module_def = {
    PyModuleDef_HEAD_INIT,
    "rapidjson",
    pyrapidjson__doc__,
    -1,
    PyrapidjsonMethods,
};

PyMODINIT_FUNC
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
