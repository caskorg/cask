TMP_PATH="/tmp/spark_html_doc"
HTML_DIR="html"
COV_DIR="coverage"
TMP_COV_PATH="tmp/spark_coverage"
TRASH=latex

help:
	@ echo "Available targets are"
	@ echo "  mock          -- build mock designs"
	@ echo "  mock-coverage -- run tests and coverage checks"
	@ echo "  doc           -- generate documentation"
	@ echo "  upd-doc       -- push doc"
	@ echo "  upd-coverage  -- push updated coverage information"

mock:
	cd scripts && python spark.py -t dfe_mock -p ../params.json -b ../test-benchmark && cd ..
	mkdir -p build
	cd build && cmake .. && make -j12

mock-coverage: build/unit_test_spmv
	bash gen_cov

clean-tags:
	rm -f tags

tags:
	ctags include/ src/ -R *

doc:
	doxygen docs/doxygen.conf

clean-all-dist:
	rm -rf src/spmv/build/Spmv* src/spmv/build/*.class

upd-doc: doc
	rm -rf ${TMP_PATH}
	cp ${HTML_DIR} ${TMP_PATH} -R && rm -rf ${HTML_DIR}
	git fetch
	git checkout gh-pages
	mkdir -p docs
	cp ${TMP_PATH}/* docs/ -R
	rm -rf ${TRASH}
	git add docs/
	git commit -m "Update documentation"
	git push -u origin gh-pages
	rm -rf ${TMP_PATH}
	git checkout master

upd-cov: mock-coverage
	rm -rf ${TMP_COV_PATH}
	cp ${COV_DIR} ${TMP_COV_PATH} -R && rm -rf ${COV_DIR}
	git fetch
	git checkout gh-pages
	mkdir -p coverage
	cp ${TMP_COV_PATH}/* coverage/ -R
	git add coverage/
	git commit -m "Update coverage"
	git push -u origin gh-pages
	rm -rf ${TMP_COV_PATH}
	git checkout master

.PHONY: doc update-doc tags
