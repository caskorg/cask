#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

using namespace std;

// a benchmark to use for the DSE
class Benchmark {
  public:
    Benchmark() {
    }
};


// the parameters and ranges to use for DSE
class DseParameters {
  public:
    DseParameters() {
    }
};


DseParameters loadParams(const boost::filesystem::path& parf) {
  std::cout << "Using " << parf << " as param file" << std::endl;
  return DseParameters{};
}


Benchmark loadBenchmark(const boost::filesystem::path& directory) {
  // include all matrices in directory in the benchmark
  std::cout << "Using " << directory << " as benchmark directory" << std::endl;
  // TODO build benchmark from directory
  return Benchmark{};
}


int main(int argc, char** argv) {

  namespace po = boost::program_options;

  std::string opt_dse_params = "dse-params-file";
  std::string opt_bench_path = "bench-path";

  // options to display in the help message
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "Print this help message");

  // required options, not displayed in help message
  string benchPath, dseparams;
  po::options_description required_options("Required arguments");
  required_options.add_options()
    (opt_bench_path.c_str(),
     po::value<string>(&benchPath),
     "Path to the directory containing benchmarks"
    )
    (opt_dse_params.c_str(),
     po::value<string>(&dseparams),
     "Path to the file containing dse parameters");
  po::positional_options_description p;
  p.add(opt_bench_path.c_str(), 1);
  p.add(opt_dse_params.c_str(), 1);

  // all options
  po::options_description cmdline_options;
  cmdline_options.add(desc).add(required_options);

  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv).
      options(cmdline_options).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << "Usage: ./main bench-path dse-params-file [options]" << endl << endl;
    std::cout << required_options << std::endl;
    cout << desc << endl;
    return 0;
  }

  // validate command line args
  namespace bfs = boost::filesystem;
  bfs::path dirp{benchPath};
  if (!bfs::is_directory(dirp)) {
    std::cout << "Error: '" << benchPath << "' not a directory" << std::endl;
    return 1;
  }

  bfs::path parf{dseparams};
  if (!bfs::is_regular_file(parf)) {
    std::cout << "Error: '" << dseparams << "' is not a file" << std::endl;
    return 1;
  }

  DseParameters params = loadParams(parf);
  Benchmark benchmark = loadBenchmark(dirp);
  // HardwareDesigns = dseTool.runDse(benchmark);
  // Executables exes = buildTool.buildExecutables(Hardware Designs)
  // PerfResults results = perfTool.runDesigns(exes)
  // results.print()

}
