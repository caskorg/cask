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

def ShouldDownload(group, name, rows, cols, nonZeros):
    print rows
    if Rows and int(rows.replace(',', '')) not in Rows:
        return False
    return True

class MyHtmlParser(HTMLParser):

    def __init__(self):
        HTMLParser.__init__(self)
        self.state = 'NONE'
        self.skipped_header = False
        self.value_fields = []
        self.downloaded_matrices = 0

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
        matriId = self.value_fields[2]
        rows = self.value_fields[6]
        cols = self.value_fields[7]
        nonZeros = self.value_fields[8]

        print self.value_fields
        
        if ShouldDownload(group, name, rows, cols, nonZeros):

            url = 'http://www.cise.ufl.edu/research/sparse/MM/' + group + '/' + name + '.tar.gz'
            print 'Fetching matrix: ' + group + ' ' + name
            filename = wget.download(url)
            print 'Done'
            self.downloaded_matrices += 1
            if self.downloaded_matrices == MATRIX_LIMIT:
                self.state = 'FINISHED'



def main():
    # if len(sys.argv) != 2:
    #     print "Usage: python get_matrices.py <html_file>"
    #     return

    # print 'Fetching matrix list...'
    # url = 'http://www.cise.ufl.edu/research/sparse/matrices/list_by_nnz.html'
    # filename = wget.download(url)
    # print 'Done'


    filename = 'list_by_nnz.html'
    f = open(filename)

    parser = MyHtmlParser()
    parser.feed(f.read())


if __name__ == '__main__':
    main()
