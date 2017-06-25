install:
	make clean
	easy_install -ZU .

.PHONY: test
test:
	python test/testall.py

clean:
	rm -rf build *.egg-info dist temp
	rm -rf test/*.pyc

pypireg:
	python setup.py register
	python setup.py sdist upload

setup:
	git submodule init
	git submodule update
