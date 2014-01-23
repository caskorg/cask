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
MATRIX_LIMIT=100

# What groups to fetch matrices from
MATRIX_GROUPS = []

# What matrices to fetch
MATRIX_NAMES = []

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
    if Rows and ToInt(rows) not in Rows:
        return False
    if Cols and ToInt(cols) not in Cols:
        return False
    if Spd and not spd in Spd:
        return False
    if Sym and not sym in Sym:
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

        group = self.value_fields[0]
        name = self.value_fields[1]
        matrixId = self.value_fields[2]
        rows = self.value_fields[6]
        cols = self.value_fields[7]
        nonZeros = self.value_fields[8]
        spd = self.value_fields[10]
        sym = self.value_fields[11]

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
