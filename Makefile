TMP_PATH="/tmp/cask_html_doc"
HTML_DIR="html"
COV_DIR="coverage"
MAT_DIR="matrices_html"
TMP_COV_PATH="/tmp/cask_coverage"
TMP_MAT_PATH="/tmp/cask_matrices"
TRASH=latex
ROOT_PATH=$(shell pwd)

CMAKE=cmake3
ifeq (, $(shell which cmake3))
	CMAKE=cmake
endif

help:
	@ echo "Available targets are"
	@ echo "To run the entire flow on a benchmark:"
	@ echo "  mock-flow    -- flow for mock designs"
	@ echo "  sim-flow     -- flow for simulation designs"
	@ echo ""
	@ echo "To run the tests"
	@ echo "  sim-test      -- run simulation tests"
	@ echo "  "
	@ echo "  coverage      -- run tests (sim, unit, mock) and coverage checks"
	@ echo "  "
	@ echo "  upd-doc       -- push doc"
	@ echo "  upd-coverage  -- push updated coverage information"

# make this depend on source files?
build/main:
	rm -rf build
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make main -j

hw: build/main
	cd src/frontend && python cask.py -t dfe -p ../params.json -b ../../test/test-benchmark -d -bm best

sim: build/main
	cd src/frontend && python cask.py -t sim -p ../params.json -b ../../test/test-benchmark -d -rb -rep html
	cd build & make -j12

sim-test: sim
	cd build && ctest -R sim*

graphs:
	cd src/frontend && PYTHONPATH=$(PYTHONPATH):$(ROOT_PATH)/sparsegrind python render_graphs.py

hw-flow:
	mkdir -p build
	cd build && $(CMAKE) -DBUILD_SIM=true -DBUILD_HW=true ..
	make -C build Eigen3
	make -C build
	cd build && ctest -E Client*

sim-flow:
	mkdir -p build
	cd build && $(CMAKE) -DBUILD_SIM=true -DBUILD_HW=false ..
	make -C build Eigen3
	make -C build
	cd build && ctest -E Client*

mock-flow:
	mkdir -p build
	cd build && $(CMAKE) -DCMAKE_CXX_COMPILER=$(CXX) -DBUILD_SIM=false -DBUILD_HW=false ..
	make -C build Eigen3
	make -C build -j8
	cd build && ctest -E Client*

matrix:
	# TODO must run build beforehand
	bash gen_matrix.sh
	rm -rf matrices_html && mkdir -p matrices_html
	python gen_matrix_html.py

coverage:
	rm -rf build && mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd src/frontend && python cask.py -t dfe_mock -p ../params.json -b ../../test/test-benchmark && cd ..
	cd src/frontend && python cask.py -t sim -p ../params.json -b ../../test/test-benchmark && cd ..
	cd build && make -j12
	bash gen_cov

clean-tags:
	rm -f tags

tags:
	ctags include/ src/ -R *

gtags:
	find src/runtime/ include/ test/ -type f -print | gtags -f -

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
