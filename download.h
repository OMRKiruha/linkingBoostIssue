//
// Created by Professional on 07.09.2024.
//
#include <string>
#include <cstdint>


[[nodiscard]] bool download(std::string_view url);

int32_t getFileSize(std::string_view url);

std::string filenameFromUrl(std::string_view url);