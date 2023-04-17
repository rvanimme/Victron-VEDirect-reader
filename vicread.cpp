
/*
 * ----------------------------------------------------------------------------
 * Author: Ronald van Immerzeel
 * Version: V1.1, 2023-04-16
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
#include <iomanip>
#include <regex>
#include <chrono>

// C header files
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>
#include <cstdio>

// Linux header files
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/file.h>

#define constexpr_strlen(s) (sizeof(s) - 1) // The -1 is to exclude the null terminator

// The VE.Direct protocol is described in the VE.Direct Protocol Specification
// document. The document is available from Victron Energy.
//
// The grammer below is based on observations of 5 VE.Direct devices.
// 1) SmartShunt 500A/50mV
// 2) SmartSolar MPPT 150/85 rev2
// 3) SmartSolar MPPT 150|70 rev2
// 4) SmartSolar MPPT 100|50
// 5) BMV-712 Smart - Battery monitor (BMV)
//
// THERE WILL BE DEVICES NOT MEETING THIS GRAMMAR!
//
// BNF Grammar:
// <ve_direct_line> ::= <capitalized-name> <TAB> <number-value> <CRLF>
//                   | <capitalized-name> <TAB> <hex-value> <CRLF>
//                   | <capitalized-name> <TAB> "ON" <CRLF>
//                   | <capitalized-name> <TAB> "OFF" <CRLF>
//                   | <capitalized-name> <TAB> "---" <CRLF>
//                   | "BMV" <TAB> <capitalized-string-value> <CRLF>
//                   | "SER#" <TAB> <capitalized-string-value> <CRLF>
//                   | "FWE" <TAB> <capitalized-string-value> <CRLF>
//
// <capitalized-name> ::= <uppercase-letter> <capitalized-name-rest>*
// <capitalized-name-rest> ::= <uppercase-letter>
//                          | <lowercase-letter>
//                          | <digit>
//
// <capitalized-string-value> ::= <uppercase-letter> <capitalized-string-value-rest>*
//                             | <digit> <capitalized-string-value-rest>*
// <capitalized-string-value-rest> ::= <uppercase-letter>
//                                  | <lowercase-letter>
//                                  | <digit>
//                                  | "/"
//                                  | " "
//
// <TAB> ::= "\t"
// <CRLF> ::= "\r\n"  
//
// <number-value> ::= <signed-integer>
// <signed-integer> ::= <digit>+
//                   | "-" <digit>+
//
// <hex-value> ::= "0x" <hex-digit>+
//
// <uppercase-letter> ::= "A" | "B" | "C" | ... | "Z"
// <lowercase-letter> ::= "a" | "b" | "c" | ... | "z"
// <digit> ::= "0" | "1" | "2" | ... | "9"
// <hex-digit> ::= <digit> | "A" | "B" | "C" | "D" | "E" | "F"


// Regex pattern explanation:
// ^ and $: Anchor the pattern to the start and end of the line.
// (?: ...): Non-capturing group.
// [A-Z][A-Za-z0-9]*: Matches a capitalized name.
// \t: Matches a tab character.
// (?:-?\d+|0x[A-F0-9]+|ON|OFF|---): Matches a number value, hex value, ON, OFF, or ---.
// |: Alternation (OR) operator.
// BMV: Matches the string "BMV".
// SER#: Matches the string "SER#".
// [A-Z][A-Za-z0-9/ ]*: Matches a capitalized string value.
// \r\n: Matches a carriage return.
//                                    1  
static std::regex ve_direct_line_regex(
    "^(?:"
        "(?:[A-Z][A-Za-z0-9]*\\t(?:-?[0-9]+|0x[A-F0-9]+|ON|OFF|---))"   // [A-Z][A-Za-z0-9]*: Matches a capitalized name. (?:-?\d+|0x[A-F0-9]+|ON|OFF|---): Matches a number value, hex value, ON, OFF, or ---.
    "|"
        "(?:BMV|SER#|FWE)\\t[A-Z0-9][A-Za-z0-9/ ]*"                     // BMV: Matches the string "BMV", "SER#"" or "FWE". [A-Z0-9][A-Za-z0-9/ ]*: Matches a capitalized string value.
    ")\\r$"                                                             // Note that the \n is not included because reading a line on Linux discards the \n.
);

// Some statistics about the errors we encounter
static unsigned int chksum_errors = 0;
static unsigned int format_errors = 0;
static unsigned int valid_blocks = 0;
static unsigned long received_bytes = 0;   // This will take a while to overflow :-)

#include <chrono>
#include <iomanip>

static void print_error_info(std::string const &first_line) {

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&timestamp), "%FT%T%z");

    if (valid_blocks == 0) {
        // We only start counting the discarded blocks after we have received the first valid block
        std::cerr   << "[" << oss.str() << "] " << first_line << " "
                    << "Waiting for first valid block. "
                    << "Received bytes: " << received_bytes
                    << std::endl;
    } else {
        unsigned int t = valid_blocks + chksum_errors + format_errors;
        // Checksum error. Print message on stderr and continue with the next block
        std::cerr   << "[" << oss.str() << "] " << first_line << " "
                    << "Received bytes: " << received_bytes << ", "
                    << "total blocks: " << t << ", "
                    << "valid blocks: " << valid_blocks << " (" << 100.0f * valid_blocks / t << "%), "
                    << "checksum errors: " << chksum_errors << " (" << 100.0f * chksum_errors / t << "%), "
                    << "format errors: " << format_errors << " (" << 100.0f * format_errors / t << "%)"
                    << std::endl;
    }
    return;
}

int main(int argc, char *argv[])
{
    // Set the precision of the floating point numbers
    std::cerr << std::fixed << std::setprecision(2);

    if (argc < 2) {
        std::cerr << "This application reads the VE.Direct protocol from a serial device and prints the data to stdout." << std::endl;
        std::cerr << "All error and information messages are sent to stderr." << std::endl;
        std::cerr << "The VE.Direct protocol is used by Victron Energy devices to communicate with a host computer." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Usage: " << argv[0] << " <serial_device> [<white_list_filter>]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "The serial device is typically a USB to serial adapter (e.g. /dev/ttyUSB0) connected to the VE.Direct port on the device." << std::endl;
        std::cerr << "The white list filter is list of field names (e.g. \"P,SOC\" or \"P SOC\") that will be printed on stdout." << std::endl;
        std::cerr << "You can use commas and/or spaces to separate the names. Quotes are optional." << std::endl;
        std::cerr << "The names in the filter are case sensitive. If no filter is specified, all fields are printed." << std::endl;
        std::cerr << std::endl;
        std::cerr << "Any block with a checksum error will be discarded." << std::endl;
        std::cerr << "As the checksum coverage isn't very strong an additional layer of checking has been added based on the grammar" << std::endl;
        std::cerr << "Any lines that don't meet the expected format/grammer will result in a discard of the current block" << std::endl;
        return -1;
    }

    // The filter list can be provided in different ways. 
    // You can use commas and or spaces to separate the names. Using quotes is optional
    std::string filter{','};        // Default to no white list filter
    {
        // Add all arguments to the filter list
        int argnr = 2;
        while (argnr < argc) {
            filter += argv[argnr++];
            filter += ',';
        }
        // Now do some cleanup of the list. We want to replace all spaces with commas and remove any consecutive commas
        std::replace(filter.begin(), filter.end(), ' ', ',');
        filter.erase(std::unique(filter.begin(), filter.end(), [](char a, char b) { return a == ',' && b == ','; }), filter.end());
    }
    if (filter == ",") {
        std::cerr << "No white list filter used" << std::endl;
        filter.clear();
    } else {
        std::cerr << "Using white list filter: \"" << filter.substr(1, filter.size() - 2) << "\"" << std::endl;
    }
    

    // Print serial device name with double quotes.
    std::cerr << "Using serial device: \"" << argv[1] << "\"" << std::endl;
    
    int fd;
    fd = open(argv[1], O_RDONLY | O_NOCTTY );   // Open the serial device in read only mode and don't make it the controlling terminal
    if (fd == -1) {
        std::cerr << "Error opening the serial device: " << std::strerror(errno) << std::endl;
        return -1;
    }

    // Lock the serial device for exclusive access
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        std::cerr << "Error locking the serial device: " << std::strerror(errno) << std::endl;
        close(fd);
        return -1;
    }

    // Serial port configuration for VE.Direct
    {
        struct termios tio;
        memset(&tio, 0, sizeof(tio));
        tio.c_cflag = CS8 | CREAD | CLOCAL;
        tio.c_iflag = IGNPAR;
        tio.c_oflag = 0;
        tio.c_lflag = 0;
        tio.c_cc[VMIN] = 1;
        tio.c_cc[VTIME] = 0;
        cfsetispeed(&tio, B19200);
        cfsetospeed(&tio, B19200);

        tcflush(fd, TCIFLUSH);
        if (tcsetattr(fd, TCSANOW, &tio) != 0) {
            std::cerr << "Error configuring the serial device: " << std::strerror(errno) << std::endl;
            flock(fd, LOCK_UN);
            close(fd);
            return -1;
        }
    }

    std::string readcharbuf;
    while (true) {
        char buf[512];
        int n = read(fd, buf, 512); // blocking read, return buf with 1..512 bytes in it
        assert(n > 0);              // Just stop if we get 0 or negative (=error)
        readcharbuf.append(buf, n); // Append the Received bytes to the end of the existing std::string buffer

        // From: VE.Direct-Protocol-3.32.pdf
        // " Some products will send Asynchronous HEX-messages, starting with “:A” and ending with a 
        //   newline ‘\n’, on their own. These messages can interrupt a regular Text-mode frame. "
        {
            // Remove any HEX-messages from the readcharbuf.
            size_t starthex, endhex;
            while ((starthex = readcharbuf.find(":A")) != std::string::npos && (endhex = readcharbuf.find("\n", starthex)) != std::string::npos) {
                readcharbuf.erase(starthex, endhex - starthex + 1);
            }
        }
        size_t startcheck;
        // Find the "Checksum" message so we know where a block ends. Assume the start of a block is at index 0 in the readcharbuf (this probably not the case the first time when we start reading)
        // Makes sure the readcharbuf length is big enough to contain the "Checksum\t" message and the checksum byte
        while (((startcheck = readcharbuf.find("Checksum\t")) != std::string::npos) && (readcharbuf.length() >= startcheck + constexpr_strlen("Checksum\t") + 1)) {

            // Extract the checksum block from the readcharbuf and remove it from the readcharbuf
            auto block = readcharbuf.substr(0, startcheck + constexpr_strlen("Checksum\t") + 1); //  +1 for the actual checksum byte
            readcharbuf.erase(0, startcheck + constexpr_strlen("Checksum\t") + 1); 
            received_bytes += block.length();

            // Calculate the modulo 256 sum of all the bytes in this block
            // The used checksum is very weak, e.g. if 2 characters have a bit-7 flip, it cannot be detected. 
            unsigned char sum = 0;
            for (std::size_t i = 0; i < block.length(); i++)
                sum += block[i];

            if (sum != 0) {
                if (valid_blocks > 0)
                    chksum_errors++;
                print_error_info("ERROR checksum, block discarded.");
                continue;
            }

            // Get rid of the checksum line (the checksum identifier and value have no further use)
            block.erase(startcheck);
            // The block now ends with \r\n (this is what we want)
            // Delete the first \r\n of this block.

            // Check if the first 2 characters are \r\n
            if (block[0] != '\r' || block[1] != '\n') {
                if (valid_blocks > 0)
                    format_errors++;
                print_error_info("ERROR format, first 2 characters of block are not \\r\\n, block discarded.");
                continue;
            }
            block.erase(0,2);

            // Extract line by line from the block and check if the line matches the grammar
            // If there is a grammer error, discard the whole block (we cannot trust anything in this block)
            {
                bool format_error = false;
                std::istringstream iss{block};
                std::string line;
                while (std::getline(iss, line)) {
                    if (!std::regex_search(line, ve_direct_line_regex)) {
                        format_error = true;
                        break;
                    }
                }
                if (format_error) {
                    // Remove any \r characters in the line (it messes up the output)
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    if (valid_blocks > 0)
                        format_errors++;
                    print_error_info("ERROR format, line \"" + line + "\", block discarded.");
                    continue;
                }
            }

            valid_blocks++;

            std::istringstream iss{block};
            std::string line;
            while (std::getline(iss, line)) {   // getline removes the \n character at the end of the line
                // Remove \r character at the end (this is Linux so we use \n which will be added by std::cout)
                assert(line.back()=='\r');
                line.erase(line.end() - 1);

                size_t tab_pos = line.find('\t');
                assert(tab_pos != std::string::npos);
                std::string name = line.substr(0, tab_pos);
                assert(name.size() > 0);
                std::string value = line.substr(tab_pos + 1);
                assert(value.size() > 0);

                if (filter.empty() || filter.find(',' + name + ',') != std::string::npos) {
                    std::cout << name << '\t' << value << std::endl;
                }
            }
        }
    }

    flock(fd, LOCK_UN);
    close(fd);
    return 0;
}
