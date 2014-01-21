import sys

from HTMLParser import HTMLParser

class MyHtmlParser(HTMLParser):

    def __init__(self):
        HTMLParser.__init__(self)
				self.state = 'NONE'

    def handle_starttag(self, tag, attrs):
        if self.state == 'FINISHED':
            return

        if tag == '<table>':
            self.state = 'PARSING_TABLE'
            print tag
        elif tag == '<td>':
            self.state ='PARSING_VALUE'
        elif tag == '<tr>':
            if skipped_header:
                self.state = 'PARSING_ENTRY'


    def handle_endtag(self, tag):
        if tag == '<table>':
            self.state ='FINISHED'
        elif tag == '<td>':
            self.state = 'PARSING_ENTRY'
        elif tag == '<tr>':
            self.state = 'PARSING_TABLE'

    def handle_data(self, data):
        if self.state == 'PARSING_VALUE':
            print data


def main():
    if len(sys.argv) != 2:
        print "Usage: python get_matrices.py <html_file>"
        return

    f = open(sys.argv[1])

    state = 'NONE'
    entry_count = 0


    text = ""
    max_lines = 100
    c = 0
    for line in f:
        text = text + '\n' + line
        if c > max_lines:
            break
        c += 1

    parser = MyHtmlParser()
    parser.feed('<table>bau</table>')


if __name__ == '__main__':
    main()
