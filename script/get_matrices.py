import sys
import wget

from HTMLParser import HTMLParser

# Max number of matrices to fetch
MATRIX_LIMIT=1000

# What groups to fetch matrices from
MATRIX_GROUPS = []

# What matrices to fetch
MATRIX_NAMES = []

# The range of legal non-zero values
NonZeros = []

# The range of legal rows
Rows = [16, 32, 64, 128, 256, 512]

# The range of legal cols
Cols = [16, 32, 64, 128, 256, 512]

def ToInt(stringVal):
    return int(stringVal.replace(',', ''))

def ShouldDownload(group, name, rows, cols, nonZeros):
    if Rows and ToInt(rows) not in Rows:
        return False
    if Cols and ToInt(cols) not in Cols:
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
        if tag == 'table':
            self.state ='FINISHED'
        elif tag == 'td':
            self.state = 'PARSING_ENTRY'
        elif tag == 'tr':
            self.state = 'PARSING_TABLE'
            self.handle_matrix_entry()
            self.value_fields = []


    def handle_data(self, data):
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

        if ShouldDownload(group, name, rows, cols, nonZeros):
            url = 'http://www.cise.ufl.edu/research/sparse/MM/' + group + '/' + name + '.tar.gz'
            print 'Fetching matrix: ' + group + ' ' + name
            filename = wget.download(url)
            self.downloaded_matrices += 1
            self.matrices.append((group, name, matrixId, rows, cols, nonZeros))
            if self.downloaded_matrices == MATRIX_LIMIT:
                self.state = 'FINISHED'

    def GetDownloadedMatrices(self):
        return self.matrices


def main():
    # if len(sys.argv) != 2:
    #     print "Usage: python get_matrices.py <html_file>"
    #     return

    print 'Fetching matrix list...'
    url = 'http://www.cise.ufl.edu/research/sparse/matrices/list_by_nnz.html'
    filename = wget.download(url)
    print 'Done'

    f = open(filename)

    parser = MyHtmlParser()
    parser.feed(f.read())

    matrices = parser.GetDownloadedMatrices()

    print 'Fetched: '
    print matrices


if __name__ == '__main__':
    main()
