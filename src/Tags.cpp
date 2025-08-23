// Bracketext.cpp
//

#include "Tags.h"

using namespace std;

/*
 * tagNumber/tag
    None -1
    String -2
    OpenBracket -3
    ClosedBracket -4
    StraightLine -5

*/

const int nString = -1; // string in parameters
const int nResult = -2; // during a conversion, certain character strings should
                        // only be converted once
const int nOpenBracket = -3;
const int nOpenedBracket = -3;
const int nClosedBracket = -4;
const int nStraightLine = -5;
const int nVerticalLine = -5;
const int nParameterBlocks = -7; // contains a complete tag
const int nArguments = -8;       // contains a complete tag
const int nGroup =
    -9; // groups together successions of tags and strings (without [ | ])
const int nTag = -6;    // tag
const int nMATag = -11; // tag argument to complete
const int nNone = -12;
// we do not define a tag when the parameters are not complete

std::vector<std::vector<std::string>> Tags::tagInfoList;
std::vector<std::string> Tags::commandList;
std::vector<std::string> Tags::functionNameList;
std::vector<std::string> Tags::scriptList;
std::vector<std::vector<int>> Tags::tagAssociationList;
std::vector<std::string> Tags::tagList;
std::vector<int> Tags::tagEntryList;
std::vector<int> Tags::tagTypeList;
std::vector<int> Tags::tagPositionList;
std::vector<Tags::Entity> Tags::document;

/*
struct Entity
{
    public int tagNumber;
    public string str; //nul when the entity is a tag
    // public List<Parameter> parameterList;
    public vector<Tags::Entity> entityList;
}
*/

// Tag association table
// 0 undefined
// 1 SINGLE (macro or function)
// 2 BEGIN_END
// 3 BEGIN_MIDDLE_END (deleted)
// 4 BEGIN_REPEATED_MIDDLE_END
// 5 BEGIN_REPEATED_AT_LEAST_ONCE_MIDDLE_END
// (not yet implemented)
// 6 MULTIPLE_REPEATED_MIDDLE_BLOCKS

const int TUndefined = 0;
const int TSingle = 1;
const int TBeginEnd = 2;
const int TBeginMiddleEnd = 3;
const int TBeginRepeatedMiddleEnd = 4;

const int TMultipleRepeatedMiddleBlocks = 6;

// Tag types in the same association.
// "/" Beginning
// "1", ..., "9" from 1 to 9 intermediate tag number
// only "1" and "2" are currently being used
// "." end

const std::string Tags::TB = "/";
const std::string Tags::TE = ".";

/*
 * List that was originally used for testing.
 *
List<string[]> tagInfoList = new List<string[]>
{
    new string[]{ "1", "Hello"},
    new string[]{ "2", "title", "/title"},
    new string[]{ "2", "h1", "/h1"},
    new string[]{ "2", "h2","/h2"},
    new string[]{ "2", "h3","/h3"},
    new string[]{ "2", "h4","/h4"},
    new string[]{ "2", "h5","/h5"},
    new string[]{ "2", "h6","/h6"},
    new string[]{ "2", "b","/b"},
    new string[]{ "2", "i","/i"},
    new string[]{ "2", "","/"},
    new string[]{ "2", "s","/s"},
    new string[]{ "2", "size","/size"},
    new string[]{ "2", "style","/style"},
    new string[]{ "2", "color","/color"},
    new string[]{ "2", "center", "/center"},
    new string[]{ "2", "left","/left"},
    new string[]{ "2", "right","/right"},
    new string[]{ "2", "quote","/quote"},
    new string[]{ "2", "ur","/ur"},
    new string[]{ "2", "img","/img"},
    new string[]{ "2", "u","/u"},
    new string[]{ "2", "o","/o"},
    new string[]{ "2", "li","/li"},
    new string[]{ "2", "code","/code"},
    new string[]{ "2", "table","/table"},
    new string[]{ "2", "tr","/tr"},
    new string[]{ "2", "th","/th"},
    new string[]{ "2", "td","/td"},
    new string[]{ "2", "youtube", "/youtube"},
    new string[]{ "3", "li","-", "/li"},
    new string[]{ "4", "list","*", "/list"},
};
*/
std::string Tags::SubStr(std::string &s, int n) {
  return (s.size() >= n ? s.substr(0, n) : "");
  /*throw Ice::IllegalConversionException(__FILE__, __LINE__, "bad encoding
   * because ...");*/
}

void remove_carriage_return(std::string &str) {
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}

std::string Tags::readFile(std::string &file_name) {
  std::ifstream file(file_name, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + file_name);
  }

  // Read file content
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  // Validate UTF-8
  if (!utf8::is_valid(content.begin(), content.end())) {
    throw std::runtime_error("Invalid UTF-8 in file: " + file_name);
  }
  // cout << content << endl;
  return content;
}

std::vector<std::string> Tags::split(const std::string &s, char delimiter) {
  std::vector<std::string> result;
  if (s.empty())
    return result;

  std::string::const_iterator start = s.begin();
  std::string::const_iterator end = s.end();
  std::string::const_iterator it = start;
  std::string current_part;
  current_part.reserve(16); // Preallocate typical word size

  while (it != end) {
    char32_t code_point;
    try {
      code_point = utf8::next(it, end);

      if (code_point == static_cast<char32_t>(delimiter)) {
        if (!current_part.empty()) {
          result.push_back(std::move(current_part));
          current_part.clear();
          current_part.reserve(16); // Re-reserve after clear
        }
      } else {
        utf8::append(code_point, std::back_inserter(current_part));
      }
    } catch (const utf8::exception &) {
      // Skip invalid UTF-8 sequences
      ++it;
    }
  }

  if (!current_part.empty()) {
    result.push_back(std::move(current_part));
  }

  return result;
}

std::string Tags::Trim(std::string &str) {
  if (str.empty())
    return "";

  // Find the first non-whitespace character from the beginning
  auto start = std::find_if(str.begin(), str.end(), [](char c) {
    return !std::isspace(c); // Use isspace for a wider range of whitespace
  });

  if (start == str.end())
    return ""; // String is entirely whitespace

  // Find the last non-whitespace character from the end
  auto end = std::find_if(str.rbegin(), str.rend(), [](char c) {
               return !std::isspace(c);
             }).base(); // Convert reverse iterator back to normal iterator

  // Handle the case where the string is entirely whitespace after trimming
  if (start == end) {
    return "";
  }

  return std::string(start, end);
}

// Function to extract UTF-8 strings between delimiters
vector<string> extractStrings(const string &input) {
  // Check for empty input
  if (input.empty()) {
    return vector<string>();
  }

  // Find the first pipe character.  Return empty if not found.
  size_t startPos = input.find('|');
  if (startPos == string::npos) {
    return vector<string>(); // Empty list if no pipes found
  }

  // Handle the leading ||||||
  if (input.substr(3, 6) != "||||||") {
    return vector<string>(); // Invalid format
  }
  return Tags::split(input.substr(9), '|');
}

std::string getFunctionName(const std::string &line) {
  size_t startPos = line.find("function");
  if (startPos == std::string::npos) {
    return "";
  }

  startPos += 8; // Move past "function"

  // Find the opening parenthesis.  Crucially, use find_first_not_of to avoid
  // issues with whitespace.
  size_t endPos = line.find_first_of('(', startPos);

  if (endPos == std::string::npos) {
    return "";
  }

  // Extract the function name
  size_t nameStart = startPos;
  size_t nameEnd = endPos;

  // Trim whitespace from the beginning of the name.  Important!
  while (nameStart < nameEnd && std::isspace(line[nameStart])) {
    ++nameStart;
  }

  // Trim whitespace from the end of the name.  Important!
  while (nameEnd > nameStart && std::isspace(line[nameEnd - 1])) {
    --nameEnd;
  }

  if (nameStart >= nameEnd) {
    return ""; // Empty name after trimming
  }

  std::string name = line.substr(nameStart, nameEnd - nameStart);

  // Crucial: Check if the extracted name contains invalid UTF-8 sequences.
  if (!utf8::is_valid(name.begin(), name.end())) {
    return ""; // Invalid UTF-8
  }

  return name;
}

void Tags::LoadMacros(std::string psFileName) {
  // provisionally, we load the entire text.
  vector<std::string> text = Tags::split(readFile(psFileName), '\n');

  int state = 0;
  std::stringstream sb;
  // 0 no function
  // 1 header
  // 2 inside the function
  for (int i = 0; i < text.size(); i++) {
    if (state == 0) {
      if (Trim(text[i]) != "-- <<<<<<")
        continue;
      state++;
    } else {
      if (state == 1) {
        // beginning of a function
        std::string header = Trim(text[i]);
        // cout << header;
        std::vector<string> entry = extractStrings(header);
        bool isGlobal = false;
        if (entry.size() > 0) {
          if (entry[0] == "g")
            isGlobal = true;
        }
        if (isGlobal) {
          sb.clear();
          state = 4;
          // pass 2 and 3
        } else {
          tagInfoList.push_back(entry);
          state++;
          // goto 2
        }
      } else {
        if (state == 2) {
          auto fun = Trim(text[i]);
          if (SubStr(fun, 9) != "function ")
            continue;
          fun = fun.substr(9);
          int ind = fun.find_first_of('(');
          fun = fun.erase(ind);

          functionNameList.push_back(fun);
          sb.clear();
          sb << Trim(text[i]) << endl;
          state++;
        } else {
          if (state == 3) {
            if (Trim(text[i]) != "-- >>>>>>") {
              sb << Trim(text[i]) << endl;
              continue;
            } else {
              Tags::commandList.push_back(sb.str());
              state = 0;
            }
          } else {
            if (state == 4) {
              if (Trim(text[i]) != "-- >>>>>>") {
                sb << Trim(text[i]) << endl;
                continue;
              } else {
                Tags::scriptList.push_back(sb.str());
                state = 0;
              }
            }
          }
        }
      }
    }
  }
  /*
  for (auto s : scriptList) {
     cout << s << endl;
  }
  for (auto f : functionNameList) {
     cout << f << endl;
  }
  for (auto tagInfo : tagInfoList) {
  for (auto t : tagInfo) {
    cout << t << " + ";
  }
  cout << endl;
  }
*/
}

bool utf8_compare(const std::string &str1, const std::string &str2) {
  auto it1 = str1.begin();
  auto it2 = str2.begin();

  while (it1 != str1.end() && it2 != str2.end()) {
    uint32_t cp1 = utf8::next(it1, str1.end()); // Decode next codepoint
    uint32_t cp2 = utf8::next(it2, str2.end());

    if (cp1 != cp2) {
      return cp1 < cp2; // Compare codepoints
    }
  }

  // If one string is a prefix of the other, the shorter one comes first
  return str1.length() < str2.length();
}

bool CompareString(Tags::info i1, Tags::info i2) {
  return utf8_compare(i1.sep, i2.sep);
}

void Tags::Init() {
  vector<info> infoList;
  int sepListCount = Tags::tagInfoList.size();
  // tagAssociationList = new int[sepListCount][];
  // tagTypeList = new int[sepListCount];
  for (int n = 0; n < sepListCount; n++) {
    vector<std::string> sep = Tags::tagInfoList[n];
    int len = sep.size();
    for (int i = 1; i < len; i++) {
      infoList.push_back(Tags::info{sep[i], n, i});
      //.Add(System.Tuple.Create<string, int, int>(sep[i], n, i));
    }
  }
  // sort by string
  sort(infoList.begin(), infoList.end(), CompareString);

  /*
             for (auto info : infoList) {
                cout << " { " << info.sep << " - " << info.entry << " - " <<
     info.pos << " } ";
             }
 */
  int tupleListCount = infoList.size();
  // tagList = new string[tupleListCount];
  // tagEntryList = new int[tupleListCount];
  // tagPositionList = new int[tupleListCount];

  for (int i = 0; i < tupleListCount; i++) {
    auto t = infoList[0];
    Tags::tagList.push_back(t.sep);
    Tags::tagEntryList.push_back(t.entry);
    Tags::tagPositionList.push_back(t.pos);
    infoList.erase(infoList.begin());
  }
  /*
              for (auto info : tagList) {
                 // cout << " { " << info << " } ";
                 cout << info << " , ";
              }
  */
  /*
  std::string t = "list";
  int tNumber = binary_search_utf8(tagList, t);
  cout << tNumber << " + " << tagList[tNumber];
*/
  for (int n = 0; n < sepListCount; n++) {
    vector<std::string> separ = Tags::tagInfoList[n];
    int len = separ.size();
    tagAssociationList.push_back(std::vector<int>(len - 1));
    for (int i = 1; i < len; i++) {
      auto lower =
          std::lower_bound(tagList.begin(), tagList.end(), separ[i], utf8_less);
      int index = std::distance(tagList.begin(), lower);
      tagAssociationList[n][i - 1] = index;
    }
    int e = std::stoi(separ[0]);
    tagTypeList.push_back(e);
  }
  // display tagList
  // for (auto s : tagList) {
  //    cout << s << " , " << endl;
  // }
}

bool Tags::IsNotDelim(char c) {
  switch (c) {
  case '[':
  case ']':
  case '|':
    return false;
  }
  return true;
}

bool Tags::IsNotSymbol(char c) {
  switch (c) {
  case '[':
  case ']':
  case '|':
  case '\\':
    return false;
  }
  return true;
}

int Tags::CharToTagNumber(char c) {
  switch (c) {
  case '[':
    return nOpenBracket;
  case ']':
    return nClosedBracket;
  case '|':
    return nStraightLine;
  }
  return nNone;
}

std::string Tags::TagNumberToString(int t) {
  switch (t) {
  case nOpenBracket:
    return "[";
  case nClosedBracket:
    return "]";
  case nStraightLine:
    return "|";
  case nString:
    return "s";
  case nTag:
    return "t";
  case nGroup:
    return "g";
  case nParameterBlocks:
    return "p";
  case nArguments:
    return "a";
  }
  return " ";
}

// Delete CR characters
// but actually does nothing.
std::string Tags::CleanTag(std::string &t) { return t; }

bool Tags::utf8_less(const std::string &a, const std::string &b) {
  auto it_a = a.begin();
  auto it_b = b.begin();

  while (it_a != a.end() && it_b != b.end()) {
    uint32_t cp_a = utf8::next(it_a, a.end()); // Decode next codepoint
    uint32_t cp_b = utf8::next(it_b, b.end());

    if (cp_a != cp_b) {
      return cp_a < cp_b; // Compare codepoints
    }
  }
  // Shorter strings come first
  return a.size() < b.size();
}

int Tags::binary_search_utf8(const std::vector<std::string> &list,
                             const std::string &key) {
  auto it = std::lower_bound(list.begin(), list.end(), key, utf8_less);

  if (it != list.end() && !utf8_less(key, *it) && !utf8_less(*it, key)) {
    return std::distance(list.begin(), it); // Return index if found
  }
  return -1; // Return -1 if not found
}

// Checks if a Unicode codepoint is whitespace (including tabs, newlines, etc.)
bool Tags::is_unicode_whitespace(uint32_t cp) {
  return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f' ||
         cp == '\v' ||
         // Additional Unicode whitespace can be added here if needed
         false;
}

// Trims whitespace from the right side of a UTF-8 string
std::string Tags::utf8_rtrim(const std::string &utf8_str) {
  if (utf8_str.empty())
    return utf8_str;

  auto it = utf8_str.begin();
  auto end_it = utf8_str.end();
  auto last_valid_pos = end_it;

  while (it != end_it) {
    auto prev_it = it;
    try {
      uint32_t cp = utf8::next(it, end_it);
      if (!is_unicode_whitespace(cp)) {
        last_valid_pos = it; // Last non-whitespace
      }
    } catch (const utf8::not_enough_room &) {
      // Skip invalid UTF-8 (treat as non-whitespace)
      last_valid_pos = end_it;
      break;
    }
  }

  return std::string(utf8_str.begin(), last_valid_pos);
}

void Tags::check_utf8(const std::string &s) {
  std::cout << "String: '" << s << "'\nHex: ";
  for (char c : s) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << (int)(unsigned char)c << " ";
  }
  std::cout << "\n---\n";
}

std::vector<Tags::Entity> extract(const std::vector<Tags::Entity> tokens, int i,
                                  int j) {
  std::vector<Tags::Entity> sublist;
  if (i >= 0 && j < tokens.size() && i <= j) {
    sublist.reserve(j - i + 1);
    std::copy(tokens.begin() + i, tokens.begin() + j + 1,
              std::back_inserter(sublist));
  }
  return sublist;
}

bool Tags::is_associated_intermediate(int index, int next_index) {
  int entry = tagEntryList[index];
  vector<int> asso = tagAssociationList[entry];
  int last = asso.size() - 1;
  return asso[last - 1] == next_index;
}

bool Tags::is_associated_last(int index, int next_index) {
  int entry = tagEntryList[index];
  vector<int> asso = tagAssociationList[entry];
  int last = asso.size() - 1;
  return asso[last] == next_index;
}

void Tags::create_tag(int i, int j, int index,
                      std::vector<Tags::Entity> &tokens,
                      std::vector<std::array<int, 2>> &parameter_list) {
  if (parameter_list.size() > 0) {
    vector<Tags::Entity> parameter_block = vector<Tags::Entity>{
        Tags::Entity{nParameterBlocks, "", vector<Tags::Entity>()}};
    vector<Tags::Entity> group;
    for (auto pl : parameter_list) {
      group.clear();
      for (int k = pl[0] + 1; k < pl[1]; k++)
        group.push_back(tokens[k]);
      parameter_block[0].entityList.push_back(Tags::Entity{nGroup, "", group});
    }
    tokens[i] = Tags::Entity{
        nTag, "",
        vector<Tags::Entity>{Tags::Entity{index, "", parameter_block}}};
  } else {
    tokens[i] =
        Tags::Entity{nTag, "",
                     vector<Tags::Entity>{Tags::Entity{
                         index, "",
                         vector<Tags::Entity>{Tags::Entity{
                             nParameterBlocks, "", vector<Tags::Entity>()}}}}};
  }
}

void Tags::add_parameter_block(
    int i, std::vector<Tags::Entity> &tokens,
    std::vector<std::array<int, 2>> &parameter_list) {
  Tags::Entity parameter_block =
      Tags::Entity{nParameterBlocks, "", vector<Tags::Entity>()};

  vector<Tags::Entity> group;
  for (auto pl : parameter_list) {
    group.clear();
    for (int k = pl[0] + 1; k < pl[1]; k++)
      group.push_back(tokens[k]);
    parameter_block.entityList.push_back(Tags::Entity{nGroup, "", group});
  }

  tokens[i].entityList[0].entityList.push_back(parameter_block);
}

void Tags::add_argument(int i, std::vector<Tags::Entity> &tokens,
                        const std::array<int, 2> argument) {
  Tags::Entity arguments = Tags::Entity{nArguments, "", vector<Tags::Entity>()};

  for (int k = argument[0] + 1; k < argument[1]; k++)
    arguments.entityList.push_back(tokens[k]);
  tokens[i].entityList[0].entityList.push_back(arguments);
  // tokens[i].entityList.push_back(arguments);
}

bool Tags::check_tag(int i, const std::vector<Tags::Entity> &tokens, int &index,
                     std::vector<std::array<int, 2>> &parameter_list, int &j) {
  parameter_list.clear();
  // Minimum pattern: [nString]
  if (i + 2 >= tokens.size()) {
    return false;
  }

  // First token must be nString
  if (tokens[i + 1].tagNumber != nString) {
    return false;
  }

  std::array<int, 2> parameter = std::array<int, 2>();

  // Is next token a nClosedBracket ?
  if (tokens[i + 2].tagNumber == nClosedBracket) {
    j = i + 2;
    std::string t = Tags::utf8_rtrim(tokens[i + 1].str);
    index = binary_search_utf8(tagList, t);
    return (index != -1);
  }

  // Minimum pattern: [nString|]
  if (i + 3 >= tokens.size()) {
    return false;
  }
  // Next token must be nVerticalLine
  if (tokens[i + 2].tagNumber != nVerticalLine) {
    return false;
  }

  int current_pos = i + 3; // Start after "nString |"

  // Now parse zero or more nLists separated by nVerticalLine
  while (current_pos < tokens.size()) {
    // Start of a new nList (could be empty)
    parameter[0] = current_pos - 1;

    // Consume all nTag/nString in this tList
    while (current_pos < tokens.size() &&
           (tokens[current_pos].tagNumber == nTag ||
            tokens[current_pos].tagNumber == nString)) {
      current_pos++;
    }

    // The end of this nList is the last nTag/nString (or begin-1 if empty)

    if (parameter[0] + 1 < current_pos) {
      parameter[1] = current_pos; // Non-empty list
      parameter_list.push_back(parameter);
    } else {
      parameter[1] = parameter[0] + 1; // Empty list (begin == end+1)
      parameter_list.push_back(parameter);
    }

    // Check if we're at the end of the pattern
    if (current_pos < tokens.size() &&
        tokens[current_pos].tagNumber == nClosedBracket) {
      j = current_pos;
      std::string t = Tags::utf8_rtrim(tokens[i + 1].str);
      index = binary_search_utf8(tagList, t);
      return (index != -1);
    }

    // Otherwise, expect another nVerticalLine followed by a tList (possibly
    // empty)
    if (current_pos >= tokens.size() ||
        tokens[current_pos].tagNumber != nVerticalLine) {
      return false;
    }

    current_pos++; // Skip the nVerticalLine
  }

  // If we reach here, we didn't find a closing bracket
  return false;
}

bool Tags::check_arg(int i, const std::vector<Tags::Entity> &tokens,
                     std::array<int, 2> &argument, int &j) {
  // Check if we're within bounds
  if (i + 1 >= tokens.size()) {
    return false;
  }
  argument[0] = i;
  i++;
  // Save starting position for error reporting if needed
  const int start_pos = i;

  // First, skip all nString and nTag elements (the nList part)
  while (i < tokens.size() && (tokens[i].tagNumber == Tags::nString ||
                               tokens[i].tagNumber == Tags::nTag)) {
    i++;
  }

  // After the tList, we must have a nClosedBracket
  if (i < tokens.size() && tokens[i].tagNumber == Tags::nOpenedBracket) {
    argument[1] = i; // Return position of the opening bracket
    j = i;
    return true;
  }

  // If we got here, pattern wasn't matched
  return false;
}

void Tags::BBCodeToTree() {
  bool modified = true;
  int index;
  int i = 0;
  int j;
  int k;
  std::vector<std::array<int, 2>> parameter_list;
  std::vector<std::vector<std::array<int, 2>>> parameter_block_list;
  std::vector<std::array<int, 2>> argument_list;
  std::array<int, 2> argument;
  while (modified) {
    modified = false;
    while (i < document.size()) {
      if (document[i].tagNumber == nOpenedBracket) {
        parameter_block_list.clear();
        parameter_list.clear();
        bool b = Tags::check_tag(i, document, index, parameter_list, j);
        if (b) {
          int position = tagPositionList[index];
          if (position == 1) {
            // When the position equals 1, we have a starting tag [begin]
            int entry = tagEntryList[index];
            int type = tagTypeList[entry];
            vector<int> asso = tagAssociationList[entry];
            int last = asso.size() - 1;
            switch (type) {
            // See the tag association table, above.
            case TSingle: {
              Tags::create_tag(i, j, index, document, parameter_list);
              document.erase(document.begin() + i + 1,
                             document.begin() + j + 1);
              modified = true;
              break;
            }
            case TBeginEnd: {
              int j1 = j;
              b = Tags::check_arg(j1, document, argument, j);
              // Tags::DisplayEntity(extract(document, argument[0]+1,
              // argument[1]-1));
              if (b) {
                int j2 = j;
                int index_end;
                parameter_block_list.push_back(parameter_list);
                /*
                for (auto p: parameter_block_list[0]) {
                     Tags::DisplayEntity(extract(document, p[0]+1, p[1]-1));
                   }
                */
                parameter_list.clear();
                bool b =
                    Tags::check_tag(j2, document, index_end, parameter_list, j);
                if (b & Tags::is_associated_last(index, index_end)) {

                  parameter_block_list.push_back(parameter_list);

                  Tags::create_tag(i, j1, index, document,
                                   parameter_block_list[0]);
                  add_parameter_block(i, document, parameter_block_list[1]);
                  add_argument(i, document, argument);
                  document.erase(document.begin() + i + 1,
                                 document.begin() + j + 1);
                  // DisplayEntity(extract(document, parameter[0]+1,
                  // parameter[1]-1));
                  modified = true;
                }
              }
              break;
            }
            case TBeginMiddleEnd:
            case TBeginRepeatedMiddleEnd: {
              vector<int> jn;
              jn.push_back(j);
              parameter_block_list.push_back(parameter_list);
              parameter_list.clear();
              argument_list.clear();
              int k = 0;
              b = Tags::check_arg(jn[k], document, argument, j);
              if (b) {
                jn.push_back(j);
                k++;
                argument_list.push_back(argument);
                b = true;
                int index_end;
                while (b) {
                  b = Tags::check_tag(jn[k], document, index_end,
                                      parameter_list, j);
                  if (b & Tags::is_associated_intermediate(index, index_end)) {
                    jn.push_back(j);
                    k++;
                    parameter_block_list.push_back(parameter_list);
                    b = Tags::check_arg(jn[k], document, argument, j);
                    if (b) {
                      jn.push_back(j);
                      k++;
                      argument_list.push_back(argument);
                    } else
                      b = false;
                  }
                  b = Tags::check_tag(jn[k], document, index_end,
                                      parameter_list, j);
                  if (b & Tags::is_associated_last(index, index_end)) {
                    jn.push_back(j);
                    k++;
                    parameter_block_list.push_back(parameter_list);

                    Tags::create_tag(i, jn[0], index, document,
                                     parameter_block_list[0]);
                    parameter_block_list.erase(parameter_block_list.begin());
                    for (auto pl : parameter_block_list)
                      add_parameter_block(i, document, pl);
                    for (auto a : argument)
                      add_argument(i, document, argument);
                    document.erase(document.begin() + i + 1,
                                   document.begin() + jn[k] + 1);
                    modified = true;
                  }
                }
              }
              break;
            }
            } // end switch
          }   // end position
        }     // end first checktag
      }       // end if openedBracket
      i++;
    } // end while i
  }   // end while modified

  // *** Test ***
  // DisplayEntity(document);
}

std::string Tags::TagToString(Tags::Entity e) {
  switch (e.tagNumber) {
  case nStraightLine:
    return "|";
  case nOpenBracket:
    return "[";
  case nClosedBracket:
    return "]";
  case nString:
    return e.str;
  case nMATag:
    return "[not recognized tag]";
  }
  return "[error] tagNumber:" + std::to_string(e.tagNumber);
}

vector<std::string> Tags::ToStringArrayList(vector<Tags::Entity> le,
                                            vector<int> index) {
  vector<std::string> res;
  int i = -1;
  std::string str = "";
  if (index.size() > 0)
    str = to_string(index[0]);
  for (int j = 1; j < index.size(); j++)
    str += "," + to_string(index[j]);
  for (Tags::Entity e : le) {
    if (e.tagNumber == nResult) {
      i++;
      res.push_back(str + "," + to_string(i));
      res.push_back(to_string(nResult));
      i++;
      res.push_back(str + "," + to_string(i));
      res.push_back(e.str);
    } else {
      i++;
      res.push_back(str + "," + to_string(i));
      res.push_back(to_string(nString));
      i++;
      res.push_back(str + "," + to_string(i));
      res.push_back(TagToString(e));
    }
  }
  return res;
}

vector<vector<vector<std::string>>> Tags::GetParameters(Tags::Entity tag) {
  vector<vector<vector<std::string>>> parameter_block;
  vector<vector<std::string>> parameters;
  vector<std::string> group;
  if (tag.tagNumber == nTag) {
    vector<Tags::Entity> pList = tag.entityList[0].entityList;
    for (Tags::Entity pa : pList) {
      if (pa.tagNumber == nParameterBlocks) {
        parameters.clear();
        if (!pa.entityList.empty()) {
          for (Tags::Entity grp : pa.entityList) {
            group.clear();
            if (!grp.entityList.empty()) {
              for (Tags::Entity e : grp.entityList) {
                if (e.tagNumber == nResult) {
                  // group.push_back(to_string(nResult));
                  group.push_back(e.str);
                } else {
                  // group.push_back(to_string(nString));
                  group.push_back(TagToString(e));
                }
              }
            }
            parameters.push_back(group);
          }
        }
        parameter_block.push_back(parameters);
      }
    }
  }
  return parameter_block;
}

vector<vector<std::string>> Tags::GetArguments(Tags::Entity tag) {
  vector<vector<std::string>> arguments;
  vector<vector<std::string>> groups;
  vector<std::string> arg;
  if (tag.tagNumber == nTag) {
    vector<Tags::Entity> pList = tag.entityList[0].entityList;
    for (Tags::Entity pa : pList) {
      if (pa.tagNumber == nArguments) {
        arg.clear();
        if (!pa.entityList.empty()) {
          for (Tags::Entity e : pa.entityList) {
            if (e.tagNumber == nResult) {
              // arg.push_back(to_string(nResult));
              arg.push_back(e.str);
            } else {
              // arg.push_back(to_string(nString));
              arg.push_back(TagToString(e));
            }
          }
        }
        arguments.push_back(arg);
      }
    }
  }
  return arguments;
}

void Tags::display_parameters(vector<vector<vector<std::string>>> p) {
  for (auto b : p) {
    cout << "block" << endl;
    for (auto parameters : b) {
      cout << "   parameter" << endl;
      cout << "   ";
      for (auto s : parameters) {
        cout << " - " << s;
      }
      cout << endl;
    }
  }
}

void Tags::display_arguments(vector<vector<std::string>> arguments) {
  for (auto argument : arguments) {
    cout << "argument" << endl;
    cout << "   ";
    for (auto s : argument) {
      cout << " - " << s;
    }
    cout << endl;
  }
}

std::string Tags::DocumentToHTML() {
  std::string sb = "";
  for (Tags::Entity e : document) {
    sb += e.str;
  }
  HTMLEntityTransformer transformer;
  return transformer.transform(sb);
}

std::string Tags::HTMLEntities(std::string s) {
  int n = s.length();
  std::string str = "";
  for (int i = 0; i < n; i++) {
    auto res = (std::string)(&s[i]);
    switch (s[i]) {
    case L' ':
      res = " ";
      break;
    case L'<':
      res = "&lt;";
      break;
    case L'>':
      res = "&gt;";
      break;
    case L'&':
      res = "&amp;";
      break;
    case L'"':
      res = "&quot;";
      break;
    case L'\'':
      res = "&apos;";
      break;
      /*              case L'¢':
                        res = "&cent;";
                        break;
                    case L'£':
                        res = "&pound;";
                        break;
                    case L'¥':
                        res = "&yen;";
                        break;
                    case L'€':
                        res = "&euro;";
                        break;
                    case L'©':
                        res = "&copy;";
                        break;
                    case L'®':
                        res = "&reg;";
                        break;*/
    }
    str += res;
  }
  return str;
}

// Replace symbol tag ([, |, ]) by a <string>
void Tags::SymbolTagToString(vector<Tags::Entity> eList, int pos) {
  switch (eList[pos].tagNumber) {
  case nStraightLine:
  case nOpenBracket:
  case nClosedBracket:
    eList[pos] = Tags::Entity{nString, TagNumberToString(eList[pos].tagNumber),
                              vector<Tags::Entity>()};
    break;
  }
}

void Tags::SymbolTagToHTML(vector<Tags::Entity> eList, int pos) {
  switch (eList[pos].tagNumber) {
  case nStraightLine:
  case nOpenBracket:
  case nClosedBracket:
    eList[pos] =
        Tags::Entity{nString, Tags::TagNumberToString(eList[pos].tagNumber),
                     vector<Tags::Entity>()};
    break;
  case nString:
  case nResult:
    eList[pos] = Tags::Entity{nString, HTMLEntities(eList[pos].str),
                              vector<Tags::Entity>()};
    break;
  }
}

std::string Tags::StringifyMATag(Tags::Entity e) {
  // we loop into parameters
  std::string sb = "[";
  if (e.tagNumber <= 0) {
    sb += tagList[e.entityList[0].tagNumber];
    vector<Tags::Entity> pList = e.entityList[0].entityList;
    // parameter list
    for (int j = 0; j < pList.size(); j++) {
      Entity pa = pList[j];
      if (pa.tagNumber == nParameterBlocks) {
        if (!pa.entityList.empty()) {
          for (Tags::Entity pb : pList[j].entityList) {
            if (!pb.entityList.empty()) {
              for (Tags::Entity grp : pb.entityList) {
                if (grp.entityList.empty()) {
                  for (int l = 0; l < grp.entityList.size(); l++) {
                    // add elements
                    sb += "|";
                    sb += grp.entityList[l].str;
                  }
                }
              }
            }
          }
        }
      }
      /*
      if (pa.tagNumber == nArguments)
      {
          if (pa.entityList != null)
          {
              foreach (Entity gra in pa.entityList)
              {
                  if (gra.entityList != null)
                  {
                      for (int l = 0; l < gra.entityList.size(); l++)
                      {

                      }
                  }
              }
          }
      }
      */
    }
    sb += "]";
  }
  return sb;
}

struct Position {
public:
  std::vector<Tags::Entity> &list;
  int index;

  // Constructor that takes a reference
  Position(std::vector<Tags::Entity> &vec, int idx) : list(vec), index(idx) {}
};

// Helper to push a C++ vector<string> as a Lua table
void push_string_vector(lua_State *L, const std::vector<std::string> &vec) {
  lua_newtable(L); // Create a new subtable
  for (size_t i = 0; i < vec.size(); ++i) {
    lua_pushinteger(L, i + 1);         // Lua uses 1-based indexing
    lua_pushstring(L, vec[i].c_str()); // Push string value
    lua_settable(L, -3);               // subtable[i+1] = string
  }
}

// Helper to push a C++ vector<vector<string>> as a nested Lua table
void push_nested_string_vector(
    lua_State *L, const std::vector<std::vector<std::string>> &nested_vec) {
  lua_newtable(L); // Create the outer table
  for (size_t i = 0; i < nested_vec.size(); ++i) {
    lua_pushinteger(L, i + 1);            // Outer table key (1-based)
    push_string_vector(L, nested_vec[i]); // Push subtable (vector<string>)
    lua_settable(L, -3);                  // outer_table[i+1] = subtable
  }
}

// Helper to check Lua errors
void check_lua(lua_State *L, int status) {
  if (status != LUA_OK) {
    std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    lua_pop(L, 1); // Remove error message
    exit(1);
  }
}

// Convert a Lua table (on stack) to a C++ vector<string>
std::vector<std::string> lua_table_to_vector(lua_State *L, int index) {
  std::vector<std::string> result;
  if (!lua_istable(L, index)) {
    return result; // Not a table → return empty vector
  }

  lua_pushnil(L);                       // Start table iteration
  while (lua_next(L, index - 1) != 0) { // Pops key, pushes value
    if (lua_isstring(L, -1)) {
      result.push_back(lua_tostring(L, -1));
    }
    lua_pop(L, 1); // Remove value, keep key for next iteration
  }
  return result;
}

// Function to call the Lua function named lua_function
std::vector<std::vector<std::string>> call_lua_function(
    lua_State *L, std::string lua_function,
    const std::vector<std::vector<std::vector<std::string>>> &params,
    const std::vector<std::vector<std::string>> &args) {
  // Push the function onto the stack
  lua_getglobal(L, lua_function.c_str());

  // First argument: params (3D vector)
  lua_newtable(L);
  for (size_t i = 0; i < params.size(); i++) {
    lua_newtable(L);
    for (size_t j = 0; j < params[i].size(); j++) {
      lua_newtable(L);
      for (size_t k = 0; k < params[i][j].size(); k++) {
        lua_pushstring(L, params[i][j][k].c_str());
        lua_rawseti(L, -2, k + 1); // Lua arrays start at 1
      }
      lua_rawseti(L, -2, j + 1);
    }
    lua_rawseti(L, -2, i + 1);
  }

  // Second argument: args (2D vector)
  lua_newtable(L);
  for (size_t i = 0; i < args.size(); i++) {
    lua_newtable(L);
    for (size_t j = 0; j < args[i].size(); j++) {
      lua_pushstring(L, args[i][j].c_str());
      lua_rawseti(L, -2, j + 1);
    }
    lua_rawseti(L, -2, i + 1);
  }

  // Call the function with 2 arguments, expect 1 return value
  if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
    std::cerr << "Error calling " << lua_function << ":" << lua_tostring(L, -1)
              << std::endl;
    lua_pop(L, 1); // pop error message
    return {};
  }

  // Process the return value (should be a table/array)
  std::vector<std::vector<std::string>> result;

  if (lua_istable(L, -1)) {
    // Iterate through the outer table (array part)
    lua_pushnil(L); // first key for iteration

    while (lua_next(L, -2) != 0) {
      // key is at index -2, value is at index -1
      if (lua_istable(L, -1)) {
        std::vector<std::string> innerVec;

        // Iterate through the inner table
        lua_pushnil(L); // first key for inner iteration
        while (lua_next(L, -2) != 0) {
          // key is at index -2, value is at index -1
          if (lua_isstring(L, -1)) {
            innerVec.push_back(lua_tostring(L, -1));
          }
          lua_pop(L, 1); // pop value, keep key for next iteration
        }
        result.push_back(innerVec);
      } else if (lua_isstring(L, -1)) {
        // Handle case where it might be a simple array of strings
        std::vector<std::string> singleElement;
        singleElement.push_back(lua_tostring(L, -1));
        result.push_back(singleElement);
      }
      lua_pop(L, 1); // pop value, keep key for next iteration
    }
  }

  lua_pop(L, 1); // pop the return value

  return result;
}

void Tags::EvalTree() {

  // initialization
  // create new Lua state
  lua_State *L = luaL_newstate();
  luaL_openlibs(L);

  bool flag = false;

  vector<vector<Position>> tagListList;
  int level = 0;
  tagListList.push_back(vector<Position>());
  for (int i = 0; i < document.size(); i++) {
    // auto doc = document;
    if (document[i].tagNumber == nTag || document[i].tagNumber == nMATag)
      tagListList[level].push_back(Position{document, i});
    else
      SymbolTagToString(document, i);
  }
  // descent to the innermost tags

  while (true) {
    if (tagListList[level].size() == 0)
      break;
    level++;
    tagListList.push_back(vector<Position>());
    for (Position pos : tagListList[level - 1]) {
      Entity e = pos.list[pos.index];
      if (e.tagNumber <= 0) {
        vector<Tags::Entity> pList = e.entityList[0].entityList;
        for (int j = 0; j < pList.size(); j++) {
          Entity pa = pList[j];
          if (pa.tagNumber == nParameterBlocks) {
            if (!pa.entityList.empty()) {
              for (Tags::Entity pb : pList[j].entityList) {
                if (!pb.entityList.empty()) {
                  for (Tags::Entity grp : pb.entityList) {
                    if (!grp.entityList.empty()) {
                      for (int l = 0; l < grp.entityList.size(); l++) {
                        if (grp.entityList[l].tagNumber == nTag ||
                            grp.entityList[l].tagNumber == nMATag)
                          tagListList[level].push_back(
                              Position{grp.entityList, l});
                        else
                          SymbolTagToString(grp.entityList, l);
                      }
                    }
                  }
                }
              }
            }
          }
          if (pa.tagNumber == nArguments) {
            if (!pa.entityList.empty()) {
              for (Tags::Entity gra : pa.entityList) {
                if (gra.entityList.empty()) {
                  for (int l = 0; l < gra.entityList.size(); l++) {
                    if (gra.entityList[l].tagNumber == nTag ||
                        gra.entityList[l].tagNumber == nMATag)
                      tagListList[level].push_back(Position{gra.entityList, l});
                    else
                      SymbolTagToString(gra.entityList, l);
                    // SymbolTagToHTML(gra.entityList, l);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  reverse(tagListList.begin(), tagListList.end());
  vector<std::string> results;
  std::string bigScript = "";
  for (std::string script : scriptList) {
    bigScript += script + "\n";
  }
  for (std::string script : commandList) {
    bigScript += script + "\n";
  }
  // cout << bigScript << endl;
  // Load and execute the Lua code
  if (luaL_loadstring(L, bigScript.c_str()) || lua_pcall(L, 0, 0, 0)) {
    std::cerr << "Lua error: " << lua_tostring(L, -1) << std::endl;
    lua_close(L);
    return;
  }

  for (vector<Position> pList : tagListList) {
    /////////////////////////reverse(pList.begin(), pList.end());

    for (Position p : pList) {

      auto tagNum = p.list[p.index].entityList[0];
      int entry = tagEntryList[tagNum.tagNumber];
      std::string command_name = functionNameList[entry];
      // cout << command_name << endl;
      auto tag = p.list[p.index];
      // Tags::DisplayEntity(std::vector<Tags::Entity>{tag});
      // Tags::DisplayEntity(document);
      if (tag.tagNumber == nTag) {
        std::vector<std::vector<std::vector<std::string>>> params =
            GetParameters(tag);
        // display_parameters(params);
        std::vector<std::vector<std::string>> args = GetArguments(tag);
        // display_arguments(args);

        // Call the function
        auto result = call_lua_function(L, command_name, params, args);

        // Print the result
        /*
        std::cout << "Result:" << std::endl;
        for (const auto& vec : result) {
            for (const auto& str : vec) {
                std::cout << str << " ";
            }
            std::cout << std::endl;
        }*/
        std::string res = "";
        for (const auto &vec : result) {
          for (const auto &str : vec) {
            res += str;
          }
          // std::cout << std::endl;
        }

        p.list[p.index] =
            Tags::Entity{nString, res, std::vector<Tags::Entity>()};

      } else {
        // nMATag
        p.list[p.index] = Tags::Entity{nString, StringifyMATag(tag),
                                       std::vector<Tags::Entity>()};
      }
      // DisplayEntity(p.list);
    }
  }
  // close the Lua state
  lua_close(L);
  // Test
  // DisplayEntity(document);
  // for (auto e: document) {
  //   cout << e.str;
  // }
}

void Tags::DisplayEntity(vector<Tags::Entity> document) {
  if (!document.empty()) {
    for (Tags::Entity e : document) {

      if (e.tagNumber < 0) {
        switch (e.tagNumber) {
        case nNone:
          cout << ">>>>>None" << endl;
          break;
        case nString:
          cout << ">>>>>" << e.str << endl;
          break;
        case nOpenBracket:
          cout << ">>>>>" << e.str << endl;
          break;
        case nClosedBracket:
          cout << ">>>>>" << e.str << endl;
          break;
        case nStraightLine:
          cout << ">>>>>" << e.str << endl;
          break;
        case nTag:
          cout << ">>>>>BEGIN TAG" << endl;
          DisplayEntity(e.entityList);
          cout << ">>>>>END TAG" << endl;
          break;
        case nMATag:
          cout << ">>>>>BEGIN MATAG" << endl;
          DisplayEntity(e.entityList);
          cout << ">>>>>END MATAG" << endl;
          break;
        case nParameterBlocks:
          cout << ">>>>>BEGIN PARAMETER BLOCK" << endl;
          DisplayEntity(e.entityList);
          cout << ">>>>>END PARAMETER BLOCK" << endl;
          break;
        case nArguments:
          cout << ">>>>>BEGIN ARGUMENTS" << endl;
          DisplayEntity(e.entityList);
          cout << ">>>>>END ARGUMENTS" << endl;
          break;
        case nGroup:
          cout << ">>>>>BEGIN GROUP" << endl;
          DisplayEntity(e.entityList);
          cout << ">>>>>END GROUP" << endl;
          break;
        }

      } else {
        cout << ">tag>" << tagList[e.tagNumber] << endl;
        if (!e.entityList.empty()) {
          cout << ">>>>>>>>>>" << endl;
          DisplayEntity(e.entityList);
          cout << ">>>>>>>>>>" << endl;
        }
      }
    }
  }
}

void Tags::scan_utf8_file(const std::string &text) {
  // if (text.empty()) return result;

  std::string::const_iterator it = text.begin();
  std::string current_token;

  while (it != text.end()) {
    char32_t code_point;
    try {
      code_point = utf8::next(it, text.end());
    } catch (const utf8::exception &) {
      // Skip invalid UTF-8 sequences
      ++it;
      continue;
    }

    // Check for backslash
    if (code_point == '\\') {
      // Check if next character is a delimiter
      auto next_it = it;
      try {
        char32_t next_cp = utf8::peek_next(next_it, text.end());
        if (next_cp == '[' || next_cp == ']' || next_cp == '|') {
          // Found escaped delimiter - replace with unescaped version
          char32_t cp_arr[] = {next_cp};
          std::string replacement;
          replacement.resize(4);
          char *end = utf8::utf32to8(cp_arr, cp_arr + 1, &replacement[0]);
          replacement.resize(end - &replacement[0]);
          current_token.append(replacement);
          utf8::next(it, text.end()); // Consume the escaped character
          continue;
        }
      } catch (const utf8::exception &) {
        // Skip invalid sequence
      }
      // Not an escaped delimiter - keep backslash as-is
      char32_t cp_arr[] = {code_point};
      std::string char_str;
      char_str.resize(4);
      char *end = utf8::utf32to8(cp_arr, cp_arr + 1, &char_str[0]);
      char_str.resize(end - &char_str[0]);
      current_token.append(char_str);
    }
    // Check for delimiters
    else if (code_point == '[' || code_point == ']' || code_point == '|') {
      // Add current token if not empty
      if (!current_token.empty()) {
        // result.push_back(current_token);
        Tags::document.push_back(Tags::Entity{nString, std::move(current_token),
                                              vector<Tags::Entity>()});
        // current_token.clear();
      }
      // Add the delimiter as a separate token
      char32_t cp_arr[] = {code_point};
      std::string delimiter;
      delimiter.resize(4);
      char *end = utf8::utf32to8(cp_arr, cp_arr + 1, &delimiter[0]);
      delimiter.resize(end - &delimiter[0]);
      // result.push_back(delimiter);
      Tags::document.push_back(Tags::Entity{CharToTagNumber(code_point),
                                            std::move(delimiter),
                                            vector<Tags::Entity>()});
    } else {
      // Regular character - add to current token
      char32_t cp_arr[] = {code_point};
      std::string char_str;
      char_str.resize(4);
      char *end = utf8::utf32to8(cp_arr, cp_arr + 1, &char_str[0]);
      char_str.resize(end - &char_str[0]);
      current_token.append(char_str);
    }
  }

  // Add any remaining characters
  if (!current_token.empty()) {
    // result.push_back(current_token);
    Tags::document.push_back(
        Tags::Entity{nString, current_token, vector<Tags::Entity>()});
  }
  // *** Test ***
  // for (Entity e : document) {
  //    cout << e.tagNumber << " >>>>> " << e.str << endl;
  // }
  // Tags::DisplayEntity(Tags::document);
}

void Tags::ScanFile(std::string fileName) {
  std::string s = fileName;
  std::string text = readFile(fileName);
  // cout << text;
  int textLength = text.length();
  std::string sb = "";
  int i = 0;
  while (i < textLength) {
    char c = text[i];
    if (IsNotDelim(c)) {
      // retrieve the string
      sb = "";
      while (i < textLength) {
        c = text[i];
        if (IsNotSymbol(c))
          sb += c;
        else {
          if (c == '\\') {
            if (++i < textLength) {
              c = text[i];
              if (IsNotSymbol(c)) {
                sb += "\\";
                sb += c;
              } else {
                // no \ before [ ] | \
                                        sb += c;
              }
            } else {
              sb += "\\";
            }
          } else
            break;
        }
        ++i;
      }
      std::string s = sb;
      document.push_back(Tags::Entity{nString, s, vector<Tags::Entity>()});
      --i;
      // Console.WriteLine(">> {0}", sb.ToString());
    } else {
      // [ or ] or | is a delim
      document.push_back(Tags::Entity{CharToTagNumber(c), (std::string)(&c),
                                      vector<Tags::Entity>()});
      // Console.WriteLine("[ or ] or |");
    }
    ++i;
  }
  // *** Test ***
  for (Entity e : document) {
    if (e.tagNumber) {
      cout << "type:" << CharToTagNumber(e.tagNumber) << endl;
    } else
      cout << "type:" << tagList[e.tagNumber] << endl;
    cout << ">>>>>>>>>>" << e.str;
  }
}
