import json
import pandas as pd
import sys


def main():
  pd.set_option('display.width', 400)

  if len(sys.argv) != 2:
    print 'Usage ./', sys.argv[0], '<path to JSON output file>'
    exit(1)

  with open(sys.argv[1]) as f:
    result = f.read()
    pd_frame = pd.read_json(result)
    print pd_frame


if __name__ == '__main__':
  main()
