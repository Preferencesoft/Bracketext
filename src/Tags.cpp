// Bracketext.cpp : définit le point d'entrée de l'application.
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

const int nString = -1;  // string in parameters
// const int nResult = -2; // during a conversion, certain character strings
// should only be converted once
const int nOpenBracket = -3;
const int nOpenedBracket = -3;
const int nClosedBracket = -4;
const int nStraightLine = -5;
const int nVerticalLine = -5;
const int nParameterBlocks = -7;  // contains a complete tag
const int nArguments = -8;        // contains a complete tag
const int nGroup =
    -9;  // groups together successions of tags and strings (without [ | ])
const int nInformations = -10; // informations about tag number when a tag has multiple intermediaries
const int nTag = -6;  // tag
// const int nMATag = -11; // tag argument to complete
const int nNone = -12;
// we do not define a tag when the parameters are not complete

std::vector<std::vector<std::string> > Tags::tagInfoList;
std::vector<std::string> Tags::commandList;
std::vector<std::string> Tags::functionNameList;
std::vector<std::string> Tags::scriptList;
std::vector<std::vector<int> > Tags::tagAssociationList;
std::vector<std::string> Tags::tagList;
std::vector<int> Tags::tagEntryList;
std::vector<int> Tags::tagTypeList;
std::vector<int> Tags::tagPositionList;
std::vector<Tags::Entity> Tags::document;
std::vector<std::vector<Tags::Position> > Tags::tagListList;

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
std::string Tags::SubStr(std::string &s, std::string::size_type n) {
    return (s.size() >= n ? s.substr(0, n) : "");
}

void remove_carriage_return(std::string &str) {
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
}

std::string Tags::readFile(std::string &file_name) {
    std::ifstream file(file_name.c_str(), std::ios::binary);  // Use .c_str()
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
    // *** Test ***
    /*
    cout << content << endl;
    */
    return content;
}

std::vector<std::string> Tags::split(const std::string &s, char delimiter) {
    std::vector<std::string> result;
    if (s.empty()) return result;

    std::string::const_iterator start = s.begin();
    std::string::const_iterator end = s.end();
    std::string::const_iterator it = start;
    std::string current_part;
    current_part.reserve(16);  // Preallocate typical word size

    while (it != end) {
        uint32_t code_point;
        try {
            code_point = utf8::next(it, end);

            if (code_point == static_cast<uint32_t>(delimiter)) {
                if (!current_part.empty()) {
                    result.push_back(current_part);
                    current_part.clear();
                    current_part.reserve(16);  // Re-reserve after clear
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
        result.push_back(current_part);
    }

    return result;
}

// Alternative using pointers instead of iterators
const std::string Tags::Trim(const std::string &str) {
    if (str.empty()) return "";

    // Find first non-whitespace
    const char *start_ptr = str.c_str();
    while (*start_ptr && std::isspace(*start_ptr)) {
        start_ptr++;
    }

    // If we reached the end
    if (*start_ptr == '\0') {
        return "";
    }

    // Find last non-whitespace
    const char *end_ptr = str.c_str() + str.size() - 1;
    while (end_ptr > start_ptr && std::isspace(*end_ptr)) {
        end_ptr--;
    }

    return std::string(
        start_ptr,
        end_ptr + 1);  // end_ptr + 1 to include the last character
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
        return vector<string>();  // Empty list if no pipes found
    }

    // Handle the leading ||||||
    if (input.substr(3, 6) != "||||||") {
        return vector<string>();  // Invalid format
    }
    return Tags::split(input.substr(9), '|');
}

std::string getFunctionName(const std::string &line) {
    size_t startPos = line.find("function");
    if (startPos == std::string::npos) {
        return "";
    }

    startPos += 8;  // Move past "function"

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
        return "";  // Empty name after trimming
    }

    std::string name = line.substr(nameStart, nameEnd - nameStart);

    // Crucial: Check if the extracted name contains invalid UTF-8 sequences.
    if (!utf8::is_valid(name.begin(), name.end())) {
        return "";  // Invalid UTF-8
    }

    return name;
}

void Tags::LoadMacros(std::string psFileName) {
    // provisionally, we load the entire text.
    vector<std::string> text = Tags::split(readFile(psFileName), '\n');
/*
    cout << "______________________"  << endl;
    for (vector<std::string>::iterator it=text.begin();it!=text.end();it++) {
       std::string& str = *it; 
       cout << str << endl;
    }
    cout << "______________________"  << endl;
*/

    enum BlockType { NONE, SCRIPT, COMMAND, SKIP };
    BlockType current_block = NONE;

    for (std::vector<std::string>::const_iterator it = text.begin(); it != text.end(); ++it) {
        const std::string& line = *it;
        
        if (current_block == NONE) {
            if (line == "-- <<<<<<") {
                ++it;
                if (it == text.end()) break;
                
                const std::string& type_line = *it;
                
                if (type_line == "-- ||||||g|" || type_line == "-- ||||||v|") {
                    current_block = SCRIPT;
                } 
                else {
                    // Handle function definition case
                    const std::string header = Tags::Trim(type_line);
                    std::vector<string> entry = extractStrings(header);
                    
                    if (!entry.empty()) {
                        ++it;
                        if (it == text.end()) break;
                        
                        const std::string& info_line = *it;
                        --it; // Go back to maintain iterator position
                        
                        std::string fun = getFunctionName(Trim(info_line));
                        if (fun != "") {
                                functionNameList.push_back(fun);
                                tagInfoList.push_back(entry);
                                current_block = COMMAND;
                        } else {
                            current_block = SKIP;
                        }
                    } else {
                        current_block = SKIP;
                    }
                }
            }
        } else {
            if (line == "-- >>>>>>") {
                current_block = NONE;
            } else {
                if (current_block == SCRIPT) {
                    Tags::scriptList.push_back(line);
                } else if (current_block == COMMAND) {
                    Tags::commandList.push_back(line);
                }
            }
        }
    }

/*
    cout << "script list " << endl;
    for (vector<std::string>::iterator it=scriptList.begin();it!=scriptList.end();it++) {
       cout << *it << endl;
    }

    cout << "script list " << endl;
    for (vector<std::string>::iterator it=commandList.begin();it!=commandList.end();it++) {
       cout << *it << endl;
    }

    cout << "tag info list " << endl;
    for (std::vector<vector<std::string> >::iterator it=tagInfoList.begin();it!=tagInfoList.end();it++) {
           vector<std::string>& tagInfo = *it;
           for (vector<std::string>::iterator itt=tagInfo.begin();itt!=tagInfo.end();itt++) {
       cout << *itt << endl;
    }
    cout << endl;
    }
*/
}

// Simple UTF-8 decoder for C++98
uint32_t utf8_next(const char *&ptr, const char *end) {
    if (ptr >= end) return 0;

    unsigned char first_byte = static_cast<unsigned char>(*ptr++);
    uint32_t code_point = 0;

    if (first_byte < 0x80) {
        // 1-byte sequence
        code_point = first_byte;
    } else if ((first_byte & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (ptr >= end) return 0;
        code_point = ((first_byte & 0x1F) << 6) | (*ptr++ & 0x3F);
    } else if ((first_byte & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (ptr + 1 >= end) return 0;
        code_point = ((first_byte & 0x0F) << 12) | ((ptr[0] & 0x3F) << 6) |
                     (ptr[1] & 0x3F);
        ptr += 2;
    } else if ((first_byte & 0xF8) == 0xF0) {
        // 4-byte sequence
        if (ptr + 2 >= end) return 0;
        code_point = ((first_byte & 0x07) << 18) | ((ptr[0] & 0x3F) << 12) |
                     ((ptr[1] & 0x3F) << 6) | (ptr[2] & 0x3F);
        ptr += 3;
    } else {
        // Invalid UTF-8, treat as replacement character
        code_point = 0xFFFD;
    }

    return code_point;
}

bool utf8_compare(const std::string &str1, const std::string &str2) {
    const char *ptr1 = str1.c_str();
    const char *ptr2 = str2.c_str();
    const char *end1 = str1.c_str() + str1.length();
    const char *end2 = str2.c_str() + str2.length();

    while (ptr1 < end1 && ptr2 < end2) {
        uint32_t cp1 = utf8_next(ptr1, end1);  // Pointer-based version
        uint32_t cp2 = utf8_next(ptr2, end2);

        if (cp1 != cp2) {
            return cp1 < cp2;
        }
    }

    return str1.length() < str2.length();
}

bool CompareString(const Tags::info& i1, const Tags::info& i2) {
    return utf8_compare(i1.sep, i2.sep);
}

void Tags::Init() {
    vector<info> infoList;
    std::vector<std::string>::size_type sepListCount = Tags::tagInfoList.size();
    for (std::vector<std::string>::size_type n = 0; n < sepListCount; n++) {
        vector<std::string> sep = Tags::tagInfoList[n];
        std::vector<std::string>::size_type len = sep.size();
        for (std::vector<std::string>::size_type i = 1; i < len; i++) {
            infoList.push_back(Tags::info(sep[i], n, i));
        }
    }
    // sort by string
    sort(infoList.begin(), infoList.end(), CompareString);

    // *** Test ***
    /*
    for (vector<Tags::info>::iterator it=infoList.begin(); it != infoList.end();
    it++) { Tags::info& info=*it; cout << " { " << info.sep << " - " <<
    info.entry
    << " - " << info.pos << " } ";
    }
    */

    int tupleListCount = infoList.size();

    for (int i = 0; i < tupleListCount; i++) {
        info t = infoList[0];
        Tags::tagList.push_back(t.sep);
        Tags::tagEntryList.push_back(t.entry);
        Tags::tagPositionList.push_back(t.pos);
        infoList.erase(infoList.begin());
    }
    // *** Test ***
    /*
    for (vector<std::string>::iterator it=tagList.begin(); it != tagList.end();
    it++) { std::string& tag=*it; cout << " { " << tag << " } ";
      // cout << tag << " , ";
    }
    */
    // *** Test ***
    /*
    std::string t = "list";
    int tNumber = binary_search_utf8(tagList, t);
    cout << tNumber << " + " << tagList[tNumber] << endl;
    */

    for (std::vector<std::string>::size_type n = 0; n < sepListCount; n++) {
        const std::vector<std::string> &separ =
            Tags::tagInfoList[n];  // Use reference
        std::vector<std::string>::size_type len = separ.size();

        if (len < 1) continue;  // Safety check

        tagAssociationList.push_back(std::vector<int>(len - 1));

        for (std::vector<int>::size_type i = 1; i < len; i++) {
            std::vector<std::string>::const_iterator lower = std::lower_bound(
                tagList.begin(), tagList.end(), separ[i], utf8_less);
            int index = 0;
            for (std::vector<std::string>::const_iterator it = tagList.begin();
                 it != lower; ++it) {
                index++;
            }
            tagAssociationList[n][i - 1] = index;
        }

        // Convert string to int using stringstream (recommended)
        int e = 0;
        std::istringstream iss(separ[0]);
        iss >> e;
        tagTypeList.push_back(e);
    }
    // *** Test ***
    // display tagList
    /*
    for (std::vector<std::string>::iterator it=tagList.begin();
    it!=tagList.end();it++) { std::string& s = *it; cout << s << " , " << endl;
    }
    */
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

bool Tags::utf8_less(const std::string &a, const std::string &b) {
    std::string::const_iterator it_a = a.begin();
    std::string::const_iterator it_b = b.begin();

    while (it_a != a.end() && it_b != b.end()) {
        uint32_t cp_a = utf8::next(it_a, a.end());  // Decode next codepoint
        uint32_t cp_b = utf8::next(it_b, b.end());

        if (cp_a != cp_b) {
            return cp_a < cp_b;  // Compare codepoints
        }
    }
    // Shorter strings come first
    return a.size() < b.size();
}

int Tags::binary_search_utf8(const std::vector<std::string> &list,
                             const std::string &key) {
    const std::vector<std::string>::const_iterator it =
        std::lower_bound(list.begin(), list.end(), key, utf8_less);

    if (it != list.end() && !utf8_less(key, *it) && !utf8_less(*it, key)) {
        return std::distance(list.begin(), it);  // Return index if found
    }
    return -1;  // Return -1 if not found
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
    if (utf8_str.empty()) return utf8_str;

    std::string::const_iterator it = utf8_str.begin();
    std::string::const_iterator end_it = utf8_str.end();
    std::string::const_iterator last_valid_pos = end_it;

    while (it != end_it) {
        // std::string::const_iterator prev_it = it;
        try {
            uint32_t cp = utf8::next(it, end_it);
            if (!is_unicode_whitespace(cp)) {
                last_valid_pos = it;  // Last non-whitespace
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
    // for (char c : s) {
    for (std::string::const_iterator it = s.begin(); it != s.end(); ++it) {
        const char &c = *it;
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)(unsigned char)c << " ";
    }
    std::cout << "\n---\n";
}

/*
std::vector<Tags::Entity> extract(const Tags::EntityVector &tokens,
                                  std::string::size_type i,
                                  std::string::size_type j) {
    Tags::EntityVector sublist;
    if (j < tokens.size() && i <= j) {
        sublist.reserve(j - i + 1);
        std::copy(tokens.begin() + i, tokens.begin() + j + 1,
                  std::back_inserter(sublist));
    }
    return sublist;
}
*/

int Tags::association_index(int index, int next_index) {
    int entry = tagEntryList[index];
    vector<int>& asso = tagAssociationList[entry];
    // Find the iterator
    vector<int>::iterator it = std::find(asso.begin(), asso.end(), next_index);
    // If found, return the position/index
    if (it != asso.end()) {
        return it - asso.begin();  // Returns the actual index position
    }
    return -1;  // Return -1 if not found
}

bool Tags::is_associated(int index, int next_index) {
    int entry = tagEntryList[index];
    vector<int>& asso = tagAssociationList[entry];
    return (std::find(asso.begin() + 1, asso.end(), next_index) != asso.end());
}

bool Tags::is_associated_intermediate(int index, int next_index) {
    int entry = tagEntryList[index];
    vector<int>& asso = tagAssociationList[entry];
    std::vector<int>::iterator it = std::find(asso.begin() + 1, asso.end() - 1, next_index);
    bool found = (it != asso.end() - 1);
    return found;
}

bool Tags::is_associated_last(int index, int next_index) {
    int entry = tagEntryList[index];
    vector<int>& asso = tagAssociationList[entry];
    int last = asso.size() - 1;
    return asso[last] == next_index;
}

void Tags::create_tag(std::vector<Tags::Entity>::size_type i, int index,
                      std::vector<Tags::Entity> &tokens,
                      std::vector<EntityIndexPair> &parameter_list) {
    if (parameter_list.size() > 0) {
        vector<Tags::Entity> parameter_block;
        parameter_block.push_back(
            Tags::Entity(nParameterBlocks, "", vector<Tags::Entity>()));
        vector<Tags::Entity> group;
        // for (auto pl : parameter_list) {
        for (std::vector<std::vector<EntityIndexPair> >::size_type it = 0;
             it < parameter_list.size(); it++) {
            group.clear();
            for (std::vector<Tags::Entity>::size_type k =
                     parameter_list[it].first + 1;
                 k < parameter_list[it].second; k++)
                group.push_back(tokens[k]);
            parameter_block[0].entityList.push_back(
                Tags::Entity(nGroup, "", group));
        }
        vector<Tags::Entity> vec_temp;
        vec_temp.push_back(Tags::Entity(index, "", parameter_block));
        tokens[i] = Tags::Entity(nTag, "", vec_temp);
    } else {
        vector<Tags::Entity> vec_temp1;
        vec_temp1.push_back(
            Tags::Entity(nParameterBlocks, "", vector<Tags::Entity>()));
        vector<Tags::Entity> vec_temp2;
        vec_temp2.push_back(Tags::Entity(index, "", vec_temp1));
        tokens[i] = Tags::Entity(nTag, "", vec_temp2);
    }
}

void Tags::add_parameter_block(std::vector<Tags::Entity>::size_type i,
                               std::vector<Tags::Entity> &tokens,
                               std::vector<EntityIndexPair> &parameter_list) {
    Tags::Entity parameter_block =
        Tags::Entity(nParameterBlocks, "", vector<Tags::Entity>());

    vector<Tags::Entity> group;
    for (std::vector<EntityIndexPair>::size_type it = 0;
         it < parameter_list.size(); it++) {
        group.clear();
        for (std::vector<Tags::Entity>::size_type k =
                 parameter_list[it].first + 1;
             k < parameter_list[it].second; k++)
            group.push_back(tokens[k]);
        parameter_block.entityList.push_back(Tags::Entity(nGroup, "", group));
    }

    tokens[i].entityList[0].entityList.push_back(parameter_block);
}

void Tags::add_argument(std::vector<Tags::Entity>::size_type i,
                        std::vector<Tags::Entity> &tokens,
                        const EntityIndexPair argument) {
    Tags::Entity arguments =
        Tags::Entity(nArguments, "", vector<Tags::Entity>());

    for (std::vector<Tags::Entity>::size_type k = argument.first + 1;
         k < argument.second; k++)
        arguments.entityList.push_back(tokens[k]);
    tokens[i].entityList[0].entityList.push_back(arguments);
}

void Tags::add_info(std::vector<Tags::Entity>::size_type i,
                        std::vector<Tags::Entity> &tokens,
                        const std::vector<int>& info) {
    Tags::Entity tag_informations =
        Tags::Entity(nInformations, "", vector<Tags::Entity>());

    for (std::vector<int>::size_type k = 0;
         k < info.size(); k++) {
        Tags::Entity tag_info =
        Tags::Entity(nString, std::to_string(info[k]), vector<Tags::Entity>());
        tag_informations.entityList.push_back(tag_info);
    }
    tokens[i].entityList[0].entityList.push_back(tag_informations);
}

bool Tags::check_tag(std::vector<Tags::Entity>::size_type i,
                     const std::vector<Tags::Entity> &tokens, int &index,
                     std::vector<EntityIndexPair> &parameter_list,
                     std::vector<Tags::Entity>::size_type &j) {
    parameter_list.clear();
    // Minimum pattern: [nString]
    if (i + 2 >= tokens.size()) {
        return false;
    }

    // First token must be nString
    if (tokens[i + 1].tagNumber != nString) {
        return false;
    }

    EntityIndexPair parameter;

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

    std::vector<Tags::Entity>::size_type current_pos =
        i + 3;  // Start after "nString |"

    // Now parse zero or more nLists separated by nVerticalLine
    while (current_pos < tokens.size()) {
        // Start of a new nList (could be empty)
        parameter.first = current_pos - 1;

        // Consume all nTag/nString in this tList
        while (current_pos < tokens.size() &&
               (tokens[current_pos].tagNumber == nTag ||
                tokens[current_pos].tagNumber == nString)) {
            current_pos++;
        }

        // The end of this nList is the last nTag/nString (or begin-1 if empty)

        if (parameter.first + 1 < current_pos) {
            parameter.second = current_pos;  // Non-empty list
            parameter_list.push_back(parameter);
        } else {
            parameter.second =
                parameter.first + 1;  // Empty list (begin == end+1)
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

        current_pos++;  // Skip the nVerticalLine
    }

    // If we reach here, we didn't find a closing bracket
    return false;
}

bool Tags::check_arg(std::vector<Tags::Entity>::size_type i,
                     const std::vector<Tags::Entity> &tokens,
                     EntityIndexPair &argument,
                     std::vector<Tags::Entity>::size_type &j) {
    // Check if we're within bounds
    if (i + 1 >= tokens.size()) {
        return false;
    }
    argument.first = i;
    i++;
    // Save starting position for error reporting if needed
    // const std::vector<Tags::Entity>::size_type start_pos = i;

    // First, skip all nString and nTag elements (the nList part)
    while (i < tokens.size() && (tokens[i].tagNumber == Tags::nString ||
                                 tokens[i].tagNumber == Tags::nTag)) {
        i++;
    }

    // After the tList, we must have a nClosedBracket
    if (i < tokens.size() && tokens[i].tagNumber == Tags::nOpenedBracket) {
        argument.second = i;  // Return position of the opening bracket
        j = i;
        return true;
    }

    // If we got here, pattern wasn't matched
    return false;
}

void Tags::DisplayEntityTag(Tags::Entity &e) {
    std::vector<Tags::Entity> elist;
    elist.push_back(e);
    cout << "begin" << endl;
    cout << "tag number: " << e.tagNumber << endl;
    Tags::DisplayEntity(elist);
    cout << "end" << endl;
}

void Tags::TagsToTree() {
    bool modified = true;
    int index;
    std::vector<Tags::Entity>::size_type j;
    std::vector<EntityIndexPair> parameter_list;
    std::vector<std::vector<EntityIndexPair> > parameter_block_list;
    std::vector<EntityIndexPair> argument_list;
    EntityIndexPair argument;
    while (modified) {
        modified = false;
        std::vector<Tags::Entity>::size_type i = 0;
        while (i < document.size()) {
            if (document[i].tagNumber == nOpenedBracket) {
                parameter_block_list.clear();
                parameter_list.clear();
                bool b = Tags::check_tag(i, document, index, parameter_list, j);
                if (b) {
                    int position = tagPositionList[index];
                    if (position == 1) {
                        // When the position equals 1, we have a starting tag
                        // [begin]
                        int entry = tagEntryList[index];
                        int type = tagTypeList[entry];
                        // std::vector<int> asso = tagAssociationList[entry];
                        // int last = asso.size() - 1;
                        switch (type) {
                            // See the tag association table, above.
                            case TSingle: {
                                Tags::create_tag(i, index, document,
                                                 parameter_list);
                                document.erase(document.begin() + i + 1,
                                               document.begin() + j + 1);
                                modified = true;
                                break;
                            }
                            case TBeginEnd: {
                                std::vector<Tags::Entity>::size_type j1 = j;
                                b = Tags::check_arg(j1, document, argument, j);
                                // Tags::DisplayEntity(extract(document,
                                // argument[0]+1, argument[1]-1));
                                if (b) {
                                    std::vector<Tags::Entity>::size_type j2 = j;
                                    int index_end;
                                    parameter_block_list.push_back(
                                        parameter_list);
                                    parameter_list.clear();
                                    bool b =
                                        Tags::check_tag(j2, document, index_end,
                                                        parameter_list, j);
                                    if (b & Tags::is_associated_last(
                                                index, index_end)) {
                                        parameter_block_list.push_back(
                                            parameter_list);

                                        Tags::create_tag(
                                            i, index, document,
                                            parameter_block_list[0]);
                                        add_parameter_block(
                                            i, document,
                                            parameter_block_list[1]);
                                        add_argument(i, document, argument);
                                        document.erase(
                                            document.begin() + i + 1,
                                            document.begin() + j + 1);
                                        // DisplayEntity(extract(document,
                                        // parameter[0]+1, parameter[1]-1));
                                        modified = true;
                                    }
                                }
                                break;
                            }
                            case TBeginMiddleEnd:
                            case TBeginRepeatedMiddleEnd: {
                                vector<std::vector<Tags::Entity>::size_type> jn;
                                jn.push_back(j);
                                parameter_block_list.push_back(parameter_list);
                                parameter_list.clear();
                                argument_list.clear();
                                int k = 0;
                                b = true;
                                int index_end;
                                bool is_final = true;
                                bool is_associated = true;
                                while (b && (is_associated)) {
                                  b = Tags::check_arg(jn[k], document, argument, j);
                                  if (b) {
                                    jn.push_back(j);
                                    k++;
                                    argument_list.push_back(argument);
                                    b = Tags::check_tag(jn[k], document,
                                                            index_end,
                                                            parameter_list, j);
                                    if (b) {
                                       is_associated = Tags::is_associated(index, index_end);
                                       if (is_associated) {
                                         jn.push_back(j);
                                         k++;
                                         parameter_block_list.push_back(parameter_list);
                                         is_final = Tags::is_associated_last(index, index_end);
                                         if (is_final) b=false;
                                       }
                                    }
                                  }
                                }
                                if (is_final) {
                                                Tags::create_tag(
                                                    i, index, document,
                                                    parameter_block_list[0]);
                                                parameter_block_list.erase(
                                                    parameter_block_list
                                                        .begin());

                                                for (std::vector<std::vector<
                                                         EntityIndexPair> >::
                                                         size_type ind = 0;
                                                     ind < parameter_block_list
                                                               .size();
                                                     ind++) {
                                                    add_parameter_block(
                                                        i, document,
                                                        parameter_block_list
                                                            [ind]);
                                                }
                                                for (std::vector<
                                                         EntityIndexPair>::
                                                         size_type ind = 0;
                                                     ind < argument_list.size();
                                                     ind++) {
                                                    add_argument(
                                                        i, document,
                                                        argument_list[ind]);
                                                }
                                                document.erase(
                                                    document.begin() + i + 1,
                                                    document.begin() + jn[k] +
                                                        1);
                                                modified = true;

                                } //end if b && is_final
                                break;
                            } // end case
                            case TMultipleRepeatedMiddleBlocks: {
                                vector<std::vector<Tags::Entity>::size_type> jn;
                                std::vector<int> tag_type_list; // tag type
                                jn.push_back(j);
                                tag_type_list.push_back(1); // we begin from index 1 for Lua
                                parameter_block_list.push_back(parameter_list);
                                parameter_list.clear();
                                argument_list.clear();
                                int k = 0;
                                b = true;
                                int index_end;
                                bool is_final = true;
                                bool is_associated = true;
                                while (b && (is_associated)) {
                                  b = Tags::check_arg(jn[k], document, argument, j);
                                  if (b) {
                                    jn.push_back(j);
                                    k++;
                                    argument_list.push_back(argument);
                                    b = Tags::check_tag(jn[k], document,
                                                            index_end,
                                                            parameter_list, j);
                                    if (b) {
                                       is_associated = Tags::is_associated(index, index_end);
                                       if (is_associated) {
                                         jn.push_back(j);
                                         k++;
                                         parameter_block_list.push_back(parameter_list);
                                         tag_type_list.push_back(Tags::association_index(index, index_end)+1);
                                         is_final = Tags::is_associated_last(index, index_end);
                                         if (is_final) b=false;
                                       }
                                    }
                                  }
                                }
                                if (is_final) {
                                                Tags::create_tag(
                                                    i, index, document,
                                                    parameter_block_list[0]);
                                                parameter_block_list.erase(
                                                    parameter_block_list
                                                        .begin());

                                                for (std::vector<std::vector<
                                                         EntityIndexPair> >::
                                                         size_type ind = 0;
                                                     ind < parameter_block_list
                                                               .size();
                                                     ind++) {
                                                    add_parameter_block(
                                                        i, document,
                                                        parameter_block_list
                                                            [ind]);
                                                }
                                                for (std::vector<
                                                         EntityIndexPair>::
                                                         size_type ind = 0;
                                                     ind < argument_list.size();
                                                     ind++) {
                                                    add_argument(
                                                        i, document,
                                                        argument_list[ind]);
                                                }
                                                add_info(
                                                        i, document,
                                                        tag_type_list);
                                                document.erase(
                                                    document.begin() + i + 1,
                                                    document.begin() + jn[k] +
                                                        1);
                                                modified = true;

                                } //end if b && is_final
                                break;
                            } // end case
                        }  // end switch
                    }      // end position
                }          // end first checktag
            }              // end if openedBracket
            i++;
        }  // end while i
    }      // end while modified

    // *** Test ***
    // cout << "Document " << endl;
    // DisplayEntity(document);
    // cout << "Fin du document " << endl;
}

std::string int_to_string(int i) {
    std::ostringstream oss;
    oss << i;
    return oss.str();
}

std::string Tags::TagToString(const Tags::Entity &e) {
    switch (e.tagNumber) {
        case nStraightLine:
            return "|";
        case nOpenBracket:
            return "[";
        case nClosedBracket:
            return "]";
        case nString:
            return e.str;
    }
    return "[error] tagNumber:" + int_to_string(e.tagNumber);
}

std::vector<std::vector<std::string> > Tags::GetParameters(
    const Tags::Entity &tag) {
    std::vector<std::vector<std::string> > parameter_block;

    if (tag.tagNumber == nTag && !tag.entityList.empty()) {
        const std::vector<Tags::Entity> &pList = tag.entityList[0].entityList;

        for (std::vector<Tags::Entity>::size_type i = 0; i < pList.size();
             i++) {
            const Tags::Entity &pa = pList[i];

            if (pa.tagNumber == nParameterBlocks) {
                std::vector<std::string> parameters;
                if (!pa.entityList.empty()) {
                    for (std::vector<Tags::Entity>::size_type j = 0;
                         j < pa.entityList.size(); j++) {
                        const Tags::Entity &grp = pa.entityList[j];
                        std::string group;

                        if (!grp.entityList.empty()) {
                            for (std::vector<Tags::Entity>::size_type k = 0;
                                 k < grp.entityList.size(); k++) {
                                Tags::Entity e = grp.entityList[k];
                                group += TagToString(e);
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

std::vector<std::string> Tags::GetArguments(
    const Tags::Entity &tag) {
    std::vector<std::string> arguments;

    if (tag.tagNumber == nTag && !tag.entityList.empty()) {
        const std::vector<Tags::Entity> &pList = tag.entityList[0].entityList;

        for (std::vector<Tags::Entity>::size_type i = 0; i < pList.size();
             i++) {
            const Tags::Entity &pa = pList[i];

            if (pa.tagNumber == nArguments) {
                std::string arg;

                if (!pa.entityList.empty()) {
                    for (std::vector<Tags::Entity>::size_type j = 0;
                         j < pa.entityList.size(); j++) {
                        Tags::Entity e = pa.entityList[j];
                        arg +=TagToString(e);

                    }
                }
                arguments.push_back(arg);
            }
        }
    }
    return arguments;
}

std::vector<std::string> Tags::GetInformations(
    const Tags::Entity &tag) {
    std::vector<std::string> informations;

    if (tag.tagNumber == nTag && !tag.entityList.empty()) {
        const std::vector<Tags::Entity> &pList = tag.entityList[0].entityList;

        for (std::vector<Tags::Entity>::size_type i = 0; i < pList.size();
             i++) {
            const Tags::Entity &pa = pList[i];

            if (pa.tagNumber == nInformations) {
                std::string arg;

                if (!pa.entityList.empty()) {
                    for (std::vector<Tags::Entity>::size_type j = 0;
                         j < pa.entityList.size(); j++) {
                        Tags::Entity e = pa.entityList[j];
                        informations.push_back(e.str);
                    }
                }
            }
        }
    }
    return informations;
}


void Tags::display_parameters(
    const std::vector<std::vector<std::string> >& p) {
    for (std::vector<std::vector<std::string> >::const_iterator it_parameters =
             p.begin();
         it_parameters != p.end(); ++it_parameters) {
        std::cout << "block" << std::endl;

        const std::vector<std::string> &parameters = *it_parameters;

        for (std::vector<std::string>::const_iterator it_parameter =
                 parameters.begin();
             it_parameter != parameters.end(); ++it_parameter) {
            std::cout << "   parameter" << std::endl;
            std::cout << "   ";

            const std::string &parameter = *it_parameter;
            std::cout << " - " << parameter;
            std::cout << std::endl;
        }
    }
}

void Tags::display_arguments(const vector<std::string>& arguments) {
    for (vector<std::string>::const_iterator it_argument = arguments.begin();
         it_argument != arguments.end(); ++it_argument) {
        const std::string &argument = *it_argument;
        cout << "argument" << endl;
        cout << "   ";
        cout << " - " << argument;
        cout << endl;
    }
}

void Tags::display_informations(const std::vector<std::string>& informations) {
    for (vector<std::string>::const_iterator it_information = informations.begin();
         it_information != informations.end(); ++it_information) {
        const std::string &information = *it_information;
        cout << "information" << endl;
        cout << "   ";
        cout << " - " << information;
        cout << endl;
    }
}

std::string Tags::DocumentToHTML() {
    std::string sb = "";
    for (std::vector<Tags::Entity>::iterator it_entity = document.begin();
         it_entity != document.end(); ++it_entity) {
        Tags::Entity e = *it_entity;
        sb += e.str;
    }
    HTMLEntityTransformer transformer;
    return transformer.transform(sb);
}

std::string Tags::DocumentToFile(const std::string &fileName) {
    std::string content = "";
    for (std::vector<Tags::Entity>::iterator it_entity = document.begin();
         it_entity != document.end(); ++it_entity) {
        Tags::Entity e = *it_entity;
        content += e.str;
    }
    HTMLEntityTransformer transformer;
    content = transformer.transform(content);
    // Check if the string is empty (optional)
    if (content.empty()) {
        return "Warning: Content is empty, creating empty file";
    }
    // Open file in binary mode to preserve UTF-8 encoding
    std::ofstream file(fileName.c_str(), std::ios::binary);

    if (!file) {
        return "Error: Failed to open file " + fileName + " for writing";
    }
    // Write content
    file.write(content.c_str(), content.size());

    // Check if write was successful
    if (!file) {
        return "Error: Failed to write to file " + fileName;
    }

    file.close();
    return "Success: File " + fileName + " saved successfully";

    // Verify file was written (optional)
    std::ifstream test(fileName.c_str(), std::ios::binary);
    if (!test) {
        return "Error: File verification failed for " + fileName;
    }
    test.close();
    return "Success: File verification succeed for " + fileName;
}


std::string Tags::HTMLEntities(std::string s) {
    std::string result;
    std::string::iterator it = s.begin();
    std::string::iterator end = s.end();
    
    while (it != end) {
        uint32_t code_point = utf8::next(it, end); // Get next UTF-8 code point
        
        switch (code_point) {
            case ' ':
                result += " ";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '&':
                result += "&amp;";
                break;
            case '"':
                result += "&quot;";
                break;
            case '\'':
                result += "&apos;";
                break;
            case 0x00A2: // ¢
                result += "&cent;";
                break;
            case 0x00A3: // £
                result += "&pound;";
                break;
            case 0x00A5: // ¥
                result += "&yen;";
                break;
            case 0x20AC: // €
                result += "&euro;";
                break;
            case 0x00A9: // ©
                result += "&copy;";
                break;
            case 0x00AE: // ®
                result += "&reg;";
                break;
            default:
                // For non-special characters, append the original UTF-8 sequence
                utf8::append(code_point, std::back_inserter(result));
                break;
        }
    }
    
    return result;
}

// Replace symbol tag ([, |, ]) by a <string>
void Tags::SymbolTagToString(vector<Tags::Entity> &eList, int pos) {
    switch (eList[pos].tagNumber) {
        case nStraightLine:
        case nOpenBracket:
        case nClosedBracket:
            eList[pos] =
                Tags::Entity(nString, TagNumberToString(eList[pos].tagNumber),
                             std::vector<Tags::Entity>());
            break;
    }
}

/*
std::string Tags::StringifyMATag(const Tags::Entity &e) {
    std::string sb = "[";
    if (e.tagNumber <= 0 && !e.entityList.empty()) {
        sb += tagList[e.entityList[0].tagNumber];
        const std::vector<Tags::Entity> &pList = e.entityList[0].entityList;

        for (std::vector<Tags::Entity>::size_type j = 0; j < pList.size();
             j++) {
            const Tags::Entity &pa = pList[j];
            if (pa.tagNumber == nParameterBlocks && !pa.entityList.empty()) {
                for (std::vector<Tags::Entity>::size_type k = 0;
                     k < pa.entityList.size(); k++) {
                    const Tags::Entity &pb = pa.entityList[k];
                    if (!pb.entityList.empty()) {
                        for (std::vector<Tags::Entity>::size_type m = 0;
                             m < pb.entityList.size(); m++) {
                            const Tags::Entity &grp = pb.entityList[m];
                            if (!grp.entityList.empty()) {
                                for (std::vector<Tags::Entity>::size_type l = 0;
                                     l < grp.entityList.size(); l++) {
                                    sb += "|";
                                    sb += grp.entityList[l].str;
                                }
                            }
                        }
                    }
                }
            }
        }
        sb += "]";
    }
    return sb;
}
*/

// Helper to push a C++ vector<string> as a Lua table
void push_string_vector(lua_State *L, const std::vector<std::string> &vec) {
    lua_newtable(L);  // Create a new subtable
    for (size_t i = 0; i < vec.size(); ++i) {
        lua_pushinteger(L, i + 1);          // Lua uses 1-based indexing
        lua_pushstring(L, vec[i].c_str());  // Push string value
        lua_settable(L, -3);                // subtable[i+1] = string
    }
}

// Helper to push a C++ vector<vector<string> > as a nested Lua table
void push_nested_string_vector(
    lua_State *L, const std::vector<std::vector<std::string> > &nested_vec) {
    lua_newtable(L);  // Create the outer table
    for (size_t i = 0; i < nested_vec.size(); ++i) {
        lua_pushinteger(L, i + 1);             // Outer table key (1-based)
        push_string_vector(L, nested_vec[i]);  // Push subtable (vector<string>)
        lua_settable(L, -3);                   // outer_table[i+1] = subtable
    }
}

// Function to call the Lua function named lua_function
std::vector<std::vector<std::string> > call_lua_function(
    lua_State *L, const std::string &lua_function,
    const std::vector<std::vector<std::string> > &params,
    const std::vector<std::string> &args, const std::vector<std::string> &infos) {
    int initial_stack = lua_gettop(L);

    // Push the function onto the stack
    lua_getglobal(L, lua_function.c_str());

    if (!lua_isfunction(L, -1)) {
        std::cerr << "Error: " << lua_function << " is not a function"
                  << std::endl;
        lua_settop(L, initial_stack);
        return std::vector<std::vector<std::string> >();
    }

    // First argument: params (2D vector) - handle empty case
    lua_newtable(L);
    if (!params.empty()) {
        for (size_t i = 0; i < params.size(); i++) {
            lua_newtable(L);
            if (!params[i].empty()) {
                for (size_t j = 0; j < params[i].size(); j++) {
                    lua_pushstring(L, params[i][j].c_str());
                    lua_rawseti(L, -2, j + 1);
                }
            }
            lua_rawseti(L, -2, i + 1);
        }
    }

    // Second argument: args (1D vector) - handle empty case
    lua_newtable(L);
    if (!args.empty()) {
        for (size_t i = 0; i < args.size(); i++) {
            lua_pushstring(L, args[i].c_str());
            lua_rawseti(L, -2, i + 1);
        }
    }
    int nparam = 2;
    // Third argument (optional): info (1D vector) - handle empty case
    if (!infos.empty()) {
        ++nparam;
        lua_newtable(L);
        for (size_t i = 0; i < infos.size(); i++) {
            lua_pushstring(L, infos[i].c_str());
            lua_rawseti(L, -2, i + 1);
        }
    }

    // Call the function
    if (lua_pcall(L, nparam, 1, 0) != 0) {
        std::cerr << "Error calling " << lua_function << ": "
                  << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        lua_settop(L, initial_stack);
        return std::vector<std::vector<std::string> >();
    }

    // Process the return value - LUA 5.1 COMPATIBLE VERSION
    std::vector<std::vector<std::string> > result;

    // Check if return value is a table
    if (lua_istable(L, -1)) {
        // Get table size for iteration
        lua_len(L, -1); // Push table length onto stack
        int table_size = lua_tointeger(L, -1);
        lua_pop(L, 1); // Pop the length
        
        // Iterate through outer table using numeric indices
        for (int i = 1; i <= table_size; i++) {
            lua_rawgeti(L, -1, i); // Push table[i] onto stack
            
            if (lua_istable(L, -1)) {
                std::vector<std::string> innerVec;
                
                // Get inner table size
                lua_len(L, -1); // Push inner table length
                int inner_size = lua_tointeger(L, -1);
                lua_pop(L, 1); // Pop the length
                
                // Iterate through inner table
                for (int j = 1; j <= inner_size; j++) {
                    lua_rawgeti(L, -1, j); // Push inner_table[j] onto stack
                    
                    if (lua_isstring(L, -1)) {
                        innerVec.push_back(lua_tostring(L, -1));
                    }
                    lua_pop(L, 1); // Pop the value
                }
                result.push_back(innerVec);
            } else if (lua_isstring(L, -1)) {
                std::vector<std::string> singleElement;
                singleElement.push_back(lua_tostring(L, -1));
                result.push_back(singleElement);
            }
            
            lua_pop(L, 1); // Pop the table element
        }
    }
    // Handle case where return value is a single string (not a table)
    else if (lua_isstring(L, -1)) {
        std::vector<std::string> singleElement;
        singleElement.push_back(lua_tostring(L, -1));
        result.push_back(singleElement);
    }

    lua_pop(L, 1);  // pop the return value
    lua_settop(L, initial_stack);

    return result;
}
/*
std::vector<std::vector<std::string> > call_lua_function(
    lua_State *L, const std::string &lua_function,
    const std::vector<std::vector<std::vector<std::string> > > &params,
    const std::vector<std::vector<std::string> > &args) {
    int initial_stack = lua_gettop(L);

    // Push the function onto the stack
    lua_getglobal(L, lua_function.c_str());

    if (!lua_isfunction(L, -1)) {
        std::cerr << "Error: " << lua_function << " is not a function"
                  << std::endl;
        lua_settop(L, initial_stack);
        return std::vector<std::vector<std::string> >();
    }

    // First argument: params (3D vector) - handle empty case
    lua_newtable(L);
    if (!params.empty()) {
        for (size_t i = 0; i < params.size(); i++) {
            lua_newtable(L);
            if (!params[i].empty()) {
                for (size_t j = 0; j < params[i].size(); j++) {
                    lua_newtable(L);
                    if (!params[i][j].empty()) {
                        for (size_t k = 0; k < params[i][j].size(); k++) {
                            lua_pushstring(L, params[i][j][k].c_str());
                            lua_rawseti(L, -2, k + 1);
                        }
                    }
                    lua_rawseti(L, -2, j + 1);
                }
            }
            lua_rawseti(L, -2, i + 1);
        }
    }

    // Second argument: args (2D vector) - handle empty case
    lua_newtable(L);
    if (!args.empty()) {
        for (size_t i = 0; i < args.size(); i++) {
            lua_newtable(L);
            if (!args[i].empty()) {
                for (size_t j = 0; j < args[i].size(); j++) {
                    lua_pushstring(L, args[i][j].c_str());
                    lua_rawseti(L, -2, j + 1);
                }
            }
            lua_rawseti(L, -2, i + 1);
        }
    }

    // Call the function
    if (lua_pcall(L, 2, 1, 0) != 0) {
        std::cerr << "Error calling " << lua_function << ": "
                  << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        lua_settop(L, initial_stack);
        return std::vector<std::vector<std::string> >();
    }

    // Process the return value - LUA 5.1 COMPATIBLE VERSION
    std::vector<std::vector<std::string> > result;

    // Alternative approach using manual iteration (more flexible)
    if (lua_istable(L, -1)) {
        // Iterate through outer table
        lua_pushnil(L);  // First key
        while (lua_next(L, -2) != 0) {
            // Key at index -2, value at index -1

            if (lua_istable(L, -1)) {
                std::vector<std::string> innerVec;

                // Iterate through inner table
                lua_pushnil(L);  // First key for inner table
                while (lua_next(L, -2) != 0) {
                    if (lua_isstring(L, -1)) {
                        innerVec.push_back(lua_tostring(L, -1));
                    }
                    lua_pop(L, 1);  // pop value, keep key
                }
                result.push_back(innerVec);
            } else if (lua_isstring(L, -1)) {
                std::vector<std::string> singleElement;
                singleElement.push_back(lua_tostring(L, -1));
                result.push_back(singleElement);
            }

            lua_pop(L, 1);  // pop value, keep key for next outer iteration
        }
    }

    lua_pop(L, 1);  // pop the return value
    lua_settop(L, initial_stack);

    return result;
}
*/
void Tags::EvalTree() {
    std::vector<std::vector<Tags::Position> >::size_type level = 0;

    Tags::tagListList.push_back(vector<Position>());
    for (vector<Tags::Entity>::size_type i = 0; i < document.size(); i++) {
        if (document[i].tagNumber == nTag) {
            tagListList[level].push_back(Tags::Position(&document, i));
        }
    }

    // descent to the innermost tags

    while (true) {
        if (tagListList[level].size() == 0) break;
        level++;
        tagListList.push_back(vector<Position>());

        size_t size = tagListList[level - 1].size();
        for (size_t i_pos = 0; i_pos < size; ++i_pos) {
            Tags::Position &pos = tagListList[level - 1][i_pos];
            // push_back operations are safe now
            // cout << pos.list << endl;;
            Tags::Entity &e = (*(pos.list))[pos.index];
            // DisplayEntityTag(e);
            // cout << "e.tagNumber" << e.tagNumber << endl;
            // if (e.tagNumber <= 0) {
            vector<Tags::Entity> &pList = e.entityList[0].entityList;
            for (std::vector<Tags::Entity>::size_type j = 0; j < pList.size();
                 j++) {
                Entity &pa = pList[j];
                if (pa.tagNumber == nParameterBlocks) {
                    // DisplayEntityTag(pa);
                    if (!pa.entityList.empty()) {
                        for (std::vector<Tags::Entity>::size_type ind_pa = 0;
                             ind_pa < pa.entityList.size(); ind_pa++) {
                            Tags::Entity &group = pa.entityList[ind_pa];
                            // cout << "parameter? (ngroup)" << group.tagNumber
                            // << endl;
                            if (!group.entityList.empty()) {
                                for (std::vector<Tags::Entity>::size_type l = 0;
                                     l < group.entityList.size(); l++) {
                                    if (group.entityList[l].tagNumber == nTag) {
                                        tagListList[level].push_back(
                                            Position(&(group.entityList), l));
                                    }
                                }
                            }
                        }
                    }
                }
                if (pa.tagNumber == nArguments) {
                    // DisplayEntityTag(pa);
                    if (!pa.entityList.empty()) {
                        for (std::vector<Tags::Entity>::size_type m = 0;
                             m < pa.entityList.size(); m++) {
                            const Tags::Entity &gra = pa.entityList[m];
                            if (gra.tagNumber == nTag) {
                                tagListList[level].push_back(
                                    Position(&(pa.entityList), m));
                                // DisplayEntityTag(gra);
                            }
                            // else
                            //  SymbolTagToString(gra.entityList, l);
                        }
                    }
                }
            }
        }
    }
    // initialization
    // create new Lua state
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    std::string bigScript = "";
    for (vector<std::string>::const_iterator it = scriptList.begin();
         it != scriptList.end(); ++it) {
        const std::string &script = *it;
        // cout << script << endl;
        bigScript += script + "\n";
    }
    for (vector<std::string>::const_iterator it = commandList.begin();
         it != commandList.end(); ++it) {
        const std::string &script = *it;
        bigScript += script + "\n";
        // cout << script << endl;
    }
    // cout << bigScript << endl;

    // Load and execute the Lua code
    // First, just try to load the script to check for syntax errors
    if (luaL_loadstring(L, bigScript.c_str()) != 0) {
        std::cerr << "Lua syntax error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        lua_close(L);
        return;
    }

    // If loading succeeded, then try to execute
    if (lua_pcall(L, 0, 0, 0) != 0) {
        std::cerr << "Lua runtime error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        lua_close(L);
        return;
    }
    // cout << "level: " << level << endl;
    for (std::vector<std::vector<Tags::Position> >::size_type v = 0; v <= level;
         v++) {
        std::vector<Tags::Position> &pList = Tags::tagListList[level - v];
        for (std::vector<Position>::size_type ip = 0; ip < pList.size(); ip++) {
            Position &p = pList[ip];
            // cout << "tagnumber = " << (*(p.list))[p.index].tagNumber << endl;
            Tags::Entity tagNum = (*(p.list))[p.index].entityList[0];
            int entry = tagEntryList[tagNum.tagNumber];
            cout << "tag name: " << tagList[tagNum.tagNumber] << endl;
            std::string command_name = functionNameList[entry];
            // cout << command_name << endl;
            Tags::Entity &tag = (*(p.list))[p.index];
            // DisplayEntityTag(tag);
            // Tags::DisplayEntity(document);
            if (tag.tagNumber == nTag) {
                std::vector<std::vector<std::string> > params =
                GetParameters(tag);
                display_parameters(params);
                std::vector<std::string> args = GetArguments(tag);
                display_arguments(args);
                std::vector<std::string> infos = GetInformations(tag);
                display_informations(infos);

                // Call the function
                // cout << "command name" << command_name << endl;
                std::vector<std::vector<std::string> > result =
                    call_lua_function(L, command_name, params, args, infos);

                // Print the result
                /*
                    std::cout << "Result:" << std::endl;
                    for (std::vector<std::vector<std::string> >::size_type i =
                   0; i < result.size(); ++i) { for
                   (std::vector<std::string>::size_type j = 0; j <
                   result[i].size(); ++j) { std::cout << result[i][j] << " ";
                      }
                      std::cout << std::endl;
                    }
                */
                std::string res = "";
                for (std::vector<std::vector<std::string> >::size_type i = 0;
                     i < result.size(); ++i) {
                    for (std::vector<std::string>::size_type j = 0;
                         j < result[i].size(); ++j) {
                        res += result[i][j];
                    }
                }

                (*p.list)[p.index] =
                    Tags::Entity(nString, res, std::vector<Tags::Entity>());
            } else {
                // other entities are converted to strings
                // (*p.list)[p.index] = Tags::Entity(nString, StringifyMATag(tag), std::vector<Tags::Entity>());
                (*p.list)[p.index] = Tags::Entity(nString, Tags::TagToString(tag), std::vector<Tags::Entity>());
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
        for (std::vector<Tags::Entity>::size_type i = 0; i < document.size();
             i++) {
            Tags::Entity e = document[i];
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
    if (text.empty()) return;

    std::string::const_iterator it = text.begin();
    std::string current_token;

    while (it != text.end()) {
        // Get current character (UTF-8 aware)
        unsigned char first_byte = static_cast<unsigned char>(*it);
        uint32_t code_point;
        size_t byte_length = 1;

        try {
            // Decode UTF-8 character manually for C++98 compatibility
            if (first_byte < 0x80) {
                // 1-byte sequence (ASCII)
                code_point = first_byte;
                byte_length = 1;
            } else if ((first_byte & 0xE0) == 0xC0) {
                // 2-byte sequence
                if (it + 1 >= text.end()) throw utf8::exception();
                code_point = ((first_byte & 0x1F) << 6) | (it[1] & 0x3F);
                byte_length = 2;
            } else if ((first_byte & 0xF0) == 0xE0) {
                // 3-byte sequence
                if (it + 2 >= text.end()) throw utf8::exception();
                code_point = ((first_byte & 0x0F) << 12) |
                             ((it[1] & 0x3F) << 6) | (it[2] & 0x3F);
                byte_length = 3;
            } else if ((first_byte & 0xF8) == 0xF0) {
                // 4-byte sequence
                if (it + 3 >= text.end()) throw utf8::exception();
                code_point = ((first_byte & 0x07) << 18) |
                             ((it[1] & 0x3F) << 12) | ((it[2] & 0x3F) << 6) |
                             (it[3] & 0x3F);
                byte_length = 4;
            } else {
                // Invalid UTF-8
                throw utf8::exception();
            }
        } catch (const utf8::exception &) {
            // Skip invalid UTF-8 sequences
            ++it;
            continue;
        }

        // Check for backslash (escape character)
        if (code_point == '\\') {
            // Check if next character is a delimiter
            std::string::const_iterator next_it = it + byte_length;
            if (next_it != text.end()) {
                unsigned char next_byte = static_cast<unsigned char>(*next_it);
                uint32_t next_cp = next_byte;  // Simple ASCII check

                if (next_cp == '[' || next_cp == ']' || next_cp == '|') {
                    // Found escaped delimiter - add the delimiter itself
                    // (without backslash)
                    current_token += static_cast<char>(next_cp);
                    it = next_it +
                         1;  // Skip both backslash and escaped character
                    continue;
                }
            }
            // Not an escaped delimiter - keep backslash as-is
            current_token += '\\';
            it += byte_length;
        }
        // Check for delimiters
        else if (code_point == '[' || code_point == ']' || code_point == '|') {
            // Add current token if not empty
            if (!current_token.empty()) {
                Tags::document.push_back(Tags::Entity(
                    nString, current_token, std::vector<Tags::Entity>()));
                current_token.clear();
            }
            // Add the delimiter as a separate token
            std::string delimiter(1, static_cast<char>(code_point));
            Tags::document.push_back(Tags::Entity(CharToTagNumber(code_point),
                                                  delimiter,
                                                  std::vector<Tags::Entity>()));
            it += byte_length;
        } else {
            // Regular character - add to current token
            // For simplicity, handle ASCII characters directly
            if (code_point < 128) {
                current_token += static_cast<char>(code_point);
            } else {
                // For non-ASCII, use UTF-8 encoding
                for (size_t i = 0; i < byte_length; ++i) {
                    current_token += *(it + i);
                }
            }
            it += byte_length;
        }
    }

    // Add any remaining characters
    if (!current_token.empty()) {
        Tags::document.push_back(
            Tags::Entity(nString, current_token, std::vector<Tags::Entity>()));
    }
    // *** Test ***
    /*
    for (Tags::EntityIterator it=document.begin(); it != document.end(); it++) {
      Tags::Entity& e = *it;
      cout << e.tagNumber << " >>>>> " << e.str << endl;
    }
    Tags::DisplayEntity(Tags::document);
    */
}

