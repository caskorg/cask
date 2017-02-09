#ifndef USERINPUT_H

#define USERINPUT_H

#include <vector>
#include <boost/algorithm/string.hpp>
#include <unordered_map>

namespace cask {
  namespace userio {

    using UserInputParams = std::unordered_map<std::string, int>;

    inline UserInputParams parseBuildParams(std::string params) {
      std::string delimiters("= ");
      std::vector<std::string> parts;
      boost::split(parts, params, boost::is_any_of(delimiters));
      for (auto s : parts)
        std::cout << s << std::endl;
      // TODO need a more flexible approach
      // for now we assume we have -> param + int value pairs
      UserInputParams buildParams;

      bool param_name = true;
      std::cout << "parts size " << parts.size() << std::endl;
      for (int i = 0; i < parts.size(); i+=2) {
        buildParams.insert(std::make_pair(parts[i], std::stoi(parts[i + 1])));
      }

      return buildParams;
    }

  }
};




#endif /* end of include guard: USERINPUT_H */
