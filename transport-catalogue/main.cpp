#include <iostream>

#include "json_reader.h"

using namespace std;

int main() {
    tc::catalogue::TransportCatalogue catalogue;
    tc::renderer::MapRenderer renderer;
    tc::routing::TransportRouter router;
    tc::io::JsonReader reader;
    tc::RequestHandler handler(catalogue, renderer, router);

    reader.ParseInput(cin);
    reader.ApplyCommands(catalogue, renderer, router);
    reader.GetOutput(handler, cout);
}