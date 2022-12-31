#pragma once

#include "json.hpp"

class Config {
    nlohmann::json j;
    std::string    m_fileName;

    void restoreAndCheckKeysCorrect();

public:
    Config() = delete;
    explicit Config(std::string fileName);
    ~Config();

    nlohmann::json& operator[](const char* key) {
        return j[key];
    }

    void saveFile() const noexcept;
};
