test:
	py.test --cov=sparsegrind

test-pdb:
	nosetests --pdb

.PHONY: test
