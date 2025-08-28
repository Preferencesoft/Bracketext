/*
BSD 2-Clause License

Copyright (c) 2023, Jean-Claude COLETTE

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include "Bracketext.h"
#include "HTMLEntityTransformer.h"
#include "Tags.h"

int main(int argc, char *argv[]) {
    int err = Bracketext::Main(argc, argv);
    // Bracketext::Test();
    /*
       HTMLEntityTransformer transformer;

        // Test string with protected zones and various characters
        std::string test_input = "Price: €100 < 200 & 'special' \1Protected:
       <>&€\4 © 2023";

        std::string result = transformer.transform(test_input);

        std::cout << "Input: " << test_input << std::endl;
        std::cout << "Output: " << result << std::endl;

        // Additional test cases
        std::vector<std::string> test_cases = {
            "Simple: <>&\"'",
            "Multiple zones: \1Keep this <>&\4 and transform this <>&",
            "\1Fully protected\4",
            "No protection: €100 < 200",
            "Nested? \1First \1Second\4 First continues\4", // Note: This
       doesn't handle nesting "Mixed: © & \1<>&\4 & ©"
        };

        for (const auto& test : test_cases) {
            std::cout << "\nTest: " << test << std::endl;
            std::cout << "Result: " << transformer.transform(test) << std::endl;
        }
    */
    return 0;
}
