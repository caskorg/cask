test:
	nosetests --with-coverage --cover-html --cover-package=sparsegrind

test-pdb:
	nosetests --pdb

.PHONY: test
