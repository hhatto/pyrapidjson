import unittest
import rapidjson


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

    def test_list_in_dict(self):
        text = """{"test": [1, "hello"]}"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, {"test": [1, "hello"]})

    def test_dict_in_dict(self):
        text = """{"test": {"hello": "world"}}"""
        ret = rapidjson.loads(text)
        self.assertEqual(ret, {"test": {"hello": "world"}})


class TestEncodeSimple(unittest.TestCase):

    def test_integer(self):
        jsonobj = 1
        ret = rapidjson.dumps(jsonobj)
        self.assertEqual(ret, 1)
