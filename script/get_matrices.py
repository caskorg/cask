import os
import shutil
import sys
import wget
import argparse

from collections import namedtuple
from subprocess import call
from HTMLParser import HTMLParser

Matrix=namedtuple('Matrix', ['group', 'name', 'id', 'rows', 'cols', 'nonZeros', 'file', 'valuetype'])

benchmarks=[
    ('matrix-vector', 'armadillo.cpp', 'g++ armadillo.cpp -o armadillo -O1 -larmadillo', 'armadillo')]

# Max number of matrices to fetch
MATRIX_LIMIT=460

# What groups to fetch matrices from
MATRIX_GROUPS = []

# What matrices to fetch
MATRIX_NAMES = []

# Max row and col sizes to fetch. Set to None if no limit is required
MAX_NON_ZEROS = 1E9

ValueType = 'real' # 'real', 'integer', 'complex', 'binary'

# The range of legal non-zero values
NonZeros = []

# The range of legal rows
Rows = []

# The range of legal cols
Cols = xrange(10000, 100000)

# Legal values for symmetry
Sym = []

# Legal values for positive definite
Spd = []

class MatrixFetcher(object):
    def __init__(self):
        pass

def ToInt(stringVal):
    return int(stringVal.replace(',', ''))


class MyHtmlParser(HTMLParser):

    def ShouldDownload(self, group, name, rows, cols, nonZeros, spd, sym, valuetype):
        if self.matrix_names and not name in self.matrix_names:
            return False
        if valuetype != ValueType:
            return False
        if Rows and rows not in Rows:
            return False
        if Cols and cols not in Cols:
            return False
        if Spd and not spd in Spd:
            return False
        if Sym and not sym in Sym:
            return False
        if MAX_NON_ZEROS and nonZeros > MAX_NON_ZEROS:
            return False
        return True

    def __init__(self, args):
        HTMLParser.__init__(self)
        self.state = 'NONE'
        self.skipped_header = False
        self.value_fields = []
        self.downloaded_matrices = 0
        self.matrices = []
        self.dryrun = args.dryrun
        self.matrix_names = self.GetMatrixList(args.matrix_list_file)
        print self.matrix_names

    def GetMatrixList(self, matrix_list_file):
        f = open(matrix_list_file)
        res = []
        for l in f.readlines():
            res.append(l.strip())
        f.close()
        return res

    def handle_starttag(self, tag, attrs):
        if self.state == 'FINISHED':
            return
        if tag == 'table':
            self.state = 'PARSING_TABLE'
        elif tag == 'td':
            self.state ='PARSING_VALUE'
        elif tag == 'tr':
            if self.skipped_header:
                self.state = 'PARSING_ENTRY'
            else:
                self.skipped_header = True

    def handle_endtag(self, tag):
        if self.state == 'FINISHED':
            return
        if tag == 'table':
            self.state ='FINISHED'
        elif tag == 'td':
            self.state = 'PARSING_ENTRY'
        elif tag == 'tr':
            self.state = 'PARSING_TABLE'
            self.handle_matrix_entry()
            self.value_fields = []

    def handle_data(self, data):
        if self.state == 'FINISHED':
            return
        if self.state == 'PARSING_VALUE':
            data = data.strip()
            if "/,\n".find(data) == -1:
                self.value_fields.append(data)

    def handle_matrix_entry(self):

        fields = self.value_fields
        group = fields[0]
        name = fields[1]
        matrixId = fields[2]
        rows = ToInt(fields[6])
        cols = ToInt(fields[7])
        nonZeros = ToInt(fields[8])
        valuetype = fields[9].split()[0]
        spd = fields[10]
        sym = fields[11]

        if self.ShouldDownload(group, name, rows, cols, nonZeros, spd, sym, valuetype):
            url = 'http://www.cise.ufl.edu/research/sparse/MM/' + group + '/' + name + '.tar.gz'

            print 'Fetching matrix: ' + group + ' ' + name, valuetype

            filename = None
            if not self.dryrun:
                filename = wget.download(url)
                print '... Done'
            self.downloaded_matrices += 1
            self.matrices.append(Matrix(group, name, matrixId,
                                        rows, cols, nonZeros, filename,
                                        valuetype
                                    ))
            if self.downloaded_matrices >= MATRIX_LIMIT:
                self.state = 'FINISHED'

        if group == 'Springer':
            print group, name, valuetype

    def GetDownloadedMatrices(self):
        return self.matrices


def RunBenchmark(matrix):
    pass


def main():
    parser = argparse.ArgumentParser(
        description='Download sparse matrices from UoF Collection.')
    parser.add_argument('-n', '--dryrun',
                        action='store_true',
                        default=False,
                        help='Print matrices that would be downloaded.')
    parser.add_argument('-f', '--matrix-list-file',
                        help='File containing a list of matrices to download.')
    args = parser.parse_args()

    print 'Fetching matrix list...'
    url = 'http://www.cise.ufl.edu/research/sparse/matrices/list_by_nnz.html'
    filename = wget.download(url)
    print 'Done'

    filename = 'list_by_nnz.html'
    f = open(filename)

    parser = MyHtmlParser(args)
    parser.feed(f.read())

    matrices = parser.GetDownloadedMatrices()

    # print matrices
    print 'Fetched {} matrices: '.format(len(matrices))

    # prepare matrices (move to directory, extract archive...)
    shutil.rmtree('matrices', True)
    os.mkdir('matrices')

    for matrix in matrices:
        if not matrix.file:
            continue
        print matrix.file
        shutil.move(matrix.file, 'matrices')
        call(['tar','-xvzf', 'matrices/' + matrix.file, '-C', 'matrices/'])
        RunBenchmark(matrix)

if __name__ == '__main__':
    main()
