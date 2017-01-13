# coding: utf-8
import sys
import os
import json
import unittest
from tempfile import NamedTemporaryFile
import rapidjson

if sys.version_info < (3, ):
    try:
        from io import BytesIO as StringIO
    except ImportError:
        from StringIO import StringIO
else:
    from io import StringIO


class TestDecodeSimple(unittest.TestCase):

    def test_integer(self):
        text = "12"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, 12)

    def test_negative_integer(self):
        text = "-12"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, -12)

    def test_float(self):
        text = "12.3"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, 12.3)

    def test_negative_float(self):
        text = "-12.3"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, -12.3)

    def test_string(self):
        text = "\"hello world\""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, "hello world")

    def test_true(self):
        text = "true"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, True)

    def test_false(self):
        text = "false"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, False)

    def test_none(self):
        text = "null"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, None)

    def test_list_size_one(self):
        text = "[null]"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, [None])

    def test_list_size_two(self):
        text = "[false, -50.3]"
        ret = rapidjson.loads(text)
        self.assertEqual(ret, [False, -50.3])

    def test_dict_size_one(self):
        text = """{"20":null}"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, {'20': None})

    def test_dict_size_two(self):
        text = """{"hoge":null, "huga":134}"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, {"hoge": None, "huga": 134})


class TestDecodeComplex(unittest.TestCase):

    def test_list_in_list(self):
        text = """["test", [1, "hello"]]"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, ["test", [1, "hello"]])

    def test_list_in_dict(self):
        text = """{"test": [1, "hello"]}"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, {"test": [1, "hello"]})

    def test_dict_in_dict(self):
        text = """{"test": {"hello": "world"}}"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, {"test": {"hello": "world"}})


class TestDecodeFail(unittest.TestCase):

    def test_use_single_quote_for_string(self):
        text = "'foo'"
        self.assertRaises(ValueError, rapidjson.loads, text)


class TestEncodeSimple(unittest.TestCase):

    def test_integer(self):
        jsonobj = 1
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "1")

    def test_negative_integer(self):
        jsonobj = -100
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "-100")

    def test_float(self):
        jsonobj = 12.3
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "12.3")

    def test_negative_float(self):
        jsonobj = -12.3
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "-12.3")

    def test_string(self):
        jsonobj = "hello world"
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "\"hello world\"")

    def test_true(self):
        jsonobj = True
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "true")

    def test_false(self):
        jsonobj = False
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "false")

    def test_none(self):
        jsonobj = None
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "null")

    def test_list_size_one(self):
        jsonobj = [None, ]
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "[null]")

    def test_list_size_two(self):
        jsonobj = [False, -50.3]
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, "[false,-50.3]")

    def test_dict_size_one(self):
        jsonobj = {'20': None}
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, """{"20":null}""")

    def test_dict_size_two(self):
        jsonobj = {"hoge": None, "huga": 134}
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(""""hoge":null""" in ret, True)
        self.assertEqual(""""huga":134""" in ret, True)

    def test_dict_not_string_key(self):
        invalid_jsonobj = {1: 1}
        ret = rapidjson.dumps(invalid_jsonobj)
        self.assertEqual(ret, """{"1":1}""")

    def test_dict_not_string_key_long(self):
        invalid_jsonobj = {429496729501234567: 1}
        ret = rapidjson.dumps(invalid_jsonobj)
        self.assertEqual(ret, """{"429496729501234567":1}""")

    def test_dict_not_string_key_complex(self):
        invalid_jsonobj = {-1.99: 1}
        ret = rapidjson.dumps(invalid_jsonobj)
        self.assertEqual(ret, """{"-1.99":1}""")


class TestEncodeComplex(unittest.TestCase):

    def test_list_in_list(self):
        jsonobj = [-123, [1, "hello"]]
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, """[-123,[1,"hello"]]""")

    def test_list_in_dict(self):
        jsonobj = {"test": [1, "hello"]}
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, """{"test":[1,"hello"]}""")

    def test_dict_in_dict(self):
        jsonobj = {"test": {"hello": "world"}}
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, """{"test":{"hello":"world"}}""")

    def test_dict_in_dict_and_list(self):
        jsonobj = {"test": {"hello": ["world", "!!"]}}
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, """{"test":{"hello":["world","!!"]}}""")


class TestFileStream(unittest.TestCase):

    def test_dump(self):
        jsonobj = {"test": [1, "hello"]}
        fp = NamedTemporaryFile(mode='w', delete=False)
        rapidjson.dump(jsonobj, fp)
        fp.close()
        check_fp = open(fp.name)
        ret = json.load(check_fp)
        self.assertEqual(jsonobj, ret)
        check_fp.close()
        os.remove(fp.name)

    def test_dump_with_utf8(self):
        jsonobj = {"test": [1, "こんにちは"]}
        fp = NamedTemporaryFile(mode='w', delete=False)
        rapidjson.dump(jsonobj, fp)
        fp.close()
        check_fp = open(fp.name)
        ret = json.load(check_fp)
        self.assertEqual(jsonobj[u"test"][0], ret[u"test"][0])
        check_fp.close()
        os.remove(fp.name)

    def test_dump_with_invalid_fp(self):
        jsonobj = {"test": [1, "hello"]}
        fp = NamedTemporaryFile(mode='w', delete=False)
        fp.close()
        self.assertRaises(ValueError, rapidjson.dump, jsonobj, fp)
        os.remove(fp.name)

    def test_dump_with_io_stringio(self):
        jsonobj = {"test": [1, "hello"]}
        stream = StringIO()
        rapidjson.dump(jsonobj, stream)
        stream.seek(0)
        self.assertEqual("{\"test\":[1,\"hello\"]}", stream.read())

    def test_dump_with_invalid_arg(self):
        jsonobj = {"test": [1, "hello"]}
        self.assertRaises(TypeError, rapidjson.dump, jsonobj, "")

    def test_load(self):
        jsonstr = b"""{"test": [1, "hello"]}"""
        fp = NamedTemporaryFile(delete=False)
        fp.write(jsonstr)
        fp.close()
        check_fp = open(fp.name)
        retobj = rapidjson.load(check_fp)
        self.assertEqual(retobj["test"], [1, "hello"])
        # teardown
        check_fp.close()
        os.remove(fp.name)

    def test_load_simple(self):
        jsonstr = """1"""
        stream = StringIO()
        stream.write(jsonstr)
        stream.seek(0)
        retobj = rapidjson.load(stream)
        self.assertEqual(retobj, 1)

    def test_load_with_utf8(self):
        jsonstr = """{"test": [1, "こんにちは"]}"""
        if sys.version_info >= (3, ):
            fp = NamedTemporaryFile(mode='w', delete=False, encoding='utf-8')
        else:
            fp = NamedTemporaryFile(mode='w', delete=False)
        fp.write(jsonstr)
        fp.close()
        check_fp = open(fp.name)
        retobj = rapidjson.load(check_fp)
        self.assertEqual(retobj["test"], [1, "こんにちは"])
        # teardown
        check_fp.close()
        os.remove(fp.name)

    def test_load_with_io_stringio(self):
        jsonstr = """{"test": [1, "hello"]}"""
        stream = StringIO()
        stream.write(jsonstr)
        stream.seek(0)
        retobj = rapidjson.load(stream)
        self.assertEqual(retobj["test"], [1, "hello"])

    def test_load_with_invalid_fp(self):
        jsonstr = b"""{"test": [1, "hello"]}"""
        fp = NamedTemporaryFile(delete=False)
        fp.write(jsonstr)
        fp.close()
        check_fp = open(fp.name)
        check_fp.close()
        self.assertRaises(ValueError, rapidjson.load, check_fp)
        # teardown
        os.remove(fp.name)

    def test_load_with_invalid_arg(self):
        self.assertRaises(TypeError, rapidjson.load, "")
