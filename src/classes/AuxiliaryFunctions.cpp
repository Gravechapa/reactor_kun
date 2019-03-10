#include "AuxiliaryFunctions.hpp"
#include <fstream>


#define readbyte(a,b) do if(((a) = (b).get()) == EOF) return Dimension(); while (0)
#define readword(a,b) do {int32_t cc_= 0, dd_ = 0; \
                          if((cc_ = (b).get()) == EOF \
                          || (dd_ = (b).get()) == EOF) return Dimension(); \
                          (a) = (cc_ << 8) + (dd_); \
                          } while(0)

Dimension getJpegResolution(std::string_view path) //http://carnage-melon.tom7.org/stuff/jpegsize.html
{
    std::ifstream file(path.data(), std::ofstream::binary);
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    int32_t marker = 0;

    if (file.get() != 0xFF || file.get() != 0xD8)
    {
        return Dimension();
    }

    Dimension result;
    while (true)
    {
        readbyte(marker, file);
        if (marker != 0xFF)
        {
            return result;
        }
        do
        {
            readbyte(marker, file);
        } while (marker == 0xFF);

        switch (marker)
        {
            case 0xC0:
            case 0xC1:
            case 0xC2:
            case 0xC3:
            case 0xC5:
            case 0xC6:
            case 0xC7:
            case 0xC9:
            case 0xCA:
            case 0xCB:
            case 0xCD:
            case 0xCE:
            case 0xCF:
            {
                file.ignore(3);
                readword(result.height, file);
                readword(result.width, file);

                return result;
            }
            case 0xDA:
            case 0xD9:
                return result;
            default:
            {
                int32_t length;

                readword(length,file);
                if (length < 2)
                {
                    return result;
                }
                length -= 2;
                file.ignore(length);
                break;
            }
        }
    }
}
