#include "terragen_file.h"
#include <iostream>
#include "terragen_file_manager.h"

TerragenFile::TerragenFile() : m_height_data(NULL)
{

}

TerragenFile::TerragenFile(const std::string & filename) : TerragenFile(TerragenFileManager::readTerragen(filename))
{

}

TerragenFile::~TerragenFile()
{

}

bool TerragenFile::write(const std::string & filename)
{
    return TerragenFileManager::writeTerragen(*this, filename);
}

void TerragenFile::summarize()
{
    std::cout << "***** TERRAGEN FILE SUMMARY *****" << std::endl;
    std::cout << "Width: " << m_header_data.width << std::endl;
    std::cout << "Height: " << m_header_data.depth << std::endl;
    std::cout << "Mode: " << m_header_data.mode << std::endl;
    std::cout << "Minimum height: " << m_header_data.min_height << std::endl;
    std::cout << "Max height: " << m_header_data.max_height << std::endl;
    std::cout << "Scale: " << m_header_data.scale << std::endl;
    std::cout << "HeightScale: " << m_header_data.height_scale << std::endl;
    std::cout << "BaseHeight: " << m_header_data.base_height << std::endl;
    std::cout << "Planet Radius: " << m_header_data.planet_radius << std::endl;
    std::cout << "*********************************" << std::endl;
}
