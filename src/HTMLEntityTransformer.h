#ifndef HTMLENTITYTRANSFORMER_H
#define HTMLENTITYTRANSFORMER_H

#include <string>
#include <map>
// #include <cstdint>
#include "stdint_portable.h"

class HTMLEntityTransformer {
private:
    std::map<uint32_t, std::string> entity_map;
    
    void append_utf8(std::string& output, uint32_t code_point);

public:
    HTMLEntityTransformer();
    std::string transform(const std::string& input);
};

#endif // HTMLENTITYTRANSFORMER_H