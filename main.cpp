#include "download.h"

#include <cstdlib>
#include <string>
#include <iostream>

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {

    std::string url{"https://www.boost.org/style-v2/css_0/get-boost.png"};
//    std::string url{"https://www.boost.org/style-v2/css_0/theme_grass/header-fg.png};

    if (download(url)) {
        std::cout << "Downloading file: " << filenameFromUrl(url) << " is well done!" << std::endl;
    } else {
        std::cout << "ERROR!!!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
