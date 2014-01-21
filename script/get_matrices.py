import sys

from HTMLParser import HTMLParser

class MyHtmlParser(HTMLParser):

    def __init__(self):
        HTMLParser.__init__(self)
        self.state = 'NONE'
        self.skipped_header = False
        self.value_fields = []

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
        print self.value_fields

def main():
    if len(sys.argv) != 2:
        print "Usage: python get_matrices.py <html_file>"
        return

    f = open(sys.argv[1])

    state = 'NONE'
    entry_count = 0


    text = ""
    max_lines = 2000
    c = 0
    for line in f:
        text = text + '\n' + line
        if c > max_lines:
            break
        c += 1


    parser = MyHtmlParser()
    parser.feed(text)


if __name__ == '__main__':
    main()
