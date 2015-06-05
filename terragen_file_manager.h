/**
 * @file
 *
 * Terragen format import and export.
 */

#ifndef TERRAGEN_FILE_MANAGER
#define TERRAGEN_FILE_MANAGER

#include <string>
#include "terragen_file.h"

struct Markers{
    const static std::string SIGNATURE;
    const static std::string XPTS;
    const static std::string YPTS;
    const static std::string SIZE;
    const static std::string SCAL;
    const static std::string CRAD;
    const static std::string CRVM;
    const static std::string ALTW;
    const static std::string EOF_;
};

namespace TerragenFileManager{
    /**
     * Load a Terragen file.
     */
    TerragenFile readTerragen(const std::string &filename);

    /**
     * Write Terragen file
     */
    bool writeTerragen(TerragenFile & terragen, const std::string & filename);

    void scale(TerragenFile & file, int scale);
}

/**
 * Save a Terragen file.
 */
//void writeTerragen(const uts::string &filename, const MemMap<height_tag> &map, const Region &region);

#endif /* !UTS_COMMON_TERRAGEN_H */
