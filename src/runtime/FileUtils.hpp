#ifndef FILEUTILS_H

#define FILEUTILS_H

#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <sstream>

namespace spark {
  namespace file_utils {
    std::vector<std::string> child_files(std::string  dirPath) {
      namespace bfs = boost::filesystem;
      bfs::path dirp{dirPath};
      if (!bfs::is_directory(dirp)) {
        std::stringstream ss;
        ss << "Error: '" << dirPath << "' not a directory" << std::endl;
        throw std::invalid_argument(ss.str());
      }

      std::vector<std::string> paths;
      for (bfs::directory_iterator end, it = bfs::directory_iterator(dirp); it != end; it++) {
        paths.push_back(it->path().string());
      }
      return paths;
    }
  }
}


#endif /* end of include guard: FILEUTILS_H */
