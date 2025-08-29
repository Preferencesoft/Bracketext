// Bracketext.h : fichier Include pour les fichiers Include système standard,
// ou les fichiers Include spécifiques aux projets.

#pragma once

#include "lua_cpp98_compat.hpp"
#include <utf8.h>

#include <locale>
#include <sstream>
#include <fstream>
// #include <codecvt>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <unistd.h>

#include "HTMLEntityTransformer.h"

class Tags
{
    /*
     * tagNumber/tag
        None -1
        String -2
        OpenBracket -3
        ClosedBracket -4
        StraightLine -5

    */
public:
    // static const int nResult = -2; // during a conversion, certain character strings should only be converted once
    static const int nOpenBracket = -3;
    static const int nOpenedBracket = -3;
    static const int nClosedBracket = -4;
    static const int nStraightLine = -5;
    static const int nVerticalLine = -5;
    static const int nParameterBlocks = -7; // contains a complete tag
    static const int nArguments = -8; // contains a complete tag
    static const int nGroup = -9; // groups together successions of tags and strings (without [ | ])
    static const int nTag = -6; // tag
    // static const int nMATag = -11; // tag argument to complete
 static const int nNone = -12;
    // we do not define a tag when the parameters are not complete
    static const int nString = -1; // string in parameters

    struct Entity
    {
     public:
        int tagNumber;
        std::string str; //nul when the entity is a tag
        std::vector<struct Entity> entityList;

        Entity(int num, const std::string& s, const std::vector<Entity>& ents = std::vector<Entity>())
            : tagNumber(num), str(s), entityList(ents) {}
    };

    // Container typedefs
    typedef std::vector<Entity> EntityVector;
    typedef EntityVector::size_type EntityIndex;
    
    // Iterator typedefs
    typedef EntityVector::iterator EntityIterator;
    typedef EntityVector::const_iterator EntityConstIterator;
    
    // Common pairs
    typedef std::pair<EntityIndex, EntityIndex> EntityIndexPair;
    typedef std::pair<EntityIterator, EntityIterator> EntityIteratorPair;

    struct info
    {
    public:
        std::string sep;
        int entry;
        int pos;

       info(const std::string& s, int ent, int p)
            : sep(s), entry(ent), pos(p) {}
    };

struct Position {
public:
  std::vector<Tags::Entity>* list;
  std::vector<Tags::Entity>::size_type index;

// Constructor that takes a reference
  Position(std::vector<Tags::Entity>* vec, size_t idx) : list(vec), index(idx) {
if (list == 0) {
            std::cerr << "ERROR: Null pointer in Position constructor!" << std::endl;
        }
}
    // Custom copy constructor
    Position(const Position& other) : list(other.list), index(other.index) {}
    
    // Custom assignment operator
    Position& operator=(const Position& other) {
        list = other.list;
        index = other.index;
        return *this;
    }
};


    // Tag association table
    // 0 undefined
    // 1 SINGLE (macro or function)
    // 2 BEGIN_END
    // 3 BEGIN_MIDDLE_END (deleted)
    // 4 BEGIN_REPEATED_MIDDLE_END
    // 5 BEGIN_REPEATED_AT_LEAST_ONCE_MIDDLE_END
    // (not yet implemented)
    // 6 MULTIPLE_REPEATED_MIDDLE_BLOCKS

    static const int TUndefined = 0;
    static const int TSingle = 1;
    static const int TBeginEnd = 2;
    static const int TBeginMiddleEnd = 3;
    static const int TBeginRepeatedMiddleEnd = 4;

    static const int TMultipleRepeatedMiddleBlocks = 6;

    // Tag types in the same association.
    // "/" Beginning
    // "1", ..., "9" from 1 to 9 intermediate tag number
    // only "1" and "2" are currently being used
    // "." end

    static const std::string TB;
    static const std::string TE;

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
        new string[]{ "2", "u","/u"},
        new string[]{ "2", "s","/s"},
        new string[]{ "2", "size","/size"},
        new string[]{ "2", "style","/style"},
        new string[]{ "2", "color","/color"},
        new string[]{ "2", "center", "/center"},
        new string[]{ "2", "left","/left"},
        new string[]{ "2", "right","/right"},
        new string[]{ "2", "quote","/quote"},
        new string[]{ "2", "urU","/urU"},
        new string[]{ "2", "img","/img"},
        new string[]{ "2", "uU","/uU"},
        new string[]{ "2", "oU","/oU"},
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
    static std::vector< std::vector<std::string> > tagInfoList;
    // list of information about tags
    static std::vector<std::string> commandList;
    // list of commands associated with tags
    static std::vector<std::string> functionNameList;
    // command name list

    static std::vector<std::string> scriptList;
    // list of functions of global scope

    static std::vector< std::vector<int> > tagAssociationList;
    static std::vector<std::string> tagList;
    static std::vector<int> tagEntryList;
    static std::vector<int> tagTypeList;
    static std::vector<int> tagPositionList;
    static std::vector<Entity> document;

    static std::vector<std::vector<Position> > tagListList;

    public:
        static std::string SubStr(std::string& s, std::string::size_type n);
        static void scan_utf8_file(const std::string& text);
        static std::string readFile(std::string& file_name);
        static std::string Trim(std::string& str);
        static std::vector<std::string> split(const std::string& s, const char delimiter);
        static void LoadMacros(std::string psFileName);
        static void Init();

        static int CharToTagNumber(char c);

        static std::string TagNumberToString(int t);
        static std::string CleanTag(std::string& t);

        static bool utf8_less(const std::string& a, const std::string& b);
        static int binary_search_utf8(const std::vector<std::string>& list, const std::string& key);

        static bool is_unicode_whitespace(uint32_t cp);
        static std::string utf8_rtrim(const std::string& utf8_str);

        static void check_utf8(const std::string& s);

        static bool is_associated_intermediate(int index, int next_index);
        static bool is_associated_last(int index, int next_index);

        static void create_tag(std::vector<Tags::Entity>::size_type i, int index, std::vector<Tags::Entity>& tokens, std::vector<EntityIndexPair >& parameter_list);
        static void add_parameter_block(std::vector<Tags::Entity>::size_type i, std::vector<Tags::Entity>& tokens, std::vector<EntityIndexPair >& parameter_list);
        static void add_argument(std::vector<Tags::Entity>::size_type i, std::vector<Tags::Entity>& tokens, const EntityIndexPair argument);

        static bool check_tag(std::vector<Tags::Entity>::size_type i, const std::vector<Tags::Entity>& tokens, int& index, 
               std::vector<EntityIndexPair >& parameter_list, std::vector<Tags::Entity>::size_type& j);
        static bool check_arg(std::vector<Tags::Entity>::size_type i, const std::vector<Tags::Entity>& tokens, EntityIndexPair& argument, std::vector<Tags::Entity>::size_type& j);

        static void BBCodeToTree();

        static std::vector<std::vector<std::string> > GetParameters(const Tags::Entity& tag);
        static std::vector<std::string> GetArguments(const Tags::Entity& tag);
        static void display_parameters(std::vector<std::vector<std::string> > p);
        static void display_arguments(std::vector<std::string> arguments);
        static std::string DocumentToHTML();
        static std::string DocumentToFile(const std::string& fileName);
        static void EvalTree();

        private:
            static bool utf8_compare(const std::string& str1, const std::string& str2);
            static std::string TagToString(const Tags::Entity& e);

            static std::string HTMLEntities(std::string s);
            static void SymbolTagToString(std::vector<Tags::Entity>& eList, int pos);
            static std::string StringifyMATag(const Tags::Entity& e);

            static void DisplayEntity(std::vector<Tags::Entity> document);
            static void DisplayEntityTag(Tags::Entity& e);
};
