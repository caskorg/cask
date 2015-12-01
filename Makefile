TMP_PATH="/tmp/spark_html_doc"
HTML_DIR="html"
COV_DIR="coverage"
MAT_DIR="matrices_html"
TMP_COV_PATH="/tmp/spark_coverage"
TMP_MAT_PATH="/tmp/spark_matrices"
TRASH=latex

help:
	@ echo "Available targets are"
	@ echo "  sim-test      -- run simulation tests"
	@ echo "  "
	@ echo "  mock          -- build mock designs"
	@ echo "  "
	@ echo "  coverage      -- run tests (sim, unit, mock) and coverage checks"
	@ echo "  "
	@ echo "  upd-doc       -- push doc"
	@ echo "  upd-coverage  -- push updated coverage information"

sim:
	rm -rf build
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
	cd scripts && python spark.py -t sim -p ../params.json -b ../test-benchmark

sim-test: sim
	cd build && ctest -R sim*

mock:
	mkdir -p build
	cd build && cmake ..
	cd scripts && python spark.py -t dfe_mock -p ../params.json -b ../test-benchmark && cd ..
	cd build && make -j12

matrix:
	# TODO must run build beforehand
	bash gen_matrix.sh
	rm -rf matrices_html && mkdir -p matrices_html
	python gen_matrix_html.py

coverage:
	rm -rf build && mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd scripts && python spark.py -t dfe_mock -p ../params.json -b ../test-benchmark && cd ..
	cd scripts && python spark.py -t sim -p ../params.json -b ../test-benchmark && cd ..
	cd build && make -j12
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

upd-coverage: coverage
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

upd-matrix: matrix
	rm -rf ${TMP_MAT_PATH}
	cp ${MAT_DIR} ${TMP_MAT_PATH} -R && rm -rf ${MAT_DIR}
	git fetch
	git checkout gh-pages
	mkdir -p matrices
	cp ${TMP_MAT_PATH}/* matrices/ -R
	git add matrices/
	git commit -m "Update matrix data"
	git push -u origin gh-pages
	rm -rf ${TMP_MAT_PATH}
	git checkout master

.PHONY: doc update-doc tags
