#include <network-monitor/file-downloader.h>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(file_downloader)
{
    const std::string fileUrl {
        "https://ltnm.learncppthroughprojects.com/network-layout.json"
    };
    const auto destination {
        std::filesystem::temp_directory_path() / "network-layout.json"
    };

    // Download the file.
    bool downloaded {NetworkMonitor::DownloadFile(fileUrl, destination, TESTS_CACERT_PEM)};
    BOOST_CHECK(downloaded);
    BOOST_CHECK(std::filesystem::exists(destination));

    // Check the content of the file.
    // We cannot check the whole file content as it changes over time, but we
    // can at least check some expected file properties.
    {
        const std::string expectedString {"\"stations\": ["};
        std::ifstream file {destination};
        std::string line {};
        bool foundExpectedString {false};
        while (std::getline(file, line)) {
            if (line.find(expectedString) != std::string::npos) {
                foundExpectedString = true;
                break;
            }
        }
        BOOST_CHECK(foundExpectedString);
    }

    // Clean up.
    std::filesystem::remove(destination);
}

BOOST_AUTO_TEST_CASE(parse_json_file)
{
    std::filesystem::path jsonFile{TESTS_NETWORK_LAYOUT_JSON};
    nlohmann::json parsedFile = NetworkMonitor::ParseJsonFile(jsonFile);
    BOOST_CHECK(parsedFile.is_object());
    BOOST_CHECK(parsedFile.contains("lines"));
    BOOST_CHECK(parsedFile.at("lines").is_array());
    BOOST_CHECK(parsedFile.at("lines").size() > 0);
    BOOST_CHECK(parsedFile.contains("stations"));
    BOOST_CHECK(parsedFile.at("stations").is_array());
    BOOST_CHECK(parsedFile.at("stations").size() > 0);
    BOOST_CHECK(parsedFile.contains("travel_times"));
    BOOST_CHECK(parsedFile.at("travel_times").is_array());
    BOOST_CHECK(parsedFile.at("travel_times").size() > 0);
}

BOOST_AUTO_TEST_SUITE_END();