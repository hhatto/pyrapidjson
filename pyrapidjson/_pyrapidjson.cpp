#include <Python.h>
#include <string.h>
#include <cstdio>
#include <sstream>

#include "rapidjson/rapidjson.h"
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
#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(key);
    key_string = PyBytes_AsString(utf8_item);
#else
    key_string = PyString_AsString(key);
#endif
    rapidjson::Value s;
    s.SetString(key_string, doc.GetAllocator());
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
#ifdef PY3
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            PyErr_SetString(PyExc_RuntimeError, "codec error.");
            return false;
        }

        doc.SetString(PyBytes_AsString(utf8_item),
                      PyBytes_Size(utf8_item), root.GetAllocator());

        Py_XDECREF(utf8_item);
#else
        doc.SetString(PyBytes_AsString(object), PyBytes_GET_SIZE(object));
#endif
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;

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
#ifdef PY3
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            PyErr_SetString(PyExc_RuntimeError, "codec error.");
            return false;
        }

        doc.SetString(PyBytes_AsString(utf8_item),
                      PyBytes_Size(utf8_item), doc.GetAllocator());

        Py_XDECREF(utf8_item);
#else
        doc.SetString(PyBytes_AsString(object), PyBytes_GET_SIZE(object));
#endif
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;
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
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::string s = buffer.GetString();

    return PyString_FromStringAndSize(s.data(), s.length());
}

static void
pyobj2pystring_filestream(PyObject *pyjson, FILE *fp)
{
    rapidjson::Document doc;
    char writeBuffer[64*1024];

    if (false == pyobj2doc(pyjson, doc)) {
        return;
    }

    rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
    doc.Accept(writer);

    return;
}

static PyObject *
pyrapidjson_loads(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {(char *)"text", NULL};
    char *text;
    PyObject *pyjson;
    rapidjson::Document doc;
    int is_float;
    unsigned int offset;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &text))
        return NULL;

    doc.Parse(text);

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
pyrapidjson_load(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {(char *)"text", NULL};
    PyObject *py_file;
    rapidjson::Document doc;
    char readBuffer[64*1024];
    int fd0, fd1;
    FILE *fp;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &py_file))
        return NULL;
    fd0 = PyObject_AsFileDescriptor(py_file);
    if (fd0 == -1) {
        PyErr_SetString(PyExc_RuntimeError, "open file error");
        return NULL;
    }
    if (!_PyVerify_fd(fd0)) {
        PyErr_SetString(PyExc_RuntimeError, "open file error");
        return NULL;
    }
    fd1 = dup(fd0);
    fp = fdopen(fd1, "rb");
    if (!fp) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    doc.ParseStream(is);
    fclose(fp);

    return doc2pyobj(doc);
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
    PyObject *py_file, *pyjson;
    rapidjson::Document doc;
    int fd0, fd1;
    FILE *fp;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", kwlist, &pyjson, &py_file))
        return NULL;
    fd0 = PyObject_AsFileDescriptor(py_file);
    if (fd0 == -1) {
        PyErr_SetString(PyExc_RuntimeError, "open file error");
        return NULL;
    }
    if (!_PyVerify_fd(fd0)) {
        PyErr_SetString(PyExc_RuntimeError, "open file error");
        return NULL;
    }

    fd1 = dup(fd0);
    fp = fdopen(fd1, "wb");
    if (!fp) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }

    pyobj2pystring_filestream(pyjson, fp);

    fclose(fp);

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
