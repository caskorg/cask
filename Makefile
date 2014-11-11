test:
	nosetests --with-cov --cov-report html

test-pdb:
	nosetests --pdb

.PHONY: test
