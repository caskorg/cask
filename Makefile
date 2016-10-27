rev=$(shell git rev-parse --short HEAD)

help:
	@echo "Available targes:"
	@echo "   test          -- runs all tests"
	@echo "   docs          -- build the documentation"
	@echo "   perf          -- runs performance benchmarks"
	@echo "   cov           -- runs tests and generates coverage reports"
	@echo "   prof          -- profile a large perf test using valgrind"
	@echo "   lint          -- run C++ style guide checks"
	@echo "   cppcheck      -- check for possible bugs with cppcheck"
	@echo "Build params:"
	@echo "   Code revision = $(rev)"
	@echo "   CC            = $(CC)"
	@echo "   CXX           = $(CXX)"

perf:
	@echo "Warning! Performance benchmarks not yet implemented"

test:
	make -C build test

cov: debug_build clean_cov
	lcov -d . -z
	lcov -d . -i -c --output base.info
# Run all unit & integration tests
	make test
	lcov -d . -c --output-file run.info
	lcov -a base.info -a run.info -o total.info
# Extract patterns for spam
	lcov -e total.info '*spam*' -o spam.info
	lcov -r spam.info '*gtest*' -o spam_final.info
	genhtml spam_final.info --output-directory coverage

perf_build:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4

debug_build:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j4

clean: clean_cov
	rm -rf build

clean_cov:
	rm -f *.info
	rm -rf coverage

clean_perf:
	rm -f callgrind.*

profile: clean_perf perf_build
	@echo "Warning! Performance benchmarks not yet implemented"
	valgrind --tool=callgrind ./build/TestLinearSolvers
	kcachegrind

profile_debug: clean_perf debug_build
	@echo "Warning! Performance benchmarks not yet implemented"
	make profile

gprofile: clean_perf debug_build
	@echo "Warning! Performance benchmarks not yet implemented"
	LD_PRELOAD=/usr/local/lib/libprofiler.so:/usr/local/lib/libpapi.so CPUPROFILE=sie.prof CPUPROFILE_FREQUENCY=1000 ./build/TestLinearSolvers
	pprof --text ./build/TestLinearSolvers sie.prof

docs:
	cd docs && doxygen doxy.conf

lint:
	python lib/styleguide/cpplint/cpplint.py \
    --filter=-readability/todo,-readability/namespace \
    --linelength=120 \
    --extensions=hpp,cpp \
    src/*.cpp include/*.hpp test/*.cpp test/*.hpp

format:
	clang-format -style=Google -i src/*.cpp src/*.hpp test/*.cpp test/*.hpp include/*.hpp

cppcheck:
	cppcheck --enable=all src/*.cpp src/.hpp include/*.hpp test/*.cpp test/*.hpp

.PHONY: build/Makefile test perf cov perf_build docs
