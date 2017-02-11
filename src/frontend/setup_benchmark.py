"""This script wil set up benchmark of sparse matrices in the current
working directory. The structure of the benchmarks is:
benchmark_data/
  Matrix.mtx
  Matrix_b.mtx
  Matrix_sol.mtx

The system matrix and RHS are extracted from UoF. The expected solution is taken
either from:
- a provided solutions.tar.gz archive of previously computed expected solutions
- re-computed using the sparse linear algebra submodule in scipy

If present, the file uof_matrix_names.txt can be used to refetech the benchmark:
python setup_benchmark.py -f uof_matrix_names.txt

This script will perform caching: if all required

The list of required matrices can be specified by command line, or by
modifying the selection criteria in this script.

Note! By default only matrices with a right hand side are
downloaded. The list of matrices is given in uof_rhs_names, and used
by fetch.py. This list may come out of the date, but seems the
simplest mechanism to keep track of RHS matrices for the moment. An
alternative would be to download the matrix and verify, but this means
we cannot first select a matrix, and then donwload it which slows the
interactive process a bit.
"""
import fetch
import os.path
import json
from tabulate import tabulate
from subprocess import call

import argparse
import urllib
import utils

SOLUTIONS_URL = 'https://www.doc.ic.ac.uk/~pg1709/solutions.tar.gz'

def matrix_name(matrix_file):
    return matrix_file.replace('_SPD_x.mtx', '')


def add_solutions(collection, solutionArchive='solutions.tar.gz', url=SOLUTIONS_URL):
    if not os.path.exists(solutionArchive):
        utils.warn('Could not find expected solutions archive!')
        utils.info('Attempting to download solutions from: ' + url)
        found = urllib.urlopen(url)
        if found.code != 200:
            utils.error('Solution download failed: ' + url)
            return
        urllib.urlretrieve(url, solutionArchive)
    call(["tar", "-xzf", solutionArchive])
    for f in os.listdir("solutions"):
        mname = matrix_name(f)
        for m in collection.matrixList:
            if m.name == mname:
                m.hasSol = True
                m.solFile = "solutions/" + f
    


def make_benchmark(collection):
    """Group all systems and solutions in a single directory"""
    # Check if we have all required information for the benchmark
    not_cached = collection.select(lambda x: not x.solFile or not x.file or not x.rhsFile)
    # import pdb;pdb.set_trace()
    if not_cached.matrixList:
        # Download the matrices which are not cached
        not_cached.download()
    else:
        utils.info('All systems are cached, will not re-fetch')

    # create benchmark directory
    benchmark_dir = 'benchmark_data'
    if not os.path.exists(benchmark_dir):
        os.mkdir(benchmark_dir)
    for m in collection.matrixList:
        mat_name = m.name + ".mtx"
        rhs_name = m.name + "_b.mtx"
        sol_name = m.name + "_sol.mtx"
        mat_path = os.path.join(benchmark_dir, mat_name)
        rhs_path = os.path.join(benchmark_dir, rhs_name)
        sol_path = os.path.join(benchmark_dir, sol_name)
        if not os.path.exists(mat_path):
            call(['cp', m.file, mat_path])
        if not os.path.exists(rhs_path):
            call(['cp', m.rhsFile, rhs_path])
        if not os.path.exists(sol_path):
            call(['cp', m.solFile, sol_path])
        m.file = mat_path
        m.rhsFile = rhs_path
        m.solFile = sol_path


def run_benchmark(collection):
    """Run each solver on a system and record the results"""
    solvers = ['../../build/test_cg_mock']
    headers = ['estimated error', 'solve took', 'setup took',
               'error', 'iterations', 'bench repetitions']
    all_data = []
    for s in solvers:
        if not os.path.exists(s):
            print 'Error! Missing solver', s
            continue
        for m in collection.matrixList:
            print 'Solving', m.file, 'with solver', s
            utils.execute([s, '-mat', m.file, '-rhs', m.rhsFile, '-lhs', m.solFile])
            for prec in ['ilu', 'upc']:
                with open('sol.' + prec + '.mtx.log') as f:
                    json_data = json.load(f)
                    all_data.append([json_data[h] for h in headers] + [m.name, 'CG:' + prec])
    headers.extend(['matrix', 'solver'])
    print tabulate(all_data, headers)


def main():
    parser = argparse.ArgumentParser(description='Run Spark DSE flow')
    parser.add_argument('-n', '--firstN', type=int, default=3)
    args = parser.parse_args()
    # Download all SPD matrices if not already present

    # get matrix list and print it

    # if no matrices specified, use the default criteria
    systems = fetch.fetch().getSpdLinearSystems()

    # add solution information
    add_solutions(systems)

    systems_without_solutions = systems.select(lambda x: not x.hasSol)
    if systems_without_solutions:
        utils.warn('Some systems do not have solutions')
        print systems_without_solutions

    systems_with_solutions = systems.select(lambda x: x.hasSol)

    if not systems_with_solutions.matrixList:
        utils.warn('No system has an expected solution')
        make_benchmark(systems_without_solutions)
        run_benchmark(systems_without_solutions.head(args.firstN))
        return

    make_benchmark(systems_with_solutions)
    run_benchmark(systems_with_solutions.head(args.firstN))

if __name__ == '__main__':
    main()
