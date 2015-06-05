#ifndef TERRAGEN_FILE_SPEC_H
#define TERRAGEN_FILE_SPEC_H

#include <string>

struct TerragenFileHeaderData{
public:
    TerragenFileHeaderData() : mode(0), scale(30.f), planet_radius(6370) {} // DEFAULTS

    int width, depth, mode;
    float scale, planet_radius, height_scale, base_height, min_height, max_height;
};

class TerragenFile
{
public:
    TerragenFile();
    TerragenFile(const std::string &filename); // Load from file
    ~TerragenFile();

    void summarize();
    bool write(const std::string & filename);

    float & operator()(int x, int z){
        return m_height_data[z * m_header_data.width + x];
    }

    TerragenFileHeaderData m_header_data;
    float* m_height_data;
};

#endif // TERRAGEN_FILE_SPEC_H
