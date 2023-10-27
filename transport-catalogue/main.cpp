#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

void run(std::istream& in_stream, std::ostream& out_stream) {
    tc::catalogue::TransportCatalogue catalogue;

    int base_request_count;
    in_stream >> base_request_count >> ws;

    {
        tc::io::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(in_stream, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    in_stream >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(in_stream, line);
        tc::io::ParseAndPrintStat(catalogue, line, out_stream);
    }
}

int main() {
    run(cin, cout);
}