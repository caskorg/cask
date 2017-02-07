import os
import shutil
import wget
import argparse
from tabulate import tabulate
from subprocess import call
from HTMLParser import HTMLParser


class Matrix(object):
    def __init__(self, group, name, id, rows, cols, nonZeros, file, valuetype, spd, sym, hasRhs):
        self.group = group
        self.name = name
        self.id = id
        self.rows = rows
        self.cols = cols
        self.nonZeros = nonZeros
        self.file = file
        self.valuetype = valuetype
        self.spd = spd
        self.sym = sym
        self.hasRhs = hasRhs
        self.rhsFile = None
        self.hasSol = False
        self.solFile = None

    def fullName(self):
        return self.group + '/' + self.name

    def __str__(self):
        return "group={} name={} id={} rows={} cols={} nonZeros={} file={} valuetype={} spd={} sym={} hasRhs={} rhsFile={}".format(
            self.group, self.name, self.id, self.rows, self.cols,
            self.nonZeros, self.file, self.valuetype, self.spd, self.sym, self.hasRhs, self.rhsFile)

    def __repr__(self):
        return self.__str__()


class MatrixCollection(object):
    def __init__(self, matrixList):
        self.matrixList = matrixList

    def head(self, n):
        return MatrixCollection(self.matrixList[:n])

    def select(self, predicate):
        return MatrixCollection(filter(predicate, self.matrixList))

    def sorted(self, keyList, reverse=False):
        return MatrixCollection(
            sorted(self.matrixList,
                   key=lambda x: tuple([getattr(x, f) for f in keyList]),
                   reverse=not reverse))

    def download(self, dir='matrices'):
        shutil.rmtree(dir, True)
        os.mkdir(dir)
        for m in self.matrixList:
            print 'Fetching {}'.format(m.fullName())
            url = 'http://www.cise.ufl.edu/research/sparse/MM/' + m.fullName() + '.tar.gz'
            m.file = wget.download(url)
            print
            shutil.move(m.file, dir)
            print 'Extracting...'
            call(['tar', '-xvzf', os.path.join(dir, m.file), '-C', dir])
            m.file = os.path.abspath(os.path.join(dir, m.name, m.name + '.mtx'))
            if not os.path.exists(m.file):
                print 'Warning! unexpected name for matrix', m.name()
            if m.hasRhs:
                m.rhsFile = os.path.abspath(os.path.join(dir, m.name, m.name + '_b.mtx'))
                if not os.path.exists(m.rhsFile):
                    print 'Warning! unexpected name for RHS of system', m.name()
            print

    def each(self, function):
        pass

    def __str__(self):
        keys = ['group', 'name', 'id', 'rows', 'cols', 'nonZeros', 'valuetype', 'spd', 'sym', 'hasRhs', 'hasSol']
        return tabulate([[getattr(m, k) for k in keys] for m in self.matrixList], keys)

    def getSpdLinearSystems(self):
        return self.select(lambda x: x.hasRhs and x.sym == 'yes' and x.spd == 'yes')

    def __repr__(self):
        return self.__str__()


def ToInt(stringVal):
    return int(stringVal.replace(',', ''))


def fetchOtherProperties(matrices):
    # Rebuild RHS information, if possible
    with open('uof_rhs_names.txt') as f:
        names = set(map(lambda x: x.strip(), f.readlines()))
        for m in matrices:
            if m.fullName() in names:
                m.hasRhs = True

    # Rebuild file path information
    if os.path.exists('matrices'):
        for m in matrices:
            mpath = os.path.join('matrices', m.name, m.name + '.mtx')
            bpath = os.path.join('matrices', m.name, m.name + '_b.mtx')
            if os.path.exists(mpath):
                m.file = mpath
            if os.path.exists(bpath):
                m.rhsFile = bpath

    return matrices


class MyHtmlParser(HTMLParser):
    def __init__(self):
        HTMLParser.__init__(self)
        self.state = 'NONE'
        self.skipped_header = False
        self.value_fields = []
        self.matrices = []

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
        self.matrices.append(
            Matrix(
                group=fields[0],
                name=fields[1],
                id=fields[2],
                rows=ToInt(fields[6]),
                cols=ToInt(fields[7]),
                nonZeros=ToInt(fields[8]),
                file='',
                valuetype=fields[9].split()[0],
                sym=fields[10],
                spd=fields[11],
                hasRhs=None
            )
        )


def fetch(force=False):
    filename = 'list_by_nnz.html'
    if not os.path.isfile(filename) or force:
        print 'Fetching matrix list...'
        url = 'http://www.cise.ufl.edu/research/sparse/matrices/list_by_nnz.html'
        filename = wget.download(url)
        print 'Done'
    else:
        print '--> Using cached', filename, '; to force refetch, use the -f option'

    f = open(filename)
    parser = MyHtmlParser()
    parser.feed(f.read())
    return MatrixCollection(fetchOtherProperties(parser.matrices))


def main():
    parser = argparse.ArgumentParser(
        description='Download sparse matrices from UoF Collection.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-n', '--dryrun',
                        action='store_true',
                        default=False,
                        help='Print matrices that would be downloaded.')
    parser.add_argument(
        '-c', '--containing',
        help='Only download matrices containing provided pattern')
    parser.add_argument(
        '-g', '--group',
        help='Only download matrices in a group containing provided pattern')
    parser.add_argument(
        '-m', '--max-matrices',
        help='Maximum number of matrices to fetch',
        type=int,
        default=100)
    parser.add_argument(
        '-f', '--force',
        action='store_true',
        default=False,
        help='Force a refetch of the list_by_nnz.html file containing the index of all matrices')
    args = parser.parse_args()

    matrices = fetch(args.force).select(
        lambda x: (not args.containing or args.containing in x.name) and (not args.group or args.group in x.group))
    matrices = MatrixCollection(matrices.matrixList[:args.max_matrices])

    if args.dryrun:
        print matrices
        print 'Fetched {} matrices'.format(len(matrices))
        return

    matrices.download()


if __name__ == '__main__':
    main()
