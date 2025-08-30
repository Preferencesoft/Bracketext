// Bracketext.cpp : définit le point d'entrée de l'application.
//

#include "Bracketext.h"
#include "version.h"

using namespace std;

void Bracketext::Test() {

  Tags::LoadMacros(
      "/home/<user>/Documents/programming/projects-cpp/bracketext/src/macros.txt");
  Tags::Init();
  // Tags::ScanFile("/home/<user>/Documents/programming/projects-cpp/bracketext/src/example.txt");
  std::string str =
      "/home/<user>/Documents/programming/projects-cpp/bracketext/src/example.txt";
  Tags::scan_utf8_file(Tags::readFile(str));

  Tags::TagsToTree();
  Tags::EvalTree();
  cout << Tags::DocumentToHTML();
}

int Bracketext::Main(int argc, char *argv[]) {

   // Test();

    std::string macroFile;
    std::string inputFile;
    std::string outputFile;
    
    int opt;
    while ((opt = getopt(argc, argv, "m:f:o:v")) != -1) {
        switch (opt) {
            case 'm':
                macroFile = optarg;
                break;
            case 'f':
                inputFile = optarg;
                break;
            case 'o':
                outputFile = optarg;
                break;
           case 'v':
                std::cout << "Application Version: " << PROJECT_VERSION << std::endl;
                std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
                return 0;
            case '?':
                std::cerr << "Unknown option: " << char(optopt) << "\n";
                return 1;
            default:
                std::cerr << "Usage: " << argv[0] << " -m <macro_file> -f <input_file> -o <output_file> [-v]\n";
                return 1;
        }
    }
    // *** Test ***
    // cout << ">>" << inputFile << endl << ">>" << outputFile << endl << ">>" << macroFile << endl;
    
       // Validate mandatory options
    if (macroFile.empty() || inputFile.empty() || outputFile.empty()) {
        std::cerr << "Error: Missing required options\n";
        std::cerr << "Usage: " << argv[0] << " -m <macro> -f <input_file> -o <output_file> [-v]\n";
        
        if (macroFile.empty()) std::cerr << "  -m option is required\n";
        if (inputFile.empty()) std::cerr << "  -f option is required\n";
        if (outputFile.empty()) std::cerr << "  -o option is required\n";
        
        return 1;
    }


  Tags::LoadMacros(macroFile);
  cout << "macros loaded" << endl;
  Tags::Init();
  std::string str = inputFile;
  Tags::scan_utf8_file(Tags::readFile(str));
  Tags::TagsToTree();
  Tags::EvalTree();
  cout << Tags::DocumentToFile(outputFile);
  // cout << Tags::DocumentToHTML();

  return 0;
}
