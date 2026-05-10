.PHONY: help configure build test asan tsan coverage tidy format check regression bench bench-quick bench-pgo clean

help:  ## Show this help
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-12s\033[0m %s\n", $$1, $$2}'

configure:  ## Configure debug build
	cmake --preset debug

build: configure  ## Build (debug)
	cmake --build --preset debug

test: build  ## Run unit tests (debug)
	ctest --preset debug

regression: build  ## Run bit-exact MPI regression
	python3 tests/run_mpi_regression.py --executable build/debug/apps/ccsd_code

asan:  ## ASan + UBSan
	cmake --preset asan && cmake --build --preset asan && ctest --preset asan

tsan:  ## TSan
	cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan

coverage:  ## Coverage build + gcovr
	cmake --preset coverage && cmake --build --preset coverage && ctest --preset coverage
	gcovr --root . --filter 'src/' --txt --print-summary --object-directory build/coverage || true

tidy:  ## clang-tidy on staged-able sources
	clang-tidy -p build/debug $$(git ls-files '*.cpp' '*.hpp' '*.h' | grep -v '^build')

format:  ## clang-format in place
	clang-format -i $$(git ls-files '*.cpp' '*.hpp' '*.h' | grep -v '^build')

check: format tidy test regression  ## Full quality suite

bench:  ## Run canonical bench matrix and refresh report
	bash scripts/run_bench.sh

bench-quick:  ## Smaller bench matrix for fast iteration
	BATCH=200 WARMUP=20 REPETITIONS=1 NP_LIST="2 4" THREADS_LIST="1" \
	    bash scripts/run_bench.sh

bench-pgo:  ## Two-stage PGO build + bench
	rm -rf build/pgo-data
	mkdir -p build/pgo-data
	cmake --preset release-fast-instrument
	cmake --build --preset release-fast-instrument --target ccsd_bench
	mpirun --oversubscribe -np 4 build/release-fast-instrument/ccsd_bench --batch 200 --warmup 20
	cmake --preset release-fast-pgo
	cmake --build --preset release-fast-pgo
	python3 tests/run_mpi_regression.py \
	    --executable build/release-fast-pgo/ccsd_code --tolerance 1e-9
	PRESETS="release-fast-pgo" NP_LIST="4" THREADS_LIST="1" \
	    BATCH=1000 WARMUP=50 REPETITIONS=3 \
	    bash scripts/run_bench.sh

clean:  ## Remove build directories
	rm -rf build/
