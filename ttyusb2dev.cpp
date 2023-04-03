
/*
 * ----------------------------------------------------------------------------
 * Author: Ronald van Immerzeel
 * Version: V1.0, 2023-04-02
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * ----------------------------------------------------------------------------
 *
 * This software is dedicated to the public domain under the CC0 1.0 Universal
 * (CC0 1.0) Public Domain Dedication. To the extent possible under law, the
 * author(s) have dedicated all copyright and related and neighboring rights
 * to this software to the public domain worldwide. This software is
 * distributed without any warranty.
 *
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */

// C++ header files
#include <iostream>
#include <filesystem>
#include <ranges>
#include <vector>
#include <regex>

// C++ header files

// Linux header files
#include <fcntl.h>
#include <unistd.h>

static std::vector<std::string> list_ttyUSB_links(const std::string& directory) {

    namespace fs = std::filesystem;

    auto dir = fs::directory_iterator(directory);

    auto filter = [](const auto& entry) {
        auto path = entry.path();
        return fs::is_symlink(path) && path.filename().string().starts_with("ttyUSB");
    };

    auto transform = [](const auto& path) {
        return fs::read_symlink(path).string();
    };

    // Use ranges to filter and transform the directory entries
    auto filtered = dir | std::views::filter(filter) | std::views::transform(transform);

    // Create an empty vector
    std::vector<std::string> result;

    // We cannot return filtered directly because dir variable is out of scope (filtered refers to dir)
    // So copy the filtered output to a vector of strings
    for (const auto& item : filtered)
        result.push_back(item);

    return result;
}

static void print_ttyUSB_list(const std::vector<std::string>& ttyusblist) {

    constexpr int column_width = 16;

    // Extract the physical address and the ttyUSB device name
    std::regex pattern(R"(^.*/([0-9.-]+):.*?(ttyUSB[0-9]+)$)");
    std::smatch matches;


    // Loop over the ttyusb list
    std::cerr << "Available ttyUSB device" << std::endl;
    std::cerr << std::endl;
    std::cerr << std::left << std::setw(column_width) << "Physical" << "Device" << std::endl;
    for (const auto& device_link : ttyusblist) {
        if (std::regex_search(device_link, matches, pattern)) {
            std::cerr << std::left << std::setw(column_width) << matches[1] << matches[2] << std::endl;
        }
    }
}

int main(int argc, char *argv[])
{
    auto ttyusblist = list_ttyUSB_links("/sys/class/tty");

    if (argc != 2) {
        std::cerr << "This application returns the full device path based on the device name or physical address of a serial USB device." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Usage: " << argv[0] << " [ <device_name | physical_address> ]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "device_name is the name of the serial USB device (e.g. ttyUSB0 or ttyUSB5)." << std::endl;
        std::cerr << "physical_address is the topology based address of the serial USB device (e.g. 3-2.3 or 1-1.1.3)" << std::endl;
        std::cerr << std::endl;
        std::cerr << "In case the device exists, the full path to the device is send to stdout" << std::endl;
        std::cerr << "In case the device does not exists or no argument is provided, a list of all tty USB devices is printed" << std::endl;
        std::cerr << std::endl;
        print_ttyUSB_list(ttyusblist);
        return -1;
    }

    // First try to open the device with the given name (as is)
    int fd;
    fd = open(argv[1], O_RDONLY | O_NOCTTY );   // Open the serial device in read only mode and don't make it the controlling terminal
    if (fd >= 0) {
        std::cout << argv[1] << std::endl;
        close(fd);
        return 0;
    }

    // Next, add the /dev/ prefix and try to open the device
    std::string device_name = "/dev/" + std::string(argv[1]);
    fd = open(device_name.c_str(), O_RDONLY | O_NOCTTY );   // Open the serial device in read only mode and don't make it the controlling terminal
    if (fd >= 0) {
        std::cout << device_name << std::endl;
        close(fd);
        return 0;
    }

    // Last, try to find the device based on the physical address
    std::regex pattern(R"(^.*/([0-9.-]+):.*?(ttyUSB[0-9]+)$)");
    std::smatch matches;

    for (const auto& device_link : ttyusblist) {
        if (std::regex_search(device_link, matches, pattern)) {
            if (matches[1] == argv[1]) {
                std::cout << "/dev/" << matches[2] << std::endl;
                return 0;
            }
        }
    }

    // Not found, print the list of available devices
    std::cerr << "Device \"" << argv[1] << "\" not found" << std::endl;
    std::cerr << std::endl;
    print_ttyUSB_list(ttyusblist);
    return -1;
}
 