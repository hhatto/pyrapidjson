install:
	make clean
	easy_install -ZU .

.PHONY: test
test:
	python test/testall.py

clean:
	rm -rf build *.egg-info dist temp
	rm -rf tests/*.pyc

pypireg:
	python setup.py register
	python setup.py sdist upload
