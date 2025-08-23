#ifndef HTMLENTITYTRANSFORMER_H
#define HTMLENTITYTRANSFORMER_H

#include <cstdint>
#include <string>
#include <unordered_map>

class HTMLEntityTransformer {
private:
  std::unordered_map<uint32_t, std::string> entity_map;

  void append_utf8(std::string &output, uint32_t code_point);

public:
  HTMLEntityTransformer();
  std::string transform(const std::string &input);
};

#endif // HTMLENTITYTRANSFORMER_H
