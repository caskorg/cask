import random
import sys

def print_header():
   print '%%MatrixMarket matrix coordinate real general'

def main():
  if len(sys.argv) < 3:
    print 'usage: generate_matrices rows cols elemsPerRow'
    sys.exit(1)

  rows = int(sys.argv[1])
  cols = int(sys.argv[2])
  elemsPerRow = int(sys.argv[3])

  print_header()
  print rows, cols, rows * elemsPerRow

  for i in range(1, rows + 1):
    for j in range(1, elemsPerRow + 1):
      value = random.random()
      print i, j, value


if __name__ == '__main__':
  main()

