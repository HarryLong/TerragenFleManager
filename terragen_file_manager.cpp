/**
 * @file
 *
 * Terragen format import and export.
 * @see http://www.planetside.co.uk/terragen/dev/tgterrain.html
 */

#include <fstream>
#include <algorithm>
#include <limits>
#include <cstring>
#include <utility>
#include <stdexcept>
#include <locale>
#include <cstdint>
#include <cmath>
#include <iostream>
#include "terragen_file_manager.h"
#include <cfloat>
#include <math.h>

/********************
 * BIN MANIPULATION *
 ********************/
/// Reads a binary little-endian float
static float readFloat(std::istream &in)
{
    static_assert(std::numeric_limits<float>::is_iec559 && sizeof(float) == 4, "float is not IEEE-754 compliant");
    unsigned char data[4];
    std::uint32_t host = 0;
    float out;

    in.read(reinterpret_cast<char *>(data), 4);
    for (int i = 0; i < 4; i++)
        host |= std::uint32_t(data[i]) << (i * 8);
    std::memcpy(&out, &host, sizeof(out));
    return out;
}

/// Writes a binary little-endian float
static void writeFloat(std::ostream &out, float f)
{
    static_assert(std::numeric_limits<float>::is_iec559 && sizeof(float) == 4, "float is not IEEE-754 compliant");
    std::uint32_t host;
    unsigned char data[4];

    std::memcpy(&host, &f, sizeof(host));
    for (int i = 0; i < 4; i++)
        data[i] = (host >> (i * 8)) & 0xff;
    out.write(reinterpret_cast<char *>(data), 4);
}

/// Reads a binary little-endian 16-bit unsigned int
static std::uint16_t readUint16(std::istream &in)
{
    unsigned char data[2];
    in.read(reinterpret_cast<char *>(data), 2);
    return data[0] | (std::uint16_t(data[1]) << 8);
}

/// Writes a binary little-endian 16-bit unsigned int
static void writeUint16(std::ostream &out, std::uint16_t v)
{
    unsigned char data[2];
    data[0] = v & 0xff;
    data[1] = v >> 8;
    out.write(reinterpret_cast<char *>(data), 2);
}

/// Reads a binary little-endian 16-bit signed int
static std::int16_t readInt16(std::istream &in)
{
    return static_cast<std::int16_t>(readUint16(in));
}

/// Writes a binary little-endian 16-bit signed int
static void writeInt16(std::ostream &out, std::int16_t v)
{
    writeUint16(out, static_cast<std::uint16_t>(v));
}

static void write_debug_Int16(std::ostream &out, std::int16_t v)
{
    std::cout << "Original Value " << v << std::endl;
    std::uint16_t c_v (static_cast<std::uint16_t>(v));
    std::cout << "Casted Value " << c_v << std::endl;
    writeUint16(out, static_cast<std::uint16_t>(v));
}

//***************************************************************

const std::string Markers::SIGNATURE = "TERRAGENTERRAIN ";
const std::string Markers::XPTS = "XPTS";
const std::string Markers::YPTS = "YPTS";
const std::string Markers::SIZE = "SIZE";
const std::string Markers::SCAL = "SCAL";
const std::string Markers::CRAD = "CRAD";
const std::string Markers::CRVM = "CRVM";
const std::string Markers::ALTW = "ALTW";
const std::string Markers::EOF_ = "EOF ";

TerragenFile TerragenFileManager::readTerragen(const std::string &filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in)
        throw std::runtime_error("Could not open " + filename);

    TerragenFile ret;
    try
    {
        in.imbue(std::locale::classic());
        in.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
        char signature[16];
        in.read(signature, 16);
        if (std::string(signature, 16) != Markers::SIGNATURE)
        {
            throw std::runtime_error("signature did not match");
        }

        while (true)
        {
            // Markers are aligned to 4-byte boundaries
            auto pos = in.tellg();
//            if (pos & 3)
//                in.seekg(4 - (pos & 3), std::ios::cur);

            char markerData[4];
            in.read(markerData, 4);
            std::string marker(markerData, 4);
            if (marker == Markers::XPTS)
            {
                ret.m_header_data.width = readUint16(in);
                in.ignore(2); // padding
            }
            else if (marker == Markers::YPTS)
            {
                ret.m_header_data.depth = readUint16(in);
                in.ignore(2); // padding
            }
            else if (marker == Markers::SIZE)
            {
                ret.m_header_data.depth = ret.m_header_data.width = readUint16(in) + 1;
                in.ignore(2); // padding
            }
            else if (marker == Markers::SCAL)
            {
                float stepX = readFloat(in);
                float stepY = readFloat(in);
                float stepZ = readFloat(in);
                if (stepY != stepX || stepZ != stepX)
                    throw std::runtime_error("SCAL values are not all equal");
                else if (stepX <= 0.0f)
                    throw std::runtime_error("SCAL value is negative");
                ret.m_header_data.scale = stepX;
            }
            else if (marker == Markers::CRAD)
                ret.m_header_data.planet_radius = readFloat(in); // radius of planet
            else if (marker == Markers::CRVM)
            {
                ret.m_header_data.mode = readUint16(in);
                in.ignore(2); // padding
            }
            else if (marker == Markers::ALTW)
            {
                if (ret.m_header_data.width <= 0 || ret.m_header_data.depth <= 0)
                    throw std::runtime_error("ALTW found before dimensions");

                ret.m_header_data.height_scale = readInt16(in) / 65536.0f;
                ret.m_header_data.base_height = readInt16(in);
                ret.m_height_data = (float*) malloc(sizeof(float) * ret.m_header_data.width * ret.m_header_data.depth);
                float min_height(FLT_MAX);
                float max_height(FLT_MIN);

                for (int y = 0; y < ret.m_header_data.depth; y++)
                    for (int x = 0; x < ret.m_header_data.width; x++)
                    {
                        float h = readInt16(in);
                        h = ret.m_header_data.base_height + (ret.m_header_data.height_scale * h);
                        //h = h * scale;

                        if(h < min_height)
                            min_height = h;
                        if(h > max_height)
                            max_height = h;

                        ret(x,y) = h;
                    }
                ret.m_header_data.min_height = min_height;
                ret.m_header_data.max_height = max_height;
                if(ret.m_header_data.depth * ret.m_header_data.width % 2 != 0)
                    in.ignore(2); // Need to realign to 4 byte chunks as per the spec
            }
            else if (marker == Markers::EOF_)
                break;
            else
                throw std::runtime_error("unexpected chunk `" + marker + "'");
        }
    }
    catch (std::runtime_error &e)
    {
        throw std::runtime_error("Failed to read " + filename + ": " + e.what());
    }
    return ret;
}

void TerragenFileManager::scale(TerragenFile & file, int scale_factor)
{
    int old_width(file.m_header_data.width);
    int old_depth(file.m_header_data.depth);

    int width(file.m_header_data.width * scale_factor);
    int depth(file.m_header_data.depth * scale_factor);

    float base_height(file.m_header_data.base_height);

    float* height_data = (float*) malloc(sizeof(float) * width * depth);

    // In the X direction
    for(int y(0); y < old_depth; y++)
    {
        for(int x(0); x < old_width; x++)
        {
            float h( (file(x, y) - base_height) * scale_factor);
            float h2( (file( std::min( x+1, file.m_header_data.width-1 ), y) - base_height) * scale_factor);
            float height_diff( (h2 - h)); // It would be /scale_factor * scale_factor. So leave it out
            float height_increments(height_diff/scale_factor);

            for(int i(0); i < scale_factor; i++)
                height_data[(y*scale_factor*width) + (x*scale_factor) + i] = base_height + (h + (i*height_increments));
        }
    }

    // All necessary data has been copied
    free(file.m_height_data);
    file.m_height_data = height_data;
    file.m_header_data.depth = depth;
    file.m_header_data.width = width;
    file.m_header_data.max_height = base_height + ((file.m_header_data.max_height - base_height) * scale_factor);
    file.m_header_data.min_height = base_height + ((file.m_header_data.min_height - base_height) * scale_factor);

    // In the Y direction
    for(int y(0); y < old_depth; y++)
    {
        for(int x(0); x < width; x++)
        {
            float h( file(x, y * scale_factor) - base_height);
            float h2( file(x, std::min( (y+1) * scale_factor , depth-1)) - base_height);
            float height_diff( (h2 - h)); // It would be /scale_factor * scale_factor. So leave it out
            float height_increments(height_diff/scale_factor);

            for(int i(0); i < scale_factor; i++)
                file(x, (y*scale_factor) + i) = base_height + h + (i*height_increments) ;
        }
    }
}

bool TerragenFileManager::writeTerragen(TerragenFile & terragen, const std::string & filename)
{
    try
    {
        if (terragen.m_header_data.width >= 65536 || terragen.m_header_data.depth >= 65536)
            throw std::runtime_error("Terrain is too large for Terragen format");
        if (terragen.m_header_data.width == 0 || terragen.m_header_data.depth == 0)
            throw std::runtime_error("Empty region cannot be written to Terragen format");

        std::ofstream out(filename, std::ios::binary);
        if (!out)
            throw std::runtime_error("Could not open file");

        out.imbue(std::locale::classic());
        out.exceptions(std::ios::failbit | std::ios::badbit);

        out.write(Markers::SIGNATURE.c_str(), 16);

        out.write(Markers::SIZE.c_str(), 4);
        writeUint16(out, std::min(terragen.m_header_data.width, terragen.m_header_data.depth) - 1);
        writeUint16(out, 0); // padding

        out.write(Markers::XPTS.c_str(), 4);
        writeUint16(out, terragen.m_header_data.width);
        writeUint16(out, 0); // padding

        out.write(Markers::YPTS.c_str(), 4);
        writeUint16(out, terragen.m_header_data.depth);
        writeUint16(out, 0); // padding

        out.write(Markers::SCAL.c_str(), 4);
        writeFloat(out, terragen.m_header_data.scale);
        writeFloat(out, terragen.m_header_data.scale);
        writeFloat(out, terragen.m_header_data.scale);

        out.write(Markers::CRAD.c_str(), 4);
        writeFloat(out, terragen.m_header_data.planet_radius);

        out.write(Markers::CRVM.c_str(), 4);
        writeUint16(out, terragen.m_header_data.mode);
        writeUint16(out, 0); // padding

        out.write(Markers::ALTW.c_str(), 4);
        writeInt16(out, std::int16_t(terragen.m_header_data.height_scale * 65536.0f));
        writeInt16(out, std::int16_t(terragen.m_header_data.base_height));

        for (int y (0); y < terragen.m_header_data.depth; y++)
            for (int x (0); x < terragen.m_header_data.width; x++)
            {
                float h = (terragen(x,y) - terragen.m_header_data.base_height) / terragen.m_header_data.height_scale;
                writeInt16(out, std::int16_t(std::round(h)));
            }

        if(terragen.m_header_data.depth * terragen.m_header_data.width % 2 != 0)
            writeUint16(out, 0); // padding

        out.write(Markers::EOF_.c_str(), 4);
        out.close();
    }
    catch (std::runtime_error &e)
    {
        throw std::runtime_error("Failed to write " + filename + ": " + e.what());
    }
}

//int main(int argc, char *argv[])
//{
//    std::string input_filename ("/home/harry/ter-files/Alps.ter");
//    std::string output_filename ("/home/harry/ter-files/GC0_reproduced.ter");

//    TerragenFile loaded_file (input_filename);
////    std::cout << "***********ORIGINAL FILE***************" << std::endl;
////    loaded_file.summarize();
////    loaded_file.write(output_filename);


////    TerragenFile reloaded_file(output_filename);
////    std::cout << "***********REPRODUCED FILE***************" << std::endl;
////    reloaded_file.summarize();

////    std::cout << "Success!" << std::endl;
//    return 0;
//}
