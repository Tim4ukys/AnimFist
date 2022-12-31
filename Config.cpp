#include "Config.h"
#include <filesystem>
#include <fstream>

Config::Config(std::string fileName)
    : m_fileName(std::filesystem::current_path().string() + '\\' + fileName) {
    std::filesystem::path cfile{m_fileName};
    if (std::filesystem::exists(cfile)) {
        std::ifstream fconf{m_fileName};
        fconf >> j;
    }

    restoreAndCheckKeysCorrect();
    saveFile();
}
Config::~Config() {
    saveFile();
}

void Config::saveFile() const noexcept {
    std::ofstream oFile{m_fileName};
    oFile << j.dump(4);
}

#define SET_DEFAULT_STR(js, str_key, str_def) \
    if (!(js)[str_key].is_string()) { \
        (js)[str_key] = str_def; \
    }

void Config::restoreAndCheckKeysCorrect() {
    SET_DEFAULT_STR(j, "fileName", "NONE")
}