#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <regex>


namespace tc::io {

using namespace catalogue;
using namespace geo;


void ParseStopCommands(std::string_view str, Coordinates& coordinates, std::vector<std::pair<std::string_view, int>>& dist) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        coordinates = {nan, nan};
        return;
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);
    auto comma2 = str.find(',', not_space2 + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2, comma2 - not_space2)));

    coordinates = {lat, lng};

    auto start_dist = comma2;
    std::match_results<std::string_view::const_iterator> m;
    const static std::regex reg(R"(,\s*(\d+\.?\d*)m\s+to\s+((?:\w[\w\s]*\w)|(?:\w))s*,?)");
    while (true) {
        if(start_dist >= str.length()) {
            break;
        }
        std::string_view cur_substr = str.substr(start_dist);
        if(!std::regex_search(cur_substr.begin(), cur_substr.end(), m, reg))
        {
            break;
        }
        dist.emplace_back(std::string_view(m[2].first, m[2].second - m[2].first),
                          std::stod(std::string(m[1].first, m[1].second)));
        start_dist += m[0].second - m[0].first;
        if(m[0].second != str.end()){
            --start_dist;
        }
    }
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue &catalogue) const {
    std::unordered_map<std::string_view, std::vector<std::pair<std::string_view, int>>> distances;

    for (const auto &command: commands_) {
        if (command.command == "Stop") {
            Coordinates coords;
            std::vector<std::pair<std::string_view, int>> dist;
            ParseStopCommands(command.description, coords, dist);
            catalogue.AddStop(command.id, coords);
            distances[command.id] = std::move(dist);
        }
    }

    for(const auto& [id, dist]: distances) {
        catalogue.SetDistances(id, dist);
    }

    for (const auto &command: commands_) {
        if (command.command == "Bus") {
            catalogue.AddBus(command.id, ParseRoute(command.description));
        }
    }
}

}