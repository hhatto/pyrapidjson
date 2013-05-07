pyrapidjson
===========
.. image:: https://secure.travis-ci.org/hhatto/pyrapidjson.png?branch=master
   :target: https://secure.travis-ci.org/hhatto/pyrapidjson
   :alt: Build status


About
-----
pyrapidjson is a wrapper for `rapidjson`_ (JSON parser/generator).

.. _`rapidjson`: http://code.google.com/p/rapidjson/


Installation
------------
from pip::

    $ pip install pyrapidjson

from easy_install::

    easy_install -ZU pyrapidjson


Requirements
------------
Python2.7+


Usage
-----

basic usage::

    >>> import rapidjson
    >>> rapidjson.loads('[1, 2, {"test": "hoge"}]')
    >>> [1, 2, {"test": "hoge"}]
    >>> rapidjson.dumps([1, 2, {"foo": "bar"}])
    '[1,2,{"foo":"bar"}]'
    >>>


Links
-----
* PyPI_
* GitHub_
* `Travis-CI`_

.. _PyPI: http://pypi.python.org/pypi/pyrapidjson/
.. _GitHub: https://github.com/hhatto/pyrapidjson
.. _`Travis-CI`: https://travis-ci.org/hhatto/pyrapidjson

