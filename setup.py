from distutils.core import setup, Extension


setup(
    name='pyrapidjson',
    version='0.5',
    description='Python Interface for rapidjson(JSON parser and generator).',
    long_description=open('README.rst').read(),
    license='Expat License',
    author='Hideo Hattori',
    author_email='hhatto.jp@gmail.com',
    url='https://github.com/hhatto/pyrapidjson',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
    ],
    keywords='json rapidjson',
    ext_modules=[
        Extension('rapidjson',
                  sources=['pyrapidjson/_pyrapidjson.cpp'],
                  include_dirs=['./pyrapidjson/rapidjson/include/'],
                  #extra_compile_args=["-DDEBUG"],
                  )]
)
