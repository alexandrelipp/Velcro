//
// Created by alexa on 2022-03-07.
//

#include "UtilsFile.h"

#include <fstream>
#include <iostream>

namespace utils{
    bool fileExists(const std::filesystem::path& path) {
        return std::filesystem::exists(path);
    }

    
    std::vector<char> getFileContent(const std::string& path) {
        std::ifstream fstream;
        fstream.open(path, std::ios::binary | std::ios::ate);
        if (!fstream.is_open()){
            SPDLOG_ERROR("Could not get file content at path {}", path);
            return {};
        }

        auto size = fstream.tellg();
        std::vector<char> output(size);

        fstream.seekg(0);
        fstream.read(output.data(), size);

        return output;
    }

}
