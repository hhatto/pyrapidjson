#include <Python.h>
#include <string.h>
#include <cstdio>
#include <sstream>

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

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

static void pyobj2doc(PyObject *object, rapidjson::Document& doc);
static void pyobj2doc(PyObject *object, rapidjson::Value& doc, rapidjson::Document& root);

static PyObject* doc2pyobj(rapidjson::Document&);
static PyObject* _get_pyobj_from_object(
        rapidjson::Value::ConstMemberIterator& doc, PyObject *root,
        const char *key);

/* The module doc strings */
PyDoc_STRVAR(pyrapidjson__doc__, "Python binding for rapidjson");
PyDoc_STRVAR(pyrapidjson_loads__doc__, "Decoding JSON");
PyDoc_STRVAR(pyrapidjson_dumps__doc__, "Encoding JSON");


struct _OutputStringStream : public std::ostringstream {
	typedef char Ch;

	void Put(char c) {
		put(c);
	}
};

static inline void
pyobj2doc_pair(PyObject *key, PyObject *value,
               rapidjson::Value& doc, rapidjson::Document& root)
{
    const char *key_string;
#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(key);
    key_string = PyBytes_AsString(utf8_item);
    Py_XDECREF(utf8_item);
#else
    key_string = PyString_AsString(key);
#endif
    rapidjson::Value _v;
    pyobj2doc(value, _v, root);
    doc.AddMember(key_string, _v, root.GetAllocator());
}
static inline void
pyobj2doc_pair(PyObject *key, PyObject *value, rapidjson::Document& doc)
{
    const char *key_string;
#ifdef PY3
    PyObject *utf8_item;
    utf8_item = PyUnicode_AsUTF8String(key);
    key_string = PyBytes_AsString(utf8_item);
    Py_XDECREF(utf8_item);
#else
    key_string = PyString_AsString(key);
#endif
    rapidjson::Value _v;
    pyobj2doc(value, _v, doc);
    doc.AddMember(key_string, _v, doc.GetAllocator());
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
            char _tmp[100] = "";
            sprintf(_tmp, "%lf", doc->GetDouble());
            obj = PyFloat_FromDouble(atof(_tmp));
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
            printf("%f\n", doc->value.GetDouble());
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

static void
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
        doc.SetInt(PyLong_AsLong(object));
    }
    else if (PyString_Check(object)) {
        doc.SetString(PyString_AsString(object), PyString_GET_SIZE(object));
    }
    else if (PyUnicode_Check(object)) {
#ifdef PY3
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            // TODO: error handling
            printf("error\n");
        }

        if (strlen(PyBytes_AsString(utf8_item)) == PyUnicode_GET_SIZE(object)) {
            doc.SetString(PyBytes_AsString(utf8_item), PyBytes_Size(utf8_item));
        }
        else {
            // TODO: not need?
            doc.SetString(PyBytes_AsString(object), PyString_GET_SIZE(object));
        }

        Py_XDECREF(utf8_item);
#else
        doc.SetString(PyBytes_AsString(object), PyBytes_GET_SIZE(object));
#endif
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;

        doc.SetArray();
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            rapidjson::Value _v;
            pyobj2doc(elm, _v, root);
            doc.PushBack(_v, root.GetAllocator());
        }
    }
    else if (PyList_Check(object)) {
        int len = PyList_Size(object), i;

        doc.SetArray();
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            rapidjson::Value _v;
            pyobj2doc(elm, _v, root);
            doc.PushBack(_v, root.GetAllocator());
        }
    }
    else if (PyDict_Check(object)) {
        doc.SetObject();
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(object, &pos, &key, &value)) {
            PyObject *_size = PyLong_FromSsize_t(pos);
            long i = PyLong_AsLong(_size);
            Py_XDECREF(_size);
            if (i == -1) break;

            pyobj2doc_pair(key, value, doc, root);
        }
    }
    else {
        // TODO: error handle
    }
}

static void
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
        doc.SetInt(PyLong_AsLong(object));
    }
    else if (PyString_Check(object)) {
        doc.SetString(PyString_AsString(object), PyString_GET_SIZE(object));
    }
    else if (PyUnicode_Check(object)) {
#ifdef PY3
        PyObject *utf8_item;
        utf8_item = PyUnicode_AsUTF8String(object);
        if (!utf8_item) {
            // TODO: error handling
            printf("error\n");
        }

        if (strlen(PyBytes_AsString(utf8_item)) == PyUnicode_GET_SIZE(object)) {
            doc.SetString(PyBytes_AsString(utf8_item), PyBytes_Size(utf8_item));
        }
        else {
            // TODO: not need?
            doc.SetString(PyBytes_AsString(object), PyString_GET_SIZE(object));
        }

        Py_XDECREF(utf8_item);
#else
        doc.SetString(PyBytes_AsString(object), PyBytes_GET_SIZE(object));
#endif
    }
    else if (PyTuple_Check(object)) {
        int len = PyTuple_Size(object), i;

        doc.SetArray();
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            rapidjson::Value _v;
            pyobj2doc(elm, _v, doc);
            doc.PushBack(_v, doc.GetAllocator());
        }
    }
    else if (PyList_Check(object)) {
        int len = PyList_Size(object), i;

        doc.SetArray();
        for (i = 0; i < len; ++i) {
            PyObject *elm = PyList_GetItem(object, i);
            rapidjson::Value _v;
            pyobj2doc(elm, _v, doc);
            doc.PushBack(_v, doc.GetAllocator());
        }
    }
    else if (PyDict_Check(object)) {
        doc.SetObject();
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(object, &pos, &key, &value)) {
            PyObject *_size = PyLong_FromSsize_t(pos);
            long i = PyLong_AsLong(_size);
            Py_XDECREF(_size);
            if (i == -1) break;

            pyobj2doc_pair(key, value, doc);
        }
    }
    else {
        // TODO: error handle
    }
}

static PyObject *
pyobj2pystring(PyObject *pyjson)
{
    rapidjson::Document doc;

    pyobj2doc(pyjson, doc);

    _OutputStringStream ss;
    rapidjson::Writer<_OutputStringStream> writer(ss);
    doc.Accept(writer);
    std::string s = ss.str();

    return PyString_FromStringAndSize(s.data(), s.length());
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

    doc.Parse<0>(text);

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
pyrapidjson_dumps(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {(char *)"obj", NULL};
    PyObject *pyjson;

    /* Parse arguments */
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &pyjson))
        return NULL;

    return pyobj2pystring(pyjson);
}


static PyMethodDef PyrapidjsonMethods[] = {
    {"loads", (PyCFunction)pyrapidjson_loads, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_loads__doc__},
    {"dumps", (PyCFunction)pyrapidjson_dumps, METH_VARARGS | METH_KEYWORDS,
     pyrapidjson_dumps__doc__},
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
