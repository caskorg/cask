import os
import shutil
import sys
import wget

from collections import namedtuple
from subprocess import call
from HTMLParser import HTMLParser

Matrix=namedtuple('Matrix', ['group', 'name', 'id', 'rows', 'cols', 'nonZeros', 'file'])

benchmarks=[
    ('matrix-vector', 'armadillo.cpp', 'g++ armadillo.cpp -o armadillo -O1 -larmadillo', 'armadillo')]

# Max number of matrices to fetch
MATRIX_LIMIT=1000

# What groups to fetch matrices from
MATRIX_GROUPS = []

# What matrices to fetch
MATRIX_NAMES = []

# Max row and col sizes to fetch. Set to None if no limit is required
MAX_ROW_SIZE = None
MAX_COL_SIZE = None
MAX_NON_ZEROS = 200000

# The range of legal non-zero values
NonZeros = []

# The range of legal rows
Rows = []

# The range of legal cols
Cols = []

# Legal values for symmetry
Sym = ['yes']

# Legal values for positive definite
Spd = ['yes']

def ToInt(stringVal):
    return int(stringVal.replace(',', ''))

def ShouldDownload(group, name, rows, cols, nonZeros, spd, sym):
    if Rows and rows not in Rows:
        return False
    if Cols and cols not in Cols:
        return False
    if Spd and not spd in Spd:
        return False
    if Sym and not sym in Sym:
        return False
    if MAX_COL_SIZE and cols > MAX_COL_SIZE:
        return False
    if MAX_ROW_SIZE and rows > MAX_ROW_SIZE:
        return False
    if MAX_NON_ZEROS and nonZeros > MAX_NON_ZEROS:
        return False
    return True

class MyHtmlParser(HTMLParser):

    def __init__(self):
        HTMLParser.__init__(self)
        self.state = 'NONE'
        self.skipped_header = False
        self.value_fields = []
        self.downloaded_matrices = 0
        self.matrices = []

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
        spd = fields[10]
        sym = fields[11]

        if ShouldDownload(group, name, rows, cols, nonZeros, spd, sym):
            url = 'http://www.cise.ufl.edu/research/sparse/MM/' + group + '/' + name + '.tar.gz'

            print 'Fetching matrix: ' + group + ' ' + name
            filename = wget.download(url)
            print '... Done'
            self.downloaded_matrices += 1
            self.matrices.append(Matrix(group, name, matrixId, rows, cols, nonZeros, filename))
            if self.downloaded_matrices >= MATRIX_LIMIT:
                self.state = 'FINISHED'

    def GetDownloadedMatrices(self):
        return self.matrices

def RunBenchmark(matrix):
    pass

def main():
    # if len(sys.argv) != 2:
    #     print "Usage: python get_matrices.py <html_file>"
    #     return

    print 'Fetching matrix list...'
    url = 'http://www.cise.ufl.edu/research/sparse/matrices/list_by_nnz.html'
    filename = wget.download(url)
    print 'Done'

    filename = 'list_by_nnz.html'
    f = open(filename)

    parser = MyHtmlParser()
    parser.feed(f.read())

    matrices = parser.GetDownloadedMatrices()

    print 'Fetched {} matrices: '.format(len(matrices))
    print matrices

    # prepare matrices (move to directory, extract archive...)
    shutil.rmtree('matrices', True)
    os.mkdir('matrices')

    for matrix in matrices:
        print matrix.file
        shutil.move(matrix.file, 'matrices')
        call(['tar','-xvzf', 'matrices/' + matrix.file, '-C', 'matrices/'])
        RunBenchmark(matrix)

if __name__ == '__main__':
    main()
