#include <iostream>

#include "json_reader.h"

using namespace std;

int main() {
    tc::catalogue::TransportCatalogue catalogue;
    tc::renderer::MapRenderer renderer;
    tc::io::JsonReader reader;
    tc::RequestHandler handler(catalogue, renderer);

    reader.ParseInput(cin);
    reader.ApplyCommands(catalogue, renderer);
    reader.GetOutput(handler, cout);
}