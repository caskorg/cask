test:
	nosetests --with-cov --cov-report html

test-pdb:
	nosetests --pdb

clean:
	rm -rf htmlcov *.tmp
	find . -name "*.pyc" -exec rm {} \;

.PHONY: test
