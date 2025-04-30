
#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "tinyxml2.h"
#include "svg.hpp"
using namespace tinyxml2;

struct Node {
    double lat, lon;
};

struct Way {
    std::vector<long long> node_refs;
    bool is_highway = false;
};

std::map<long long, Node> nodes;
std::vector<Way> ways;

double min_lat = 1e9, max_lat = -1e9;
double min_lon = 1e9, max_lon = -1e9;

void parse_osm(const char* filename) {
    XMLDocument doc;
    if (doc.LoadFile(filename) != XML_SUCCESS)
        throw std::runtime_error("Failed to read OSM file");

    XMLElement* root = doc.RootElement();
    for (XMLElement* elem = root->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        std::string tagName = elem->Name();
        if (tagName == "node") {
            long long id = std::stoll(elem->Attribute("id"));
            double lat = std::stod(elem->Attribute("lat"));
            double lon = std::stod(elem->Attribute("lon"));
            nodes[id] = {lat, lon};
            min_lat = std::min(min_lat, lat);
            max_lat = std::max(max_lat, lat);
            min_lon = std::min(min_lon, lon);
            max_lon = std::max(max_lon, lon);
        } else if (tagName == "way") {
            Way way;
            bool is_hw = false;
            for (XMLElement* child = elem->FirstChildElement(); child; child = child->NextSiblingElement()) {
                std::string childName = child->Name();
                if (childName == "nd") {
                    long long ref = std::stoll(child->Attribute("ref"));
                    way.node_refs.push_back(ref);
                } else if (childName == "tag") {
                    const char* k = child->Attribute("k");
                    const char* v = child->Attribute("v");
                    if (k && std::string(k) == "highway") {
                        is_hw = true;
                    }
                }
            }
            way.is_highway = is_hw;
            ways.push_back(way);
        }
    }
}

int scale(double value, double minv, double maxv, int size) {
    return static_cast<int>((value - minv) / (maxv - minv) * (size - 1));
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input.osm> <output.bmp>\n";
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    parse_osm(input_file);
    int width = 2000, height = 2000;
    svg image(output_file,width, height);

    for (const auto& way : ways) {
        for (size_t i = 1; i < way.node_refs.size(); ++i) {
            auto it1 = nodes.find(way.node_refs[i - 1]);
            auto it2 = nodes.find(way.node_refs[i]);
            if (it1 == nodes.end() || it2 == nodes.end()) continue;
            int x1 = scale(it1->second.lon, min_lon, max_lon, width);
            int y1 = height - scale(it1->second.lat, min_lat, max_lat, height);
            int x2 = scale(it2->second.lon, min_lon, max_lon, width);
            int y2 = height - scale(it2->second.lat, min_lat, max_lat, height);
            color clr = way.is_highway ? color(255, 0, 0) : color(0, 0, 0);
            image.draw_line( x1, y1, x2, y2, clr);
        }
    }

    return 0;
}
