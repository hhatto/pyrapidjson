import rapidjson

jsonstr = """\
[
    "name",
    [1, 3, true],
    {"windows": "XP", "linux": null}
]"""
print(rapidjson.loads(jsonstr))

print("=" * 30)
jsonstr = """\
{
    "name" : "Jack",
    "bar" : false,
    "foo" : true,
    "age" : 20,
    "length" : 65.97,
    "pc" : {"windows": "XP", "linux": null}
}"""
print(rapidjson.loads(jsonstr))

print("=" * 30)
print(rapidjson.loads("true"))

print("=" * 30)
print(rapidjson.loads("null"))

print("=" * 30)
print(rapidjson.loads("2.14"))
