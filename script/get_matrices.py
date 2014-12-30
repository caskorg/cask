import os
import shutil
import wget
import argparse

from collections import namedtuple
from subprocess import call
from HTMLParser import HTMLParser


class Matrix(object):

    def __init__(self, group, name, id, rows, cols,
                 nonZeros, valuetype, spd, sym):
        self.group = group
        self.group = group
        self.name = name
        self.id = id
        self.rows = rows
        self.cols = cols
        self.nonZeros = nonZeros
        self.valuetype = valuetype
        self.spd = spd
        self.sym = sym
        self.filename = group + '/' + name + '.tar.gz'
        self.downloaded = False

    def __str__(self):
        return self.name + ' ' + self.group


# Max number of matrices to fetch
MATRIX_LIMIT = 460

# What groups to fetch matrices from
MATRIX_GROUPS = []

# What matrices to fetch
MATRIX_NAMES = []

# Max row and col sizes to fetch. Set to None if no limit is required
MAX_NON_ZEROS = 1E9

ValueType = 'real'  # 'real', 'integer', 'complex', 'binary'

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


def ToInt(stringVal):
    return int(stringVal.replace(',', ''))


class MyHtmlParser(HTMLParser):

    def ShouldDownload(self, matrix):
        if self.matrix_names and matrix.name not in self.matrix_names:
            return False
        if matrix.valuetype != ValueType:
            return False
        if Rows and matrix.rows not in Rows:
            return False
        if Cols and matrix.cols not in Cols:
            return False
        if Spd and matrix.spd not in Spd:
            return False
        if Sym and matrix.sym not in Sym:
            return False
        if MAX_NON_ZEROS and matrix.nonZeros > MAX_NON_ZEROS:
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
            self.state = 'PARSING_VALUE'
        elif tag == 'tr':
            if self.skipped_header:
                self.state = 'PARSING_ENTRY'
            else:
                self.skipped_header = True

    def handle_endtag(self, tag):
        if self.state == 'FINISHED':
            return
        if tag == 'table':
            self.state = 'FINISHED'
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

        m = Matrix(group=fields[0],
                   name=fields[1],
                   id=fields[2],
                   rows=ToInt(fields[6]),
                   cols=ToInt(fields[7]),
                   nonZeros=ToInt(fields[8]),
                   valuetype=fields[9].split()[0],
                   spd=fields[10],
                   sym=fields[11])

        if self.ShouldDownload(m):
            url = 'http://www.cise.ufl.edu/research/sparse/MM/' + m.filename
            if not self.dryrun:
                wget.download(url)
                print '... Done'
                m.downloaded = True
            self.downloaded_matrices += 1
            self.matrices.append(m)
            if self.downloaded_matrices >= MATRIX_LIMIT:
                self.state = 'FINISHED'

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
    parser.add_argument('-v', '--verbose',
                        action='store_true',
                        default=False,
                        help='Print the list of downloaded matrices.')
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
    if args.verbose:
        for m in matrices:
            print m

    # prepare matrices (move to directory, extract archive...)
    shutil.rmtree('matrices', True)
    os.mkdir('matrices')

    for matrix in matrices:
        if not matrix.downloaded:
            continue
        print matrix.filename
        shutil.move(matrix.filename, 'matrices')
        call(['tar', '-xvzf', 'matrices/' + matrix.file, '-C', 'matrices/'])
        RunBenchmark(matrix)

if __name__ == '__main__':
    main()
