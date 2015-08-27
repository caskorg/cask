import random

def print_header():
   print '%%MatrixMarket matrix coordinate real general'

def main():
  rowLength = 64
  cols = 64

  print_header()
  print rowLength, cols, rowLength * cols

  for i in range(1, rowLength + 1):
    for j in range(1, cols + 1):
      value = random.random()
      print i, j, value


if __name__ == '__main__':
  main()

