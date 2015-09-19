import random
import sys

def print_header():
   print '%%MatrixMarket matrix coordinate real general'

def main():
  order     = int(sys.argv[1])
  rowLength = order
  cols = order

  print_header()
  print rowLength, cols, rowLength * cols

  for i in range(1, rowLength + 1):
    for j in range(1, cols + 1):
      value = random.random()
      print i, j, value


if __name__ == '__main__':
  main()

