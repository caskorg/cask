import spark

def main():
    p = spark.PrjConfig({'num_pipes': '0'}, 'sim',
                        'block-diagonal', 0,
                        '../src/designs/block-diagonal/build')
    b = spark.MaxBuildRunner()
    b.runBuilds([p])
    # How do we do this b.run()


if __name__ == '__main__':
  main()
