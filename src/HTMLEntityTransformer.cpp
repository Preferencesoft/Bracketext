#include "HTMLEntityTransformer.h"
#include <utf8.h>
#include <vector>

HTMLEntityTransformer::HTMLEntityTransformer() {
  // Initialize HTML entity mapping
  entity_map = {
      {0x0020, "&nbsp;"},   // space
      {0x003C, "&lt;"},     // <
      {0x003E, "&gt;"},     // >
      {0x0026, "&amp;"},    // &
      {0x0022, "&quot;"},   // "
      {0x0027, "&apos;"},   // '
      {0x00A2, "&cent;"},   // ¢
      {0x00A3, "&pound;"},  // £
      {0x00A5, "&yen;"},    // ¥
      {0x20AC, "&euro;"},   // €
      {0x00A9, "&copy;"},   // ©
      {0x00AE, "&reg;"},    // ®
      {0x00D7, "&times;"},  // ×
      {0x00F7, "&divide;"}, // ÷
      {0x00B1, "&plusmn;"}, // ±
                            // Add more entities as needed
  };
}

std::string HTMLEntityTransformer::transform(const std::string &input) {
  std::string result;
  std::vector<uint32_t> utf32_buffer;

  // Convert UTF-8 to UTF-32 for easier processing
  utf8::utf8to32(input.begin(), input.end(), std::back_inserter(utf32_buffer));

  bool in_protected_zone = false;

  for (auto it = utf32_buffer.begin(); it != utf32_buffer.end(); ++it) {
    uint32_t code_point = *it;

    // Check for delimiters
    if (code_point == 0x0001) { // U+0001 (START)
      in_protected_zone = true;
      // append_utf8(result, code_point); // Keep the delimiter
      continue;
    } else if (code_point == 0x0004) { // U+0004 (END)
      in_protected_zone = false;
      // append_utf8(result, code_point); // Keep the delimiter
      continue;
    }

    // Process character based on zone
    if (in_protected_zone) {
      // In protected zone - keep original character
      append_utf8(result, code_point);
    } else {
      // Outside protected zone - replace with entity if available
      auto entity_it = entity_map.find(code_point);
      if (entity_it != entity_map.end()) {
        result += entity_it->second;
      } else {
        append_utf8(result, code_point);
      }
    }
  }

  return result;
}

void HTMLEntityTransformer::append_utf8(std::string &output,
                                        uint32_t code_point) {
  // Convert UTF-32 code point back to UTF-8
  char utf8_buffer[5] = {0}; // Maximum 4 bytes for UTF-8 + null terminator
  char *end = utf8::append(code_point, utf8_buffer);
  output.append(utf8_buffer, end - utf8_buffer);
}
