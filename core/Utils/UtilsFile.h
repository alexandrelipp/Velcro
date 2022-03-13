//
// Created by alexa on 2022-03-07.
//

#pragma once

#include <filesystem>


namespace utils{
    bool fileExists(const std::filesystem::path& path);

    std::vector<char> getFileContent(const std::string& path);
}


