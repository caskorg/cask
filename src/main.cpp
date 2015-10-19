#include <Spark/Dse.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <string>
#include <chrono>
#include <sstream>

using namespace std;
namespace pt = boost::property_tree;

spark::dse::DseParameters loadParams(const boost::filesystem::path& parf) {
  std::cout << "Using " << parf << " as param file" << std::endl;
  pt::ptree tree;
  pt::read_json(parf.filename().string(), tree);
  spark::dse::DseParameters dsep;
  dsep.numPipesRange =
    spark::utils::Range{
      tree.get<int>("dse_params.num_pipes.start"),
      tree.get<int>("dse_params.num_pipes.stop"),
      tree.get<int>("dse_params.num_pipes.step"),
    };
  dsep.cacheSizeRange =
    spark::utils::Range {
      tree.get<int>("dse_params.cache_size.start"),
      tree.get<int>("dse_params.cache_size.stop"),
      tree.get<int>("dse_params.cache_size.step"),
    };
  dsep.inputWidthRange =
    spark::utils::Range {
      tree.get<int>("dse_params.input_width.start"),
      tree.get<int>("dse_params.input_width.stop"),
      tree.get<int>("dse_params.input_width.step"),
    };
  return dsep;
}

spark::dse::Benchmark loadBenchmark(const boost::filesystem::path& p) {
  using namespace boost::filesystem;
  std::cout << "Using " << p << " as benchmark directory" << std::endl;
  spark::dse::Benchmark benchmark{};
  for (directory_iterator end, it = directory_iterator(p); it != end; it++) {
    benchmark.add_matrix_path(it->path().string());
  }
  return benchmark;
}

void write_dse_results(
    const std::vector<std::shared_ptr<spark::spmv::SpmvArchitecture>>& results,
    double took
    ) {
  pt::ptree tree, children;
  stringstream ss;
  auto end = std::chrono::system_clock::now();
  auto end_time = std::chrono::system_clock::to_time_t(end);
  ss << std::ctime(&end_time);
  tree.put("date", ss.str());
  tree.put("took", took);
  for (const auto& arch : results) {
    std::string arch_name = arch->get_name();
    pt::ptree archJson;
    archJson.put("name", arch_name);
    archJson.put("estimated_gflops", arch->getEstimatedGFlops());
    archJson.add_child("architecture_params", arch->write_params());
    children.push_back(std::make_pair("", archJson));
  }
  tree.add_child("best_architectures", children);
  pt::write_json("test_out.json", tree);
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

  spark::dse::DseParameters params = loadParams(parf);
  params.gflopsOnly = true;
  std::cout << params << std::endl;
  spark::dse::Benchmark benchmark = loadBenchmark(dirp);
  std::cout << benchmark << std::endl;
  spark::dse::SparkDse dseTool;

  auto start = std::chrono::high_resolution_clock::now();
  auto results = dseTool.run(
      benchmark,
      params);
  auto diff = dfesnippets::timing::clock_diff(start);
  write_dse_results(results, diff);

  // Executables exes = buildTool.buildExecutables(Hardware Designs)
  // PerfResults results = perfTool.runDesigns(exes)
  // results.print()

}
