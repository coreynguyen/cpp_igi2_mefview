// main.cpp

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Slider.H>



#include "viewport3d.h" // Ensure this header includes all necessary declarations
#include "stringext.h"
#include "filesystem.h"

#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdint> // Include for fixed-width integer types
#include <array>   // Include for std::array
#include <sstream> // Include for stringstream
#include <string>
#include <cstring> // For memset
#include <algorithm>  // For std::find_if
#include <cmath>
#include <unordered_map>
#include <windows.h>


using namespace std;

// Template functions for readValueing and writing data
template <typename T>
void readValue(std::ifstream &f, T &data) {
    f.read(reinterpret_cast<char*>(&data), sizeof(T));
}

template <typename T>
void writeValue(std::ofstream &f, const T &data) {
    f.write(reinterpret_cast<const char*>(&data), sizeof(T));
}

// Function to swap byte order for 16-bit integers
uint16_t swapEndian16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

// Function to swap byte order for 32-bit integers
uint32_t swapEndian32(uint32_t val) {
    return ((val & 0x000000FF) << 24) |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0xFF000000) >> 24);
}

// Function to swap byte order for 32-bit signed integers
int32_t swapEndian32Signed(int32_t val) {
    uint32_t uval = swapEndian32(static_cast<uint32_t>(val));
    return static_cast<int32_t>(uval);
}

// Function to read a null-terminated string
bool readString(std::ifstream &f, std::string &str) {
    char ch;
    str.clear();
    while (f.get(ch)) {
        if (ch == '\0') {
            return true;
        }
        str += ch;
    }
    return false; // Failed to read string
}

// Helper function to convert integer to FourCC string
std::string intToFourCC(uint32_t intVal, bool reverse = false) {
    std::string str(4, ' ');
    if (reverse) {
        for (int i = 0; i < 4; ++i) {
            str[i] = static_cast<char>((intVal >> (8 * (3 - i))) & 0xFF);
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            str[i] = static_cast<char>((intVal >> (8 * i)) & 0xFF);
        }
    }
    return str;
}





// Helper functions
std::string get_extension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        return filename.substr(pos);
    }
    return "";
}

std::string get_directory(const std::string& filename) {
    size_t pos = filename.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filename.substr(0, pos);
    }
    return "";
}

std::string get_filename_without_extension(const std::string& filename) {
    size_t pos1 = filename.find_last_of("/\\");
    size_t pos2 = filename.find_last_of('.');
    if (pos2 > pos1 && pos2 != std::string::npos) {
        return filename.substr(pos1 + 1, pos2 - pos1 - 1);
    }
    return filename.substr(pos1 + 1);
}








// Template specialization for reading big endian values
template <typename T>
bool readValueBE(std::ifstream &f, T &data);

// Specialization for uint16_t
template <>
bool readValueBE<uint16_t>(std::ifstream &f, uint16_t &data) {
    if (!f.read(reinterpret_cast<char*>(&data), sizeof(uint16_t))) {
        return false;
    }
    data = swapEndian16(data);
    return true;
}

// Specialization for uint32_t
template <>
bool readValueBE<uint32_t>(std::ifstream &f, uint32_t &data) {
    char buffer[4];
    if (!f.read(buffer, sizeof(uint32_t))) {
        return false;
    }
    std::cout << "Read bytes: "
              << std::hex << (unsigned char)buffer[0] << " "
              << (unsigned char)buffer[1] << " "
              << (unsigned char)buffer[2] << " "
              << (unsigned char)buffer[3] << std::dec << std::endl;
    data = (unsigned char)buffer[0] << 24 |
           (unsigned char)buffer[1] << 16 |
           (unsigned char)buffer[2] << 8  |
           (unsigned char)buffer[3];
    return true;
}


// Specialization for int32_t
template <>
bool readValueBE<int32_t>(std::ifstream &f, int32_t &data) {
    if (!f.read(reinterpret_cast<char*>(&data), sizeof(int32_t))) {
        return false;
    }
    data = swapEndian32Signed(data);
    return true;
}

// Function to check if a file exists
bool fileExists(const std::string& filename) {
    DWORD fileAttr = GetFileAttributesA(filename.c_str());
    return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
}

// Function to generate a unique filename if the file already exists
std::string generateUniqueFilename(const std::string& baseFilename) {
    std::string uniqueFilename = baseFilename;
    int counter = 2;
    size_t dotPosition = baseFilename.find_last_of('.');
    std::string namePart = (dotPosition != std::string::npos) ? baseFilename.substr(0, dotPosition) : baseFilename;
    std::string extensionPart = (dotPosition != std::string::npos) ? baseFilename.substr(dotPosition) : "";

    while (fileExists(uniqueFilename)) {
        uniqueFilename = namePart + "(" + std::to_string(counter) + ")" + extensionPart;
        counter++;
    }

    return uniqueFilename;
}

    // Enum for chunk types (expand as needed)
    enum ChunkType : uint32_t {
        FORM = 0x464F524D, // 'FORM'
        BANM = 0x42414E4D, // 'BANM'
        SNDS = 0x534E4453, // 'SNDS'
        SVOL = 0x53564F4C, // 'SVOL'
        MODS = 0x4D4F4453, // 'MODS'
        VNAM = 0x564E414D, // 'VNAM'
        INST = 0x494E5354, // 'INST'
        TEXF = 0x54455846, // 'TEXF'
        PALF = 0x50414C46, // 'PALF'
        GTT_ = 0x47545420  // 'GTT '
        // Add other types as needed
    };


enum class MeshResourceType : uint32_t {
    MESH = 0x4D455348,        // 'MESH'
    ATTA = 0x41545441,        // 'ATTA'
    MVTX = 0x4D565458,        // 'MVTX'
    RD3D = 0x52443344,        // 'RD3D'
    HIER = 0x48494552,        // 'HIER'
    BNAM = 0x424E414D,        // 'BNAM'
    FACE = 0x46414345,        // 'FACE'
    REND = 0x52454E44,        // 'REND'
    VRTX = 0x56525458,        // 'VRTX'
    CVTX = 0x43565458,        // 'CVTX'
    CFCE = 0x43464345,        // 'CFCE'
    CMAT = 0x434D4154,        // 'CMAT'
    CSPH = 0x43535048,        // 'CSPH'
    MRPH = 0x4D525048,        // 'MRPH'
    CMSH = 0x434D5348,        // 'CMSH'
    IFLF = 0x46464C49,        // 'IFLF'
    MECO = 0x4D45434F,         // 'MECO'
    FORM = 0x464F524D, // 'FORM'
    BANM = 0x42414E4D, // 'BANM'
    SNDS = 0x534E4453, // 'SNDS'
    SVOL = 0x53564F4C, // 'SVOL'
    MODS = 0x4D4F4453, // 'MODS'
    VNAM = 0x564E414D, // 'VNAM'
    INST = 0x494E5354, // 'INST'
    TEXF = 0x54455846, // 'TEXF'
    PALF = 0x50414C46, // 'PALF'
    GTT_ = 0x47545420,  // 'GTT '
    MTP_ = 0x4D545020,
    SMES = 0x534D4553,
    SVTX = 0x53565458,
    SFAC = 0x53464143,
    EDGE = 0x45444745,
    LTMP = 0x4C544D50,
    GLOW = 0x474C4F57,
    NAME = 0x454D414E,
    PATH = 0x48544150,
    ILFF = 0x46464C49,
    IRES = 0x53455249,
    BODY = 0x59444F42,
    NewO = 0x4F77654E




};

struct mefMeshResource {
    virtual ~mefMeshResource() {}

    // Virtual function for reading
    virtual void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) = 0;

    // Virtual function for writing
    virtual void writeData(std::ofstream &f) const = 0;

    // Virtual function for converting to string
    virtual std::string to_string() const = 0;
};

struct mefDateStamp_t {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint32_t millisecond;

    mefDateStamp_t() {
        year = 1970;
        month = 1;
        day = 1;
        hour = 0;
        minute = 0;
        second = 0;
        millisecond = 0;
    }

    void print_date_as_string() const {
        std::cout << year << "-"
                  << std::setw(2) << std::setfill('0') << month << "-"
                  << std::setw(2) << std::setfill('0') << day << " "
                  << std::setw(2) << std::setfill('0') << hour << ":"
                  << std::setw(2) << std::setfill('0') << minute << ":"
                  << std::setw(2) << std::setfill('0') << second << "."
                  << std::setw(3) << std::setfill('0') << millisecond
                  << std::endl;
    }

    void read(std::ifstream &f) {
        readValue(f, year);       // Read year
        readValue(f, month);      // Read month
        readValue(f, day);        // Read day
        readValue(f, hour);       // Read hour
        readValue(f, minute);     // Read minute
        readValue(f, second);     // Read second
        readValue(f, millisecond); // Read millisecond

        print_date_as_string();
    }

    void write(std::ofstream &f) const {
        writeValue(f, year);       // Write year
        writeValue(f, month);      // Write month
        writeValue(f, day);        // Write day
        writeValue(f, hour);       // Write hour
        writeValue(f, minute);     // Write minute
        writeValue(f, second);     // Write second
        writeValue(f, millisecond); // Write millisecond
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << year << "-"
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day << " "
            << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second << "."
            << std::setw(3) << std::setfill('0') << millisecond;
        return oss.str();
    }
};

struct mefMeshSphere_t {
    float origin[3];  // Array to hold x, y, z coordinates
    float radius;

    mefMeshSphere_t() {
        origin[0] = 0.0f;
        origin[1] = 0.0f;
        origin[2] = 0.0f;
        radius = 0.0f;
    }

    void read(std::ifstream &f) {
        readValue(f, origin[0]); // Read x
        readValue(f, origin[1]); // Read y
        readValue(f, origin[2]); // Read z
        readValue(f, radius);     // Read radius
    }

    void write(std::ofstream &f) const {
        writeValue(f, origin[0]); // Write x
        writeValue(f, origin[1]); // Write y
        writeValue(f, origin[2]); // Write z
        writeValue(f, radius);     // Write radius
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "Origin: ["
            << origin[0] << ", "
            << origin[1] << ", "
            << origin[2] << "], Radius: "
            << radius;
        return oss.str();
    }
};


struct mefMeshHier_t : public mefMeshResource {
    std::vector<uint8_t> num_children;   // Store number of children as bytes
    std::vector<std::array<float, 3>> position; // Store positions as arrays of floats

    mefMeshHier_t() {}

    // Override readData method
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        if (count > 0) {
            num_children.resize(count);
            position.resize(count);

            for (size_t i = 0; i < count; ++i) {
                readValue(f, num_children[i]); // Read each child count as byte
            }

            // Skip padding
            size_t padding = calculatePadding(count);
            f.seekg(padding, std::ios::cur);

            for (size_t i = 0; i < count; ++i) {
                readValue(f, position[i]); // Read 3 floats for position
            }
        } else {
            std::cerr << "mefMeshHier_t: Count is Zero" << std::endl;
        }
    }

    // Override writeData method
    void writeData(std::ofstream &f) const override {
        size_t count = num_children.size();
        if (count > 0) {
            for (const auto &child : num_children) {
                writeValue(f, child); // Write each child count as byte
            }

            // Write padding
            size_t padding = calculatePadding(count);
            for (size_t i = 0; i < padding; ++i) {
                uint8_t zero = 0;
                f.write(reinterpret_cast<const char*>(&zero), sizeof(uint8_t)); // Write padding bytes
            }

            for (const auto &pos : position) {
                writeValue(f, pos); // Write 3 floats for position
            }
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Children Count: " << num_children.size() << ", Positions: [";
        for (const auto &pos : position) {
            oss << "[" << pos[0] << ", " << pos[1] << ", " << pos[2] << "], ";
        }
        if (!position.empty()) {
            oss.seekp(-2, oss.cur); // Remove last comma and space
        }
        oss << "]";
        return oss.str();
    }

private:
    size_t calculatePadding(size_t count) const {
        return (4 - (count % 4)) % 4; // Calculate padding needed
    }
};


struct mefMeshBNam_t : public mefMeshResource {
    std::vector<std::string> names; // Vector to hold bone names

    mefMeshBNam_t() {}

    // Override readData method
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        names.clear(); // Clear existing names

        if (count > 0) {
            names.reserve(count); // Reserve space for performance
            for (uint32_t i = 0; i < count; ++i) {
                char buffer[17] = {0}; // Buffer to read up to 16 characters + null terminator
                f.read(buffer, 16); // Read 16 bytes
                std::string name(buffer); // Create string from buffer

                // Trim any null characters from the right
                name.erase(std::find(name.begin(), name.end(), '\0'), name.end());

                names.push_back(name); // Add the name to the list
            }
        } else {
            std::cerr << "mefMeshBNam_t: Count is Zero" << std::endl;
        }
    }

    // Override writeData method
    void writeData(std::ofstream &f) const override {
        for (const std::string &name : names) {
            std::string padded_name = name;
            padded_name.resize(16, '\0'); // Pad name to 16 characters
            f.write(padded_name.c_str(), 16); // Write 16 bytes directly
        }
    }

    // Override to_string method
    std::string to_string() const override {
        std::ostringstream oss;
        for (const auto &name : names) {
            oss << "Bone Name: " << name << "\n";
        }
        return oss.str();
    }
};


struct mefMesh_t : public mefMeshResource {
    float unk01;
    mefDateStamp_t date;
    uint32_t model_type;
    uint32_t unk10;
    uint32_t unk11;
    uint32_t unk12;
    std::array<mefMeshSphere_t, 3> unk16;
    uint32_t num_r_faces;
    uint32_t num_r_verts;
    uint32_t num_r_buffer;
    uint32_t sum_c_faces;
    uint32_t sum_c_verts;
    uint32_t sum_c_buffer;
    float model_radius;
    uint16_t num_mverts;
    uint16_t num_attach;
    uint16_t num_pverts;
    uint16_t num_pfaces;
    uint16_t num_portals;
    uint16_t num_bones;
    uint16_t num_glows;
    std::array<uint16_t, 19> reserve;

    mefMesh_t()
        : unk01(0.0f), model_type(0), unk10(0), unk11(0), unk12(0),
          num_r_faces(0), num_r_verts(0), num_r_buffer(0), sum_c_faces(0),
          sum_c_verts(0), sum_c_buffer(0), model_radius(0.0f),
          num_mverts(0), num_attach(0), num_pverts(0), num_pfaces(0),
          num_portals(0), num_bones(0), num_glows(0) {
        reserve.fill(0);
    }

    // Existing read function
    void read(std::ifstream &f) {
        readValue(f, unk01);
        date.read(f);
        readValue(f, model_type);
        readValue(f, unk10);
        readValue(f, unk11);
        readValue(f, unk12);
        for (auto &sphere : unk16) {
            sphere.read(f);
        }
        readValue(f, num_r_faces);
        readValue(f, num_r_verts);
        readValue(f, num_r_buffer);
        readValue(f, sum_c_faces);
        readValue(f, sum_c_verts);
        readValue(f, sum_c_buffer);
        readValue(f, model_radius);
        readValue(f, num_mverts);
        readValue(f, num_attach);
        readValue(f, num_pverts);
        readValue(f, num_pfaces);
        readValue(f, num_portals);
        readValue(f, num_bones);
        readValue(f, num_glows);
        for (auto &res : reserve) {
            readValue(f, res);
        }
    }

    // Existing write function
    void write(std::ofstream &f) const {
        writeValue(f, unk01);
        date.write(f);
        writeValue(f, model_type);
        writeValue(f, unk10);
        writeValue(f, unk11);
        writeValue(f, unk12);
        for (const auto &sphere : unk16) {
            sphere.write(f);
        }
        writeValue(f, num_r_faces);
        writeValue(f, num_r_verts);
        writeValue(f, num_r_buffer);
        writeValue(f, sum_c_faces);
        writeValue(f, sum_c_verts);
        writeValue(f, sum_c_buffer);
        writeValue(f, model_radius);
        writeValue(f, num_mverts);
        writeValue(f, num_attach);
        writeValue(f, num_pverts);
        writeValue(f, num_pfaces);
        writeValue(f, num_portals);
        writeValue(f, num_bones);
        writeValue(f, num_glows);
        for (const auto &res : reserve) {
            writeValue(f, res);
        }
    }

    // Overridden readData function
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        read(f);
    }

    // Overridden writeData function
    void writeData(std::ofstream &f) const override {
        write(f);
    }

    // Overridden to_string function
    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Unknown Field 01: " << unk01 << "\n"
            << "Date of Model: " << date.to_string() << "\n"
            << "Model Type: " << model_type << "\n"
            << "Unknown Fields 10-12: " << unk10 << ", " << unk11 << ", " << unk12 << "\n"
            << "Mesh Spheres: [";
        for (const auto &sphere : unk16) {
            oss << sphere.to_string() << ", ";
        }
        if (!unk16.empty()) {
            oss.seekp(-2, oss.cur); // Remove last comma and space
        }
        oss << "]\n"
            << "Number of R Faces: " << num_r_faces << "\n"
            << "Number of R Verts: " << num_r_verts << "\n"
            << "Number of R Buffer: " << num_r_buffer << "\n"
            << "Sum of C Faces: " << sum_c_faces << "\n"
            << "Sum of C Verts: " << sum_c_verts << "\n"
            << "Sum of C Buffer: " << sum_c_buffer << "\n"
            << "Model Radius: " << model_radius << "\n"
            << "Number of M Verts: " << num_mverts << "\n"
            << "Number of Attachments: " << num_attach << "\n"
            << "Number of P Verts: " << num_pverts << "\n"
            << "Number of P Faces: " << num_pfaces << "\n"
            << "Number of Portals: " << num_portals << "\n"
            << "Number of Bones: " << num_bones << "\n"
            << "Number of Glows: " << num_glows << "\n"
            << "Reserve Array: [";
        for (const auto &res : reserve) {
            oss << res << ", ";
        }
        if (!reserve.empty()) {
            oss.seekp(-2, oss.cur); // Remove last comma and space
        }
        oss << "]\n";
        return oss.str();
    }
};


struct mefMeshAttaEntry_t {
    std::string name;          // Name (up to 16 bytes)
    float unk42[3][4];        // 3x4 array
    uint32_t unk43;           // Unknown 32-bit integer
    int32_t bone_index;       // Bone index (signed)

    mefMeshAttaEntry_t() {
        name.clear();
        std::memset(unk42, 0, sizeof(unk42)); // Initialize unk42 to 0
        unk43 = 0;
        bone_index = 0;
    }

    void read(std::ifstream &f) {
        name.clear();
        char b;

        // Read the name (up to 16 bytes)
        for (int j = 0; j < 16; ++j) {
            readValue(f, b);  // Read one byte
            if (b > 0) {
                name += b;
            } else {
                f.seekg(16 - j - 1, std::ios::cur); // Skip remaining bytes
                break;
            }
        }

        // Read 3x4 array
        for (auto &row : unk42) {
            for (auto &value : row) {
                readValue(f, value);
            }
        }

        readValue(f, unk43);           // Read unk43
        readValue(f, bone_index);      // Read bone_index
    }

    void write(std::ofstream &f) const {
        std::string name_bytes = name.substr(0, 16); // Limit to 16 bytes
        name_bytes.resize(16, '\0'); // Pad with null bytes

        // Write each byte of the name
        for (char byte : name_bytes) {
            writeValue(f, byte);
        }

        // Write 3x4 array
        for (const auto &row : unk42) {
            for (const auto &value : row) {
                writeValue(f, value);
            }
        }

        writeValue(f, unk43);           // Write unk43
        writeValue(f, bone_index);      // Write bone_index
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "Name: " << name << ", Unk42: [";
        for (size_t i = 0; i < 3; ++i) {
            oss << "[";
            for (size_t j = 0; j < 4; ++j) {
                oss << unk42[i][j];
                if (j < 3) oss << ", ";
            }
            oss << "]";
            if (i < 2) oss << ", ";
        }
        oss << "], Unk43: " << unk43 << ", Bone Index: " << bone_index;
        return oss.str();
    }
};

struct mefMeshAtta_t : public mefMeshResource {
    std::vector<mefMeshAttaEntry_t> entries; // Vector to hold entries

    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entries.clear();  // Clear previous entries
        if (count > 0) {
            entries.resize(count); // Resize vector to hold 'count' entries
            for (auto &entry : entries) {
                entry.read(f); // Read each entry
            }
        }
    }

    void writeData(std::ofstream &f) const override {
        for (const auto &entry_item : entries) {
            entry_item.write(f); // Write each entry
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (const auto &entry_item : entries) {
            oss << entry_item.to_string() << "\n"; // Concatenate entry strings
        }
        return oss.str();
    }
};


struct mefMeshFace_t : public mefMeshResource {
    std::vector<std::array<uint16_t, 3>> entry; // Using array for fixed size faces

    mefMeshFace_t() {
        entry.clear();
    }

    // Corrected readData function to match the base class
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize to count
            for (size_t i = 0; i < count; ++i) {
                readValue(f, entry[i][0]); // Read first index
                readValue(f, entry[i][1]); // Read second index
                readValue(f, entry[i][2]); // Read third index
            }
        }
    }

    // Corrected writeData function to match the base class
    void writeData(std::ofstream &f) const override {
        for (const auto& face : entry) {
            writeValue(f, face[0]); // Write first index
            writeValue(f, face[1]); // Write second index
            writeValue(f, face[2]); // Write third index
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (const auto& face : entry) {
            oss << "Face Indices: [" << face[0] << ", " << face[1] << ", " << face[2] << "]\n";
        }
        return oss.str();
    }
};

struct mefMeshMVtx_t : public mefMeshResource {
    std::vector<std::array<float, 4>> entry; // Using array for fixed size vertices

    mefMeshMVtx_t() {
        entry.clear();
    }

    // Override readData to match the base class signature
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize to count
            for (size_t i = 0; i < count; ++i) {
                readValue(f, entry[i][0]); // Read x
                readValue(f, entry[i][1]); // Read y
                readValue(f, entry[i][2]); // Read z
                readValue(f, entry[i][3]); // Read w
            }
        }
    }

    // Override writeData to match the base class signature
    void writeData(std::ofstream &f) const override {
        uint32_t count = static_cast<uint32_t>(entry.size());
        writeValue(f, count); // Write the count of vertices
        for (const auto& vertex : entry) {
            for (const auto& value : vertex) {
                writeValue(f, value); // Write each vertex component
            }
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshMVtx_t(entry=["; // Format output for string representation
        for (const auto& vertex : entry) {
            oss << "[";
            for (size_t i = 0; i < vertex.size(); ++i) {
                oss << vertex[i];
                if (i < vertex.size() - 1) {
                    oss << ", ";
                }
            }
            oss << "], ";
        }
        if (!entry.empty()) {
            oss.seekp(-2, oss.cur); // Remove last comma and space
        }
        oss << "])";
        return oss.str();
    }
};


struct mefMeshRD3D_t : public mefMeshResource {
    uint32_t flag;
    uint32_t num_faces;
    uint32_t num_meshes;
    uint32_t num_vertices;
    uint32_t reserved_type0[5];
    uint32_t verts_0;
    uint32_t verts_1;
    uint32_t num_vertices_extra;
    uint32_t reserved_type1[4];
    uint32_t num_lightmaps;
    uint32_t reserved_type3[6];
    std::vector<uint32_t> reserved_extra;

    mefMeshRD3D_t() {
        flag = 0;
        num_faces = 0;
        num_meshes = 0;
        num_vertices = 0;
        std::fill(std::begin(reserved_type0), std::end(reserved_type0), 0);
        verts_0 = 0;
        verts_1 = 0;
        num_vertices_extra = 0;
        std::fill(std::begin(reserved_type1), std::end(reserved_type1), 0);
        num_lightmaps = 0;
        std::fill(std::begin(reserved_type3), std::end(reserved_type3), 0);
    }

    // Override readData method to match mefMeshResource signature
    void readData(std::ifstream &f, uint32_t size, int model_type = 0) override {
        if (size < 16) {
            std::cerr << "Error: Specified size " << size << " is smaller than the base structure size (16 bytes)." << std::endl;
            return;
        }

        readValue(f, flag);
        readValue(f, num_faces);
        readValue(f, num_meshes);
        readValue(f, num_vertices);

        // Check for specific sizes to determine which reserved fields to read
        if (size == 36) {
            for (auto &val : reserved_type0) {
                readValue(f, val);
            }
        } else if (size == 40) {
            readValue(f, verts_0);
            readValue(f, verts_1);
            for (auto &val : reserved_type1) {
                readValue(f, val);
            }
        } else if (size == 44) {
            readValue(f, num_lightmaps);
            for (auto &val : reserved_type3) {
                readValue(f, val);
            }
        } else {
            // Handle unknown sizes
            reserved_extra.resize((size - 16) / sizeof(uint32_t));
            for (auto &val : reserved_extra) {
                readValue(f, val);
            }
            std::cout << "Warning: Unknown size " << size << ". Extra reserved data captured." << std::endl;
        }
    }

    // Override writeData method to match mefMeshResource signature
    void writeData(std::ofstream &f) const override {
        writeValue(f, flag);
        writeValue(f, num_faces);
        writeValue(f, num_meshes);
        writeValue(f, num_vertices);

        // Write reserved fields based on size
        for (const auto &val : reserved_type0) {
            writeValue(f, val);
        }

        writeValue(f, verts_0);
        writeValue(f, verts_1);
        for (const auto &val : reserved_type1) {
            writeValue(f, val);
        }

        writeValue(f, num_lightmaps);
        for (const auto &val : reserved_type3) {
            writeValue(f, val);
        }

        for (const auto &val : reserved_extra) {
            writeValue(f, val);
        }
    }

    // Override to_string method to match mefMeshResource signature
    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Flag: " << flag << "\n"
            << "Number of Faces: " << num_faces << "\n"
            << "Number of Meshes: " << num_meshes << "\n"
            << "Number of Vertices: " << num_vertices;

        // Include reserved fields in string representation
        if (!std::equal(std::begin(reserved_type0), std::end(reserved_type0), std::begin(reserved_type0))) {
            oss << "\nReserved Type 0: ";
            for (const auto &val : reserved_type0) {
                oss << val << " ";
            }
        }
        if (!std::equal(std::begin(reserved_type1), std::end(reserved_type1), std::begin(reserved_type1))) {
            oss << "\nReserved Type 1: ";
            for (const auto &val : reserved_type1) {
                oss << val << " ";
            }
        }
        if (!std::equal(std::begin(reserved_type3), std::end(reserved_type3), std::begin(reserved_type3))) {
            oss << "\nNumber of Lightmaps: " << num_lightmaps << "\nReserved Type 3: ";
            for (const auto &val : reserved_type3) {
                oss << val << " ";
            }
        }
        if (!reserved_extra.empty()) {
            oss << "\nReserved Extra Data: ";
            for (const auto &val : reserved_extra) {
                oss << val << " ";
            }
        }
        return oss.str();
    }
};

struct mefMeshGlow_t : public mefMeshResource {
    float unk93[7];
    uint32_t unk94;

    mefMeshGlow_t() {
        unk93[0] = 0;
        unk93[1] = 0;
        unk93[2] = 0;
        unk93[3] = 0;
        unk93[4] = 0;
        unk93[5] = 0;
        unk93[6] = 0;
        unk94 = 0;
    }

    // Override readData method to match mefMeshResource signature
    void readData(std::ifstream &f, uint32_t size, int model_type = 0) override {
        readValue(f, unk93[0]);
        readValue(f, unk93[1]);
        readValue(f, unk93[2]);
        readValue(f, unk93[3]);
        readValue(f, unk93[4]);
        readValue(f, unk93[5]);
        readValue(f, unk93[6]);
        readValue(f, unk94);
    }

    // Override writeData method to match mefMeshResource signature
    void writeData(std::ofstream &f) const override {
        writeValue(f, unk93[0]);
        writeValue(f, unk93[1]);
        writeValue(f, unk93[2]);
        writeValue(f, unk93[3]);
        writeValue(f, unk93[4]);
        writeValue(f, unk93[5]);
        writeValue(f, unk93[6]);
        writeValue(f, unk94);
    }

    // Override to_string method to match mefMeshResource signature
    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshGlow_t(unk93=["
            << unk93[0] << ", "
            << unk93[1] << ", "
            << unk93[2] << ", "
            << unk93[3] << ", "
            << unk93[4] << ", "
            << unk93[5] << ", "
            << unk93[6] << "], unk94="
            << unk94 << "])";
        return oss.str();
    }
};

struct mefMeshRendEntry_t {
    uint8_t opacity;
    uint8_t material_shininess;
    uint8_t diffuse_color;
    uint8_t opacity_detail;
    float position[3]; // Array to hold x, y, z coordinates
    uint16_t face_pos;
    uint16_t face_count;
    uint16_t vertex_pos;
    uint16_t vertex_count;
    int16_t texture_diffuse_index;
    int16_t texture_bump_index;
    int16_t texture_reflection_index;
    uint8_t texture_reflection_detail;
    uint8_t texture_bump_detail;
    int16_t lightmap_index;
    std::vector<uint32_t> reserved_extra; // Dynamic size for reserved extra data

    mefMeshRendEntry_t() {
        opacity = 0;
        material_shininess = 0;
        diffuse_color = 0;
        opacity_detail = 0;
        position[0] = position[1] = position[2] = 0.0f;
        face_pos = 0;
        face_count = 0;
        vertex_pos = 0;
        vertex_count = 0;
        texture_diffuse_index = -1;
        texture_bump_index = -1;
        texture_reflection_index = -1;
        texture_reflection_detail = 0;
        texture_bump_detail = 0;
        lightmap_index = -1;
    }

    // Method to read data
    void read(std::ifstream &f, uint32_t size) {
        std::cout << "[mefMeshRendEntry_t::read] Starting to read entry with size " << size << " bytes." << std::endl;

        // Read the common fields
        readValue(f, opacity);
        readValue(f, material_shininess);
        readValue(f, diffuse_color);
        readValue(f, opacity_detail);
        for (auto &coord : position) {
            readValue(f, coord); // Read x, y, z
        }
        readValue(f, face_pos);
        readValue(f, face_count);
        readValue(f, vertex_pos);
        readValue(f, vertex_count);
        readValue(f, texture_diffuse_index);

        // Determine the appropriate fields to read based on the structure size
        if (size == 28) {
            readValue(f, lightmap_index);
            std::cout << "[mefMeshRendEntry_t::read] Read lightmap_index: " << lightmap_index << std::endl;
        }
        else if (size == 32) {
            readValue(f, texture_bump_index);
            readValue(f, texture_reflection_index);
            readValue(f, texture_reflection_detail);
            readValue(f, texture_bump_detail);
            std::cout << "[mefMeshRendEntry_t::read] Read texture_bump_index: " << texture_bump_index << std::endl;
            std::cout << "[mefMeshRendEntry_t::read] Read texture_reflection_index: " << texture_reflection_index << std::endl;
            std::cout << "[mefMeshRendEntry_t::read] Read texture_reflection_detail: " << static_cast<int>(texture_reflection_detail) << std::endl;
            std::cout << "[mefMeshRendEntry_t::read] Read texture_bump_detail: " << static_cast<int>(texture_bump_detail) << std::endl;
        }
        else if (size > 32) {
            // Read additional fields when size > 32
            readValue(f, texture_bump_index);
            readValue(f, texture_reflection_index);
            readValue(f, texture_reflection_detail);
            readValue(f, texture_bump_detail);

            std::cout << "[mefMeshRendEntry_t::read] Read texture_bump_index: " << texture_bump_index << std::endl;
            std::cout << "[mefMeshRendEntry_t::read] Read texture_reflection_index: " << texture_reflection_index << std::endl;
            std::cout << "[mefMeshRendEntry_t::read] Read texture_reflection_detail: " << static_cast<int>(texture_reflection_detail) << std::endl;
            std::cout << "[mefMeshRendEntry_t::read] Read texture_bump_detail: " << static_cast<int>(texture_bump_detail) << std::endl;

            // Calculate remaining bytes for reserved extra data
            uint32_t extra_size = size - 32;
            uint32_t num_extra = extra_size / 4;
            if (extra_size % 4 != 0) {
                std::cerr << "[mefMeshRendEntry_t::read] Warning: Extra data size (" << extra_size << " bytes) is not a multiple of 4 bytes." << std::endl;
            }

            reserved_extra.resize(num_extra);
            for (auto &extra : reserved_extra) {
                readValue(f, extra);
                std::cout << "[mefMeshRendEntry_t::read] Read reserved_extra: " << extra << std::endl;
            }
            std::cout << "[mefMeshRendEntry_t::read] Warning: Unknown size " << size << " bytes. Extra reserved data captured." << std::endl;
        }
        else if (size > 28) {
            // Handle sizes between 28 and 32 (if any)
            readValue(f, lightmap_index);
            std::cout << "[mefMeshRendEntry_t::read] Read lightmap_index: " << lightmap_index << std::endl;

            uint32_t extra_size = size - 28;
            uint32_t num_extra = extra_size / 4;
            if (extra_size % 4 != 0) {
                std::cerr << "[mefMeshRendEntry_t::read] Warning: Extra data size (" << extra_size << " bytes) is not a multiple of 4 bytes." << std::endl;
            }

            reserved_extra.resize(num_extra);
            for (auto &extra : reserved_extra) {
                readValue(f, extra);
                std::cout << "[mefMeshRendEntry_t::read] Read reserved_extra: " << extra << std::endl;
            }
            std::cout << "[mefMeshRendEntry_t::read] Warning: Unknown size " << size << " bytes. Extra reserved data captured." << std::endl;
        }
        else {
            std::cerr << "[mefMeshRendEntry_t::read] Error: Specified size " << size << " bytes is too small or unknown." << std::endl;
        }

        // Debug output for the read entry
        std::cout << "[mefMeshRendEntry_t::read] Finished reading entry:\n" << to_string() << std::endl;
    }

    // Method to write data
    void write(std::ofstream &f, uint32_t size) const {
        writeValue(f, opacity);
        writeValue(f, material_shininess);
        writeValue(f, diffuse_color);
        writeValue(f, opacity_detail);
        for (const auto &coord : position) {
            writeValue(f, coord); // Write x, y, z
        }
        writeValue(f, face_pos);
        writeValue(f, face_count);
        writeValue(f, vertex_pos);
        writeValue(f, vertex_count);
        writeValue(f, texture_diffuse_index);

        if (size == 32) {
            writeValue(f, texture_bump_index);
            writeValue(f, texture_reflection_index);
            writeValue(f, texture_reflection_detail);
            writeValue(f, texture_bump_detail);
        }
        else if (size == 28) {
            writeValue(f, lightmap_index);
        }

        for (const auto &data : reserved_extra) {
            writeValue(f, data);
        }
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "Opacity: " << static_cast<int>(opacity) << "\n"
            << "Material Shininess: " << static_cast<int>(material_shininess) << "\n"
            << "Diffuse Color: " << static_cast<int>(diffuse_color) << "\n"
            << "Opacity Detail: " << static_cast<int>(opacity_detail) << "\n"
            << "Position: [" << std::fixed << std::setprecision(2)
            << position[0] << ", " << position[1] << ", " << position[2] << "]\n"
            << "Face Position: " << face_pos << "\n"
            << "Face Count: " << face_count << "\n"
            << "Vertex Position: " << vertex_pos << "\n"
            << "Vertex Count: " << vertex_count << "\n"
            << "Texture Diffuse Index: " << texture_diffuse_index << "\n"
            << "Texture Bump Index: " << texture_bump_index << "\n"
            << "Texture Reflection Index: " << texture_reflection_index << "\n"
            << "Texture Reflection Detail: " << static_cast<int>(texture_reflection_detail) << "\n"
            << "Texture Bump Detail: " << static_cast<int>(texture_bump_detail) << "\n"
            << "Lightmap Index: " << lightmap_index << "\n";

        if (!reserved_extra.empty()) {
            oss << "Reserved Extra Data: ";
            for (const auto& extra : reserved_extra) {
                oss << extra << " ";
            }
            oss << "\n";
        } else {
            oss << "No Reserved Extra Data.\n";
        }
        return oss.str();
    }
};

struct mefMeshRend_t : public mefMeshResource {
    std::vector<mefMeshRendEntry_t> entry;

    mefMeshRend_t() = default;

    // Override the readData method
    void readData(std::ifstream &f, uint32_t data_size, int model_type = 0) override {
        entry.clear();

        // Determine stride based on model_type
        uint32_t stride = 32; // Default stride
        if (model_type == 3) {
            stride = 28;
        }

        // Debug information
        std::cout << "[mefMeshRend_t::readData] Starting to read Rend data." << std::endl;
        std::cout << "data_size: " << data_size << " bytes" << std::endl;
        std::cout << "model_type: " << model_type << std::endl;
        std::cout << "stride: " << stride << " bytes" << std::endl;

        // Check if data_size is sufficient
        if (data_size < stride) {
            std::cerr << "[mefMeshRend_t::readData] Error: Data size (" << data_size << " bytes) is smaller than stride (" << stride << " bytes)." << std::endl;
            return;
        }

        // Calculate number of entries
        uint32_t count = data_size / stride;
        uint32_t remaining_bytes = data_size % stride;

        std::cout << "count: " << count << std::endl;
        std::cout << "remaining_bytes: " << remaining_bytes << std::endl;

        if (remaining_bytes != 0) {
            std::cerr << "[mefMeshRend_t::readData] Warning: Data size (" << data_size << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
        }

        if (count == 0) {
            std::cerr << "[mefMeshRend_t::readData] Warning: No entries to read." << std::endl;
            return;
        }

        std::cout << "[mefMeshRend_t::readData] Reading " << count << " Rend entries with stride " << stride << " bytes." << std::endl;

        // Resize the entry vector and read each entry
        entry.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            std::cout << "[mefMeshRend_t::readData] Reading Rend Entry " << i << std::endl;
            entry[i].read(f, stride);
        }

        std::cout << "[mefMeshRend_t::readData] Successfully read Rend entries." << std::endl;
    }

    // Override the writeData method
    void writeData(std::ofstream &f) const override {
        // Assuming a fixed stride of 32 for writing
        uint32_t stride = 32;
        std::cout << "[mefMeshRend_t::writeData] Writing Rend entries with stride " << stride << " bytes." << std::endl;
        for (size_t i = 0; i < entry.size(); ++i) {
            std::cout << "[mefMeshRend_t::writeData] Writing Rend Entry " << i << std::endl;
            entry[i].write(f, stride); // Use fixed stride
        }
        std::cout << "[mefMeshRend_t::writeData] Finished writing Rend entries." << std::endl;
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << "Rend Entry " << i << ":\n" << entry[i].to_string() << "\n";
        }
        return oss.str();
    }
};

struct mefMeshEdge_t : public mefMeshResource {
    std::vector<std::array<uint32_t, 2>> entry; // Using array for fixed size faces

    mefMeshEdge_t() {
        entry.clear();
    }

    // Corrected readData function to match the base class
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize to count
            for (size_t i = 0; i < count; ++i) {
                readValue(f, entry[i][0]); // Read first index
                readValue(f, entry[i][1]); // Read second index
            }
        }
    }

    // Corrected writeData function to match the base class
    void writeData(std::ofstream &f) const override {
        for (const auto& face : entry) {
            writeValue(f, face[0]); // Write first index
            writeValue(f, face[1]); // Write second index
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (const auto& edge : entry) {
            oss << "Edge Indices: [" << edge[0] << ", " << edge[1] << "]\n";
        }
        return oss.str();
    }
};

struct mefMeshLtMp_t : public mefMeshResource {
    std::vector<std::array<uint16_t, 4>> entry; // Using array for fixed size faces

    mefMeshLtMp_t() {
        entry.clear();
    }

    // Corrected readData function to match the base class
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize to count
            for (size_t i = 0; i < count; ++i) {
                readValue(f, entry[i][0]); // Read first index
                readValue(f, entry[i][1]); // Read second index
                readValue(f, entry[i][2]); // Read third index
                readValue(f, entry[i][3]); // Read fourth index
            }
        }
    }

    // Corrected writeData function to match the base class
    void writeData(std::ofstream &f) const override {
        for (const auto& item : entry) {
            writeValue(f, item[0]); // Write first index
            writeValue(f, item[1]); // Write second index
            writeValue(f, item[2]); // Write third index
            writeValue(f, item[3]); // Write fourth index
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (const auto& item : entry) {
            oss << "item: [" << item[0] << ", " << item[1] << ", " << item[2] << ", " << item[3] << "]\n";
        }
        return oss.str();
    }
};


struct mefMeshVrtxEntry_t {
    std::array<float, 3> position;    // x, y, z coordinates
    std::array<float, 2> texcoord0;  // First texture coordinates
    std::array<float, 3> normal;      // Normal vector
    float weight;                     // Weight for vertex
    std::array<float, 2> texcoord1;  // Second texture coordinates
    uint16_t vertex_index;            // Vertex index
    uint16_t bone_index;              // Bone index

    mefMeshVrtxEntry_t()
        : position{0.0f, 0.0f, 0.0f}, texcoord0{0.0f, 0.0f},
          normal{0.0f, 0.0f, 0.0f}, weight(0.0f),
          texcoord1{0.0f, 0.0f}, vertex_index(0), bone_index(0) {}

    void readData(std::ifstream &f, int model_type) {
        readValue(f, position[0]);
        readValue(f, position[1]);
        readValue(f, position[2]);

        if (model_type < 3) {
            readValue(f, normal[0]);
            readValue(f, normal[1]);
            readValue(f, normal[2]);
        }

        readValue(f, texcoord0[0]);
        readValue(f, texcoord0[1]);

        if (model_type > 1) {
            readValue(f, texcoord1[0]);
            readValue(f, texcoord1[1]);
        }

        if (model_type == 1) {
            readValue(f, weight);
            readValue(f, vertex_index);
            readValue(f, bone_index);
        }
    }

    // Write method now renamed to writeData
    void writeData(std::ofstream &f, int model_type) const {
        // Write position
        for (const auto& coord : position) {
            writeValue(f, coord);
        }

        // Write normal for specific model types
        if (model_type < 3) {
            for (const auto& norm : normal) {
                writeValue(f, norm);
            }
        }

        // Write first texture coordinates
        for (const auto& coord : texcoord0) {
            writeValue(f, coord);
        }

        // Write second texture coordinates for specific model types
        if (model_type > 1) {
            for (const auto& coord : texcoord1) {
                writeValue(f, coord);
            }
        }

        // Write weight and indices for specific model types
        if (model_type == 1) {
            writeValue(f, weight);
            writeValue(f, vertex_index);
            writeValue(f, bone_index);
        }
    }
    std::string to_string() const {
        std::ostringstream oss;
        oss << "Position: [" << std::fixed << std::setprecision(2)
            << position[0] << ", " << position[1] << ", " << position[2] << "]\n"
            << "Texcoord0: [" << texcoord0[0] << ", " << texcoord0[1] << "]\n"
            << "Normal: [" << normal[0] << ", " << normal[1] << ", " << normal[2] << "]\n"
            << "Weight: " << weight << "\n"
            << "Texcoord1: [" << texcoord1[0] << ", " << texcoord1[1] << "]\n"
            << "Vertex Index: " << vertex_index << "\n"
            << "Bone Index: " << bone_index << "\n";
        return oss.str();
    }
};

struct mefMeshVrtx_t : public mefMeshResource {
    std::vector<mefMeshVrtxEntry_t> entry; // Vector to hold vertex entries

    mefMeshVrtx_t() {}

    // Updated to readData
    void readData(std::ifstream &f, uint32_t count, int model_type) override {
        entry.clear(); // Clear any existing entries

        if (count > 0) {
            entry.resize(count); // Reserve space for entries
            for (size_t i = 0; i < count; ++i) {
                entry[i].readData(f, model_type); // Read each vertex entry
            }
        }
    }

    // Updated to writeData
    void writeData(std::ofstream &f) const override {
        for (const auto& entry_instance : entry) {
            entry_instance.writeData(f, entry_instance.bone_index); // Use model_type for writing
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "Vertices:\n";
        for (const auto& vertex : entry) {
            oss << vertex.to_string() << "\n"; // Ensure you implement to_string in mefMeshVrtxEntry_t
        }
        return oss.str();
    }
};


struct mefMeshCSphEntry_t {
    float position[3]; // Array to hold x, y, z coordinates
    float radius;
    uint16_t unk72;
    uint16_t unk73;
    uint16_t unk74;
    uint16_t unk75;

    mefMeshCSphEntry_t() {
        position[0] = 0.0f;
        position[1] = 0.0f;
        position[2] = 0.0f;
        radius = 0.0f;
        unk72 = 0;
        unk73 = 0;
        unk74 = 0;
        unk75 = 0;
    }

    // Updated to match the new virtual method signature
    void readData(std::ifstream &f) {
        readValue(f, position[0]); // Read x
        readValue(f, position[1]); // Read y
        readValue(f, position[2]); // Read z
        readValue(f, radius);      // Read radius
        readValue(f, unk72);       // Read unk72
        readValue(f, unk73);       // Read unk73
        readValue(f, unk74);       // Read unk74
        readValue(f, unk75);       // Read unk75
    }

    // Updated to match the new virtual method signature
    void writeData(std::ofstream &f) const {
        writeValue(f, position[0]); // Write x
        writeValue(f, position[1]); // Write y
        writeValue(f, position[2]); // Write z
        writeValue(f, radius);      // Write radius
        writeValue(f, unk72);       // Write unk72
        writeValue(f, unk73);       // Write unk73
        writeValue(f, unk74);       // Write unk74
        writeValue(f, unk75);       // Write unk75
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshCSphEntry_t(position=["
            << position[0] << ", "
            << position[1] << ", "
            << position[2] << "], radius="
            << radius << ", unk72="
            << unk72 << ", unk73="
            << unk73 << ", unk74="
            << unk74 << ", unk75="
            << unk75 << ")";
        return oss.str();
    }
};

struct mefMeshCSph_t : public mefMeshResource {
    std::vector<mefMeshCSphEntry_t> entry; // Vector to hold multiple entries

    // Updated to match the new virtual method signature
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear(); // Clear existing entries
        if (count > 0) {
            for (uint32_t i = 0; i < count; ++i) {
                mefMeshCSphEntry_t sph_entry;
                sph_entry.readData(f); // Use readData method
                entry.push_back(sph_entry);
            }
        }
    }

    // Updated to match the new virtual method signature
    void writeData(std::ofstream &f) const override {
        for (const auto &sph_entry : entry) {
            sph_entry.writeData(f); // Use writeData method
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshCSph_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};

struct mefMeshCVtxEntry_t {
    float position[3];  // Array to hold x, y, z coordinates
    uint32_t bone_index; // Assuming bone_index is a uint32_t
    uint32_t unk78;      // Assuming unk78 is a uint32_t

    mefMeshCVtxEntry_t() {
        position[0] = 0.0f;
        position[1] = 0.0f;
        position[2] = 0.0f;
        bone_index = 0;
        unk78 = 0;
    }

    // Updated readData function
    void readData(std::ifstream &f) {
        readValue(f, position[0]); // Read x
        readValue(f, position[1]); // Read y
        readValue(f, position[2]); // Read z
        readValue(f, bone_index);   // Read bone_index
        readValue(f, unk78);        // Read unk78
    }

    // Updated writeData function
    void writeData(std::ofstream &f) const {
        writeValue(f, position[0]); // Write x
        writeValue(f, position[1]); // Write y
        writeValue(f, position[2]); // Write z
        writeValue(f, bone_index);   // Write bone_index
        writeValue(f, unk78);        // Write unk78
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshCVtxEntry_t(position=["
            << position[0] << ", "
            << position[1] << ", "
            << position[2] << "], bone_index="
            << bone_index << ", unk78="
            << unk78 << ")";
        return oss.str();
    }
};

// Struct for vertex list
struct mefMeshCVtx_t : public mefMeshResource {
    std::vector<mefMeshCVtxEntry_t> entry;

    // Updated readData function
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count);
            for (size_t i = 0; i < count; ++i) {
                entry[i].readData(f); // Call the new readData method
            }
        }
    }

    // Updated writeData function
    void writeData(std::ofstream &f) const override {
        for (const auto &vtx_entry : entry) {
            vtx_entry.writeData(f); // Call the new writeData method
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshCVtx_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};

struct mefMeshSVtxEntry_t {
    float position[3];  // Array to hold x, y, z coordinates


    mefMeshSVtxEntry_t() {
        position[0] = 0.0f;
        position[1] = 0.0f;
        position[2] = 0.0f;
    }

    // Updated readData function
    void readData(std::ifstream &f) {
        readValue(f, position[0]); // Read x
        readValue(f, position[1]); // Read y
        readValue(f, position[2]); // Read z
    }

    // Updated writeData function
    void writeData(std::ofstream &f) const {
        writeValue(f, position[0]); // Write x
        writeValue(f, position[1]); // Write y
        writeValue(f, position[2]); // Write z
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshSVtxEntry_t(position=["
            << position[0] << ", "
            << position[1] << ", "
            << position[2] << "])";
        return oss.str();
    }
};

// Struct for vertex list
struct mefMeshSVtx_t : public mefMeshResource {
    std::vector<mefMeshSVtxEntry_t> entry;

    // Updated readData function
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count);
            for (size_t i = 0; i < count; ++i) {
                entry[i].readData(f); // Call the new readData method
            }
        }
    }

    // Updated writeData function
    void writeData(std::ofstream &f) const override {
        for (const auto &vtx_entry : entry) {
            vtx_entry.writeData(f); // Call the new writeData method
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshCVtx_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};


// Struct for face entry
struct mefMeshCFceEntry_t {
    uint16_t face[3];        // Array to hold vertex indices for the face
    int16_t material_index;   // Assuming signed short for material_index
    int16_t unk80;            // Assuming signed short for unk80
    int16_t unk81;            // Assuming signed short for unk81

    mefMeshCFceEntry_t() {
        face[0] = 0;
        face[1] = 0;
        face[2] = 0;
        material_index = 0;
        unk80 = 0;
        unk81 = 0;
    }

    void read(std::ifstream &f) {
        readValue(f, face[0]);          // Read vertex index 0
        readValue(f, face[1]);          // Read vertex index 1
        readValue(f, face[2]);          // Read vertex index 2
        readValue(f, material_index);    // Read material_index
        readValue(f, unk80);            // Read unk80
        readValue(f, unk81);            // Read unk81
    }

    void write(std::ofstream &f) const {
        writeValue(f, face[0]);         // Write vertex index 0
        writeValue(f, face[1]);         // Write vertex index 1
        writeValue(f, face[2]);         // Write vertex index 2
        writeValue(f, material_index);   // Write material_index
        writeValue(f, unk80);           // Write unk80
        writeValue(f, unk81);           // Write unk81
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshCFceEntry_t(face=["
            << face[0] << ", "
            << face[1] << ", "
            << face[2] << "], material_index="
            << material_index << ", unk80="
            << unk80 << ", unk81="
            << unk81 << ")";
        return oss.str();
    }
};

// Struct for face list
struct mefMeshCFce_t : public mefMeshResource {
    std::vector<mefMeshCFceEntry_t> entry;

    // Overridden readData function
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count);
            for (size_t i = 0; i < count; ++i) {
                entry[i].read(f);
            }
        }
    }

    // Overridden writeData function
    void writeData(std::ofstream &f) const override {
        for (const auto &fce_entry : entry) {
            fce_entry.write(f);
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshCFce_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};


struct mefMeshSFceEntry_t {
    uint32_t face[3];        // Array to hold vertex indices for the face
    int32_t unk90;            // Assuming signed short for unk80
    float unk91[3];            // Assuming signed short for unk81

    mefMeshSFceEntry_t() {
        face[0] = 0;
        face[1] = 0;
        face[2] = 0;
        unk90 = 0;
        unk91[0] = 0;
        unk91[1] = 0;
        unk91[2] = 0;
    }

    void read(std::ifstream &f) {
        readValue(f, face[0]);          // Read vertex index 0
        readValue(f, face[1]);          // Read vertex index 1
        readValue(f, face[2]);          // Read vertex index 2
        readValue(f, unk90);            // Read unk90
        readValue(f, unk91[0]);          // Read vertex index 0
        readValue(f, unk91[1]);          // Read vertex index 1
        readValue(f, unk91[2]);          // Read vertex index 2
    }

    void write(std::ofstream &f) const {
        writeValue(f, face[0]);         // Write vertex index 0
        writeValue(f, face[1]);         // Write vertex index 1
        writeValue(f, face[2]);         // Write vertex index 2
        writeValue(f, unk90);           // Write unk80
        writeValue(f, unk91[0]);         // Write vertex index 0
        writeValue(f, unk91[1]);         // Write vertex index 1
        writeValue(f, unk91[2]);         // Write vertex index 2
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshSFceEntry_t(face=["
            << face[0] << ", "
            << face[1] << ", "
            << face[2] << "], unk90="
            << unk90 << ", unk91=["
            << unk91[0] << ", "
            << unk91[1] << ", "
            << unk91[2] << "])";
        return oss.str();
    }
};

struct mefMeshSFce_t : public mefMeshResource {
    std::vector<mefMeshSFceEntry_t> entry;

    // Overridden readData function
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count);
            for (size_t i = 0; i < count; ++i) {
                entry[i].read(f);
            }
        }
    }

    // Overridden writeData function
    void writeData(std::ofstream &f) const override {
        for (const auto &fce_entry : entry) {
            fce_entry.write(f);
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshSFce_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};


struct mefMeshCMshEntry_t {
    uint32_t num_faces;
    uint32_t num_vertices;
    uint32_t num_materials;
    uint32_t num_spheres;
    uint32_t reserved[4]; // Array to hold reserved values

    mefMeshCMshEntry_t() : num_faces(0), num_vertices(0), num_materials(0), num_spheres(0) {
        std::fill(std::begin(reserved), std::end(reserved), 0);
    }

    void readData(std::ifstream &f) {
        readValue(f, num_faces);
        readValue(f, num_vertices);
        readValue(f, num_materials);
        readValue(f, num_spheres);
        for (size_t i = 0; i < 4; ++i) {
            readValue(f, reserved[i]);
        }
    }

    void writeData(std::ofstream &f) const {
        writeValue(f, num_faces);
        writeValue(f, num_vertices);
        writeValue(f, num_materials);
        writeValue(f, num_spheres);
        for (size_t i = 0; i < 4; ++i) {
            writeValue(f, reserved[i]);
        }
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshCMshEntry_t(num_faces=" << num_faces
            << ", num_vertices=" << num_vertices
            << ", num_materials=" << num_materials
            << ", num_spheres=" << num_spheres
            << ", reserved=[" << reserved[0] << ", " << reserved[1]
            << ", " << reserved[2] << ", " << reserved[3] << "])";
        return oss.str();
    }
};

struct mefMeshCMsh_t : public mefMeshResource {
    std::vector<mefMeshCMshEntry_t> entry;

    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize the vector to hold the number of entries
            for (size_t i = 0; i < count; ++i) {
                entry[i].readData(f); // Call readData for each entry
            }
        }
    }

    void writeData(std::ofstream &f) const override {
        for (const auto &msh_entry : entry) {
            msh_entry.writeData(f); // Call writeData for each entry
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshCMsh_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};

struct mefMeshCMatEntry_t {
    uint32_t unk48;
    int16_t unk49;  // Assuming signed short
    uint16_t unk50; // Assuming unsigned short
    uint32_t unk51;
    uint32_t unk52;

    mefMeshCMatEntry_t() : unk48(0), unk49(0), unk50(0), unk51(0), unk52(0) {}

    void read(std::ifstream &f) {
        readValue(f, unk48);
        readValue(f, unk49);
        readValue(f, unk50);
        readValue(f, unk51);
        readValue(f, unk52);
    }

    void write(std::ofstream &f) const {
        writeValue(f, unk48);
        writeValue(f, unk49);
        writeValue(f, unk50);
        writeValue(f, unk51);
        writeValue(f, unk52);
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshCMatEntry_t(unk48=" << unk48
            << ", unk49=" << unk49
            << ", unk50=" << unk50
            << ", unk51=" << unk51
            << ", unk52=" << unk52 << ")";
        return oss.str();
    }
};

struct mefMeshCMat_t : public mefMeshResource {
    std::vector<mefMeshCMatEntry_t> entry;

    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize to count
            for (size_t i = 0; i < count; ++i) {
                entry[i].read(f); // Read each material entry
            }
        }
    }

    void writeData(std::ofstream &f) const override {
        for (const auto &mat_entry : entry) {
            mat_entry.write(f); // Write each material entry
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshCMat_t(entry=[";
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << entry[i].to_string();
            if (i < entry.size() - 1) {
                oss << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};

struct mefMeshMrphEntry_t {
    uint32_t index;
    float delta[3];

    mefMeshMrphEntry_t() {
        index = 0;
        delta[0] = 0.0f;
        delta[1] = 0.0f;
        delta[2] = 0.0f;
    }

    // Updated to readData
    void readData(std::ifstream &f) {
        readValue(f, index);  // Read index
        readValue(f, delta[0]); // Read delta[0]
        readValue(f, delta[1]); // Read delta[1]
        readValue(f, delta[2]); // Read delta[2]
    }

    // Updated to writeData
    void writeData(std::ofstream &f) const {
        writeValue(f, index);  // Write index
        for (float value : delta) {
            writeValue(f, value); // Write each delta value
        }
    }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshMrphEntry_t(index=" << index << ", delta=["
            << delta[0] << ", " << delta[1] << ", " << delta[2] << "])";
        return oss.str();
    }
};
struct mefMeshMrph_t : public mefMeshResource {
    std::vector<std::vector<mefMeshMrphEntry_t>> entry;

    mefMeshMrph_t() : entry(16) {}  // Initialize entry with 16 vectors

    // Updated to readData
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        std::vector<uint32_t> counts(16); // Array to hold counts

        for (size_t i = 0; i < counts.size(); ++i) {
            readValue(f, counts[i]); // Read counts
        }

        entry.resize(counts.size());
        for (size_t i = 0; i < counts.size(); ++i) {
            if (counts[i] > 0) {
                entry[i].resize(counts[i]); // Resize entry for current count
                for (uint32_t j = 0; j < counts[i]; ++j) {
                    mefMeshMrphEntry_t mrph_entry;
                    mrph_entry.readData(f); // Updated to readData
                    entry[i][j] = mrph_entry; // Store the entry
                }
            }
        }
    }

    // Updated to writeData
    void writeData(std::ofstream &f) const override {
        for (const auto& count : entry) {
            uint32_t count_size = static_cast<uint32_t>(count.size());
            writeValue(f, count_size); // Write the count of entries
            for (const auto& entry_item : count) {
                entry_item.writeData(f); // Updated to writeData
            }
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "mefMeshMrph_t(entry=[";
        for (const auto& e : entry) {
            for (const auto& entry_item : e) {
                oss << entry_item.to_string() << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};


struct mefMeshSmes_t : public mefMeshResource { // shadow mesh?
    std::vector<std::array<uint32_t, 2>> entry; // Using array for fixed size faces

    mefMeshSmes_t() {
        entry.clear();
    }

    // Corrected readData function to match the base class
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear();
        if (count > 0) {
            entry.resize(count); // Resize to count
            for (size_t i = 0; i < count; ++i) {
                readValue(f, entry[i][0]); // Read first index
                readValue(f, entry[i][1]); // Read second index
            }
        }
    }

    // Corrected writeData function to match the base class
    void writeData(std::ofstream &f) const override {
        for (const auto& item : entry) {
            writeValue(f, item[0]); // Write first index
            writeValue(f, item[1]); // Write second index
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (const auto& item : entry) {
            oss << "Face Indices: [" << item[0] << ", " << item[1] << "]\n";
        }
        return oss.str();
    }
};


struct mefMeshChunk_t {
    uint32_t type;
    uint32_t data;
    uint32_t flag;
    uint32_t size;
    mefMeshResource* res;  // Pointer to base class for different resource types
    std::string type_debug;

    // Constructor
    mefMeshChunk_t()
        : type(0), data(0), flag(0), size(0), res(nullptr), type_debug("") {}

    // Destructor
    ~mefMeshChunk_t() {
        if (res) {
            delete res;
            res = nullptr;
        }
    }

    // Template method to create a resource and read its data
    template <typename T>
    mefMeshResource* createResource(std::ifstream &f, uint32_t count = 0, int model_type = 0) {
        T* resource = new T();
        resource->readData(f, count, model_type); // Corrected to call readData
        return resource;
    }

    // Template method to clone a resource
    template <typename T>
    mefMeshResource* cloneResource(const mefMeshResource* other_res) {
        const T* derived_res = dynamic_cast<const T*>(other_res);
        return derived_res ? new T(*derived_res) : nullptr;
    }

    // Copy Constructor
    mefMeshChunk_t(const mefMeshChunk_t& other)
        : type(other.type), data(other.data), flag(other.flag),
          size(other.size), type_debug(other.type_debug), res(nullptr) { // Initialize res to nullptr

        if (other.res) {
            // Clone the resource based on the type
            switch (static_cast<MeshResourceType>(other.type)) {
                case MeshResourceType::MESH:
                    res = cloneResource<mefMesh_t>(other.res);
                    break;
                case MeshResourceType::ATTA:
                    res = cloneResource<mefMeshAtta_t>(other.res);
                    break;
                case MeshResourceType::MVTX:
                    res = cloneResource<mefMeshMVtx_t>(other.res);
                    break;
                case MeshResourceType::RD3D:
                    res = cloneResource<mefMeshRD3D_t>(other.res);
                    break;
                case MeshResourceType::GLOW:
                    res = cloneResource<mefMeshGlow_t>(other.res);
                    break;
                case MeshResourceType::HIER:
                    res = cloneResource<mefMeshHier_t>(other.res);
                    break;
                case MeshResourceType::BNAM:
                    res = cloneResource<mefMeshBNam_t>(other.res);
                    break;
                case MeshResourceType::FACE:
                    res = cloneResource<mefMeshFace_t>(other.res);
                    break;
                case MeshResourceType::EDGE:
                    res = cloneResource<mefMeshEdge_t>(other.res);
                    break;
                case MeshResourceType::LTMP:
                    res = cloneResource<mefMeshLtMp_t>(other.res);
                    break;
                case MeshResourceType::REND:
                    res = cloneResource<mefMeshRend_t>(other.res);
                    break;
                case MeshResourceType::VRTX:
                    res = cloneResource<mefMeshVrtx_t>(other.res);
                    break;
                case MeshResourceType::CVTX:
                    res = cloneResource<mefMeshCVtx_t>(other.res);
                    break;
                case MeshResourceType::SVTX:
                    res = cloneResource<mefMeshSVtx_t>(other.res);
                    break;
                case MeshResourceType::CFCE:
                    res = cloneResource<mefMeshCFce_t>(other.res);
                    break;
                case MeshResourceType::SFAC:
                    res = cloneResource<mefMeshSFce_t>(other.res);
                    break;
                case MeshResourceType::CMAT:
                    res = cloneResource<mefMeshCMat_t>(other.res);
                    break;
                case MeshResourceType::CSPH:
                    res = cloneResource<mefMeshCSph_t>(other.res);
                    break;
                case MeshResourceType::MRPH:
                    res = cloneResource<mefMeshMrph_t>(other.res);
                    break;
                case MeshResourceType::CMSH:
                    res = cloneResource<mefMeshCMsh_t>(other.res);
                    break;
                case MeshResourceType::SMES:
                    res = cloneResource<mefMeshSmes_t>(other.res);
                    break;
                default:
                    res = nullptr; // Unknown type
                    break;
            }
        }
    }

    // Copy Assignment Operator
    mefMeshChunk_t& operator=(const mefMeshChunk_t& other) {
        if (this != &other) {
            // Clean up existing resource
            if (res) {
                delete res;
                res = nullptr; // Set to nullptr after deletion
            }

            // Copy basic fields
            type = other.type;
            data = other.data;
            flag = other.flag;
            size = other.size;
            type_debug = other.type_debug;

            // Clone the resource if it exists
            if (other.res) {
                switch (static_cast<MeshResourceType>(other.type)) {
                    case MeshResourceType::MESH:
                        res = cloneResource<mefMesh_t>(other.res);
                        break;
                    case MeshResourceType::ATTA:
                        res = cloneResource<mefMeshAtta_t>(other.res);
                        break;
                    case MeshResourceType::RD3D:
                        res = cloneResource<mefMeshRD3D_t>(other.res);
                        break;
                    case MeshResourceType::GLOW:
                        res = cloneResource<mefMeshGlow_t>(other.res);
                        break;
                    case MeshResourceType::MVTX:
                        res = cloneResource<mefMeshMVtx_t>(other.res);
                        break;
                    case MeshResourceType::HIER:
                        res = cloneResource<mefMeshHier_t>(other.res);
                        break;
                    case MeshResourceType::BNAM:
                        res = cloneResource<mefMeshBNam_t>(other.res);
                        break;
                    case MeshResourceType::FACE:
                        res = cloneResource<mefMeshFace_t>(other.res);
                        break;
                    case MeshResourceType::EDGE:
                        res = cloneResource<mefMeshEdge_t>(other.res);
                        break;
                    case MeshResourceType::LTMP:
                        res = cloneResource<mefMeshLtMp_t>(other.res);
                        break;
                    case MeshResourceType::REND:
                        res = cloneResource<mefMeshRend_t>(other.res);
                        break;
                    case MeshResourceType::VRTX:
                        res = cloneResource<mefMeshVrtx_t>(other.res);
                        break;
                    case MeshResourceType::CVTX:
                        res = cloneResource<mefMeshCVtx_t>(other.res);
                        break;
                    case MeshResourceType::SVTX:
                        res = cloneResource<mefMeshSVtx_t>(other.res);
                        break;
                    case MeshResourceType::CFCE:
                        res = cloneResource<mefMeshCFce_t>(other.res);
                        break;
                    case MeshResourceType::SFAC:
                        res = cloneResource<mefMeshSFce_t>(other.res);
                        break;
                    case MeshResourceType::CMAT:
                        res = cloneResource<mefMeshCMat_t>(other.res);
                        break;
                    case MeshResourceType::CSPH:
                        res = cloneResource<mefMeshCSph_t>(other.res);
                        break;
                    case MeshResourceType::MRPH:
                        res = cloneResource<mefMeshMrph_t>(other.res);
                        break;
                    case MeshResourceType::CMSH:
                        res = cloneResource<mefMeshCMsh_t>(other.res);
                        break;
                    case MeshResourceType::SMES:
                        res = cloneResource<mefMeshSmes_t>(other.res);
                        break;
                    default:
                        res = nullptr; // Unknown type, set res to nullptr
                        break;
                }
            } else {
                res = nullptr; // Ensure res is nullptr if the other resource is nullptr
            }
        }
        return *this;
    }

    // Read method to parse a chunk from the file
    void read(std::ifstream &f, mefMesh_t &header, mefMeshRD3D_t &render3D, bool verbose = false, bool stopOnNewChunk = false) {
        // Save the position in the file for reference
        std::streampos pos = f.tellg();
        readValue(f, type);
        readValue(f, data);
        readValue(f, flag);
        readValue(f, size);

        // Construct type_debug string for easier debugging
        type_debug.clear();
        for (int i = 0; i < 4; ++i) {
            type_debug += static_cast<char>((type >> (8 * (3 - i))) & 0xFF);
        }

        if (verbose) {
            std::cout << "[mefMeshChunk_t::read] Chunk { " << type_debug << " } @ " << pos << std::endl;
        }

        // Initialize res to nullptr to avoid any accidental usage before assignment
        if (res) {
            delete res;
            res = nullptr;
        }

        // Read the resource based on the type
        if (data > 0) {
            switch (static_cast<MeshResourceType>(type)) {
                case MeshResourceType::MESH: {

                    header.readData(f, data, 0); // Assuming model_type is 0 for RD3D
                    res = new mefMesh_t(header);

                    std::cout << "[mefMeshChunk_t::read] " << type_debug << ": Count# \t" << data << "\n" << res->to_string() << "\n----------------------------" << std::endl;
                    break;
                }
                case MeshResourceType::ATTA: {
                    uint32_t count = data / sizeof(mefMeshAttaEntry_t); // Adjust count based on actual data size
                    res = createResource<mefMeshAtta_t>(f, count, 0); // Assuming model_type is not needed
                    break;
                }
                case MeshResourceType::MVTX: {
                    uint32_t count = data / (sizeof(float) * 4); // Assuming each vertex has 4 floats
                    res = createResource<mefMeshMVtx_t>(f, count, 0); // Assuming model_type is not needed
                    break;
                }
                case MeshResourceType::RD3D: {
                    render3D.readData(f, data, header.model_type); // Assuming model_type is 0 for RD3D
                    res = new mefMeshRD3D_t(render3D);
                    break;
                }
                case MeshResourceType::GLOW: {
                    res = createResource<mefMeshGlow_t>(f, 1, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::HIER: {
                    uint32_t count = header.num_bones;
                    res = createResource<mefMeshHier_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::BNAM: {
                    uint32_t count = data / 16; // BNAME element size
                    res = createResource<mefMeshBNam_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::FACE: {
                    uint32_t count = (header.sum_c_faces > 0) ? header.sum_c_faces : data / (sizeof(uint16_t) * 3); // Each face has 3 uint16_t
                    res = createResource<mefMeshFace_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::EDGE: {
                    uint32_t stride = 8;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for EDGE chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'EDGE' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshEdge_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::LTMP: {
                    uint32_t stride = 8;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for LTMP chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'LTMP' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshLtMp_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::SMES: {
                    uint32_t stride = 8; // Default stride
                    // Calculate number of entries
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for SMES chunk." << std::endl;
                        break;
                    }
                    uint32_t count = (data - 4) / stride;
                    res = createResource<mefMeshSmes_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::REND: {
                    // Determine stride based on model_type
                    uint32_t stride = 32; // Default stride
                    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>header.model_type: \t" << header.model_type << std::endl;
                    if (header.model_type == 3) {
                        stride = 28;
                    } else if (header.model_type != 0) {
                        stride = 32;
                    }

                    // Calculate number of entries
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for REND chunk." << std::endl;
                        break;
                    }
                    uint32_t count = data / stride;
                    std::cout << "REND DATA: \t" << data << std::endl;
                    std::cout << "REND stride: \t" << stride << std::endl;
                    std::cout << "REND count: \t" << count << std::endl;
                    uint32_t remaining_bytes = data % stride;
                    std::cout << "REND remaining_bytes: \t" << remaining_bytes << std::endl;
                    if (remaining_bytes != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'REND' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }

                    std::cout << "[mefMeshChunk_t::read] Reading " << count << " Rend entries with stride " << stride << " bytes." << std::endl;

                    res = createResource<mefMeshRend_t>(f, data, header.model_type);
                    std::cout << "[mefMeshChunk_t::read] " << type_debug << ": Count# \t" << count << "\n" << res->to_string() << "\n----------------------------" << std::endl;

                    break;
                }
                case MeshResourceType::VRTX: {
                    uint32_t stride = 32;
                    if (header.model_type == 1) {
                        stride = 40;
                        }
                    uint32_t count = data / stride;
                    uint32_t remaining_bytes = data % stride;
                    if (remaining_bytes != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'VRTX' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshVrtx_t>(f, count, header.model_type);
                    break;
                }
                case MeshResourceType::CVTX: {
                    uint32_t stride = 20;//(header.sum_c_verts > 0) ? data / header.sum_c_verts : 0;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for CVTX chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'CVTX' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshCVtx_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::SVTX: {
                    uint32_t stride = 12;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for SVTX chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'CVTX' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshSVtx_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::CFCE: {
                    uint32_t stride = 12;//(header.sum_c_faces > 0) ? data / header.sum_c_faces : 0;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for CFCE chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'CFCE' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshCFce_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::SFAC: {
                    uint32_t stride = 28;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[mefMeshChunk_t::read] Invalid stride calculated for SFAC chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[mefMeshChunk_t::read] Warning: 'SFAC' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<mefMeshSFce_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::CMAT: {
                    uint32_t count = data / sizeof(mefMeshCMatEntry_t); // Adjust count based on material size
                    res = createResource<mefMeshCMat_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::CSPH: {
                    uint32_t count = data / sizeof(mefMeshCSphEntry_t); // Sphere size is sizeof(mefMeshCSphEntry_t)
                    res = createResource<mefMeshCSph_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::MRPH: {
                    // Assuming 'MRPH' handles multiple morph targets internally
                    res = createResource<mefMeshMrph_t>(f, 0, 0); // Assuming count and model_type are handled internally
                    break;
                }
                case MeshResourceType::CMSH: {
                    uint32_t count = data / sizeof(mefMeshCMshEntry_t); // CMSH size
                    res = createResource<mefMeshCMsh_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                default:
                    std::cerr << "[mefMeshChunk_t::read] Unexpected Chunk {" << type_debug << "} @ " << pos << ", Parsing Halted." << std::endl;
                    if (stopOnNewChunk) {
                        return; // Stop if required
                    }
            }

            // Print verbose information if necessary
            if (verbose && res) {
                std::cout << "[mefMeshChunk_t::read] " << type_debug << ": Count# \t" << data << "\n" << res->to_string() << "\n----------------------------" << std::endl;
            }
        }

        // Calculate padding and adjust file pointer to the next chunk
        uint32_t padding = (4 - (data % 4)) % 4;
        f.seekg(pos + static_cast<std::streamoff>(16 + data + padding));
    }

    // Write method to serialize the chunk back to the file
    void write(std::ofstream &f) const {
        writeValue(f, type);   // Write type
        writeValue(f, data);   // Write data
        writeValue(f, flag);   // Write flag
        writeValue(f, size);   // Write size

        if (res) {
            res->writeData(f); // Call writeData on the resource
        }
    }

    // Method to convert chunk information to string
    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefMeshChunk_t(type=0x" << std::hex << type << std::dec
            << ", data=" << data
            << ", flag=" << flag
            << ", size=" << size
            << ", type_debug=\"" << type_debug << "\")";
        if (res) {
            oss << "\nResource Details:\n" << res->to_string();
        }
        return oss.str();
    }
};


struct mefFile_t {
    uint32_t file_type;     // File type
    uint32_t file_size;     // File size
    uint32_t file_unk1;     // Unknown field 1
    uint32_t file_unk2;     // Unknown field 2
    uint32_t content_type;  // Content type
    std::vector<mefMeshChunk_t> content; // Vector to hold content

    mefFile_t()
        : file_type(static_cast<uint32_t>(MeshResourceType::IFLF)), // Replaced with enum value
          file_size(0),
          file_unk1(4),
          file_unk2(0),
          content_type(static_cast<uint32_t>(MeshResourceType::MECO)) { // Replaced with enum value
    }

    // Function to get content by block name
    const mefMeshChunk_t* get_content(const std::string& block_name) const {
        const mefMeshChunk_t* data = nullptr;
        for (const auto& c : content) {
            // Assuming mefMeshChunk_t has a member 'type_debug'
            if (c.type_debug == block_name) {
                data = &c; // Returning a pointer to the chunk
                break;
            }
        }
        return data;
    }


    // Function to read from a file
    bool readData(std::ifstream &f, float mscale = 0.0254f, bool importToScene = true) {
        bool result = false;

        if (f.is_open()) {
            std::streampos file_pos = f.tellg();
            readValue(f, file_type);

            // Validate file type
            if (file_type == static_cast<uint32_t>(MeshResourceType::IFLF)) {  // 'IFLF'
                readValue(f, file_size);
                readValue(f, file_unk1);
                readValue(f, file_unk2);

                // Calculate file end position based on file_size
                std::streampos file_end = file_pos + file_size;

                // Process only if we are within the file bounds
                if (f.tellg() < file_end) {
                    mefMesh_t mHeader;      // Initialize mesh header
                    mefMeshRD3D_t mRender3D; // Initialize render 3D data

                    readValue(f, content_type);

                    // Check for correct content type
                    if (content_type == static_cast<uint32_t>(MeshResourceType::MECO)) {  // 'MECO'
                        while (f.tellg() < file_end) {
                            mefMeshChunk_t chunk;

                            // Read chunk data and validate
                            chunk.read(f, mHeader, mRender3D); // Use the correct read method

                            // Check if the chunk is valid before adding
                            if (chunk.res != nullptr) {
                                content.push_back(chunk); // Append chunk to content
                            } else {
                                std::cerr << "Warning: Chunk read failed or is null." << std::endl;
                            }
                        }

                        // Validate file end position
                        if (f.tellg() == file_end) {
                            std::cout << "Done" << std::endl;
                        } else {
                            std::cerr << "Warning: File ended unexpectedly at " << f.tellg() << " bytes." << std::endl;
                        }

                        result = true;

                        // Optionally build the scene
                        if (importToScene) {
                            // Uncomment the following line to implement the build function
                            // build(mHeader, mscale); // Implement the build function
                        }
                    } else {
                        std::cerr << "Unsupported Content Type: " << content_type << std::endl;
                    }
                } else {
                    std::cerr << "Error: File read position exceeds expected file end." << std::endl;
                }
            } else {
                std::cerr << "Invalid File Header: Expected IFLF type." << std::endl;
            }
        } else {
            std::cerr << "Failed to open file." << std::endl;
        }
        return result;
    }

    // Function to print object information
    std::string to_string() const {
        std::ostringstream oss;
        oss << "mefFile_t(file_type=" << file_type
            << ", file_size=" << file_size
            << ", content_type=" << content_type
            << ", content_count=" << content.size() << ")";
        return oss.str();
    }

    void assign_parent_indices(size_t bone_index,
                               const std::vector<std::vector<size_t>>& bone_children,
                               std::vector<int>& parent_indices) {
        for (size_t child_index : bone_children[bone_index]) {
            parent_indices[child_index] = static_cast<int>(bone_index);
            assign_parent_indices(child_index, bone_children, parent_indices);
        }
    }

    bool exportOBJ(const std::string& filename, float mscale = 0.0254f, bool merge_submeshes = true, bool export_materials = true) {
        // Open the file for writing
        std::ofstream objFile(filename);
        if (!objFile.is_open()) {
            std::cerr << "[exportOBJ] Failed to open OBJ file for writing: " << filename << std::endl;
            return false;
        }
        std::cout << "[exportOBJ] Successfully opened OBJ file: " << filename << std::endl;

        // Retrieve necessary chunks
        const mefMeshChunk_t* hier_chunk = get_content("HIER");
        const mefMeshChunk_t* bnam_chunk = get_content("BNAM");
        const mefMeshChunk_t* vrtx_chunk = get_content("VRTX");
        const mefMeshChunk_t* face_chunk = get_content("FACE");
        const mefMeshChunk_t* rend_chunk = get_content("REND");

        // Check for required chunks
        if (!vrtx_chunk || !face_chunk || !rend_chunk) {
            std::cerr << "[exportOBJ] Required chunks (VRTX, FACE, REND) are missing." << std::endl;
            objFile.close();
            return false;
        }

        // Initialize model_type
        int model_type = 0; // Default model_type
        // If model_type is determined by the presence of HIER and BNAM, set accordingly
        if (hier_chunk && bnam_chunk) {
            // Assuming model_type is determined from HIER or BNAM chunks
            // Replace the following line with actual logic to set model_type
            // For example, model_type could be stored in a specific field within HIER or BNAM
            // Here, I'm setting it to 1 if HIER and BNAM are present
            model_type = 1;
        } else {
            // If HIER and BNAM are missing, set model_type to a value that represents meshes without bones
            // This depends on your specific application logic
            model_type = 0;
        }

        // Build bone hierarchy if available
        std::vector<std::string> bone_names;
        std::vector<std::array<float, 3>> bone_positions;
        std::vector<int> parent_indices;

        if (hier_chunk && bnam_chunk) {
            mefMeshHier_t* hier = dynamic_cast<mefMeshHier_t*>(hier_chunk->res);
            mefMeshBNam_t* bnam = dynamic_cast<mefMeshBNam_t*>(bnam_chunk->res);

            if (hier && bnam && hier->num_children.size() == bnam->names.size()) {
                size_t num_bones = hier->num_children.size();
                bone_names = bnam->names;
                bone_positions.resize(num_bones);
                parent_indices.resize(num_bones, -1);

                // Build parent indices based on hierarchy
                std::vector<std::vector<size_t>> bone_children(num_bones);
                size_t currentIndex = 1;
                for (size_t i = 0; i < num_bones; ++i) {
                    uint8_t numChildren = hier->num_children[i];
                    for (uint8_t j = 0; j < numChildren; ++j) {
                        if (currentIndex < num_bones) {
                            bone_children[i].push_back(currentIndex);
                            ++currentIndex;
                        } else {
                            std::cerr << "[exportOBJ] Warning: currentIndex (" << currentIndex
                                      << ") exceeds number of bones (" << num_bones << ")." << std::endl;
                            break;
                        }
                    }
                }

                // Recursively assign parent indices using a helper function
                assign_parent_indices(0, bone_children, parent_indices);

                // Compute bone positions
                for (size_t i = 0; i < num_bones; ++i) {
                    std::array<float, 3> head_pos = {
                        hier->position[i][0] * mscale,
                        hier->position[i][1] * mscale,
                        hier->position[i][2] * mscale
                    };
                    if (parent_indices[i] >= 0) {
                        const auto& parent_pos = bone_positions[parent_indices[i]];
                        bone_positions[i][0] = head_pos[0] + parent_pos[0];
                        bone_positions[i][1] = head_pos[1] + parent_pos[1];
                        bone_positions[i][2] = head_pos[2] + parent_pos[2];
                    } else {
                        bone_positions[i] = head_pos;
                    }
                }
            } else {
                std::cerr << "[exportOBJ] Invalid HIER or BNAM data." << std::endl;
            }
        } else {
            std::cout << "[exportOBJ] Bone hierarchy not available. Exporting mesh without bone transformations." << std::endl;
        }

        // Now process vertices and faces
        mefMeshVrtx_t* vrtx = dynamic_cast<mefMeshVrtx_t*>(vrtx_chunk->res);
        mefMeshFace_t* face = dynamic_cast<mefMeshFace_t*>(face_chunk->res);
        mefMeshRend_t* rend = dynamic_cast<mefMeshRend_t*>(rend_chunk->res);

        if (!vrtx || !face || !rend) {
            std::cerr << "[exportOBJ] Failed to cast resource chunks." << std::endl;
            objFile.close();
            return false;
        }

        if (rend->entry.empty()) {
            std::cerr << "[exportOBJ] No rend entries to process." << std::endl;
            objFile.close();
            return false;
        }

        // Write OBJ file header
        objFile << "# Exported by mefFile_t::exportOBJ\n";

        // Handle group naming based on merge_submeshes
        if (merge_submeshes) {
            objFile << "g CombinedMesh\n";
        }

        size_t global_vertex_offset = 1; // OBJ indices start at 1

        // Loop through each sub-mesh
        for (size_t smesh_index = 0; smesh_index < rend->entry.size(); ++smesh_index) {
            const auto& smesh = rend->entry[smesh_index];

            size_t face_pos = smesh.face_pos / 3; // Assuming face_pos is index into face entries
            size_t face_count = smesh.face_count;
            size_t vert_pos = smesh.vertex_pos;
            size_t vert_count = smesh.vertex_count;

            if (!merge_submeshes) {
                // Write group name for each submesh
                objFile << "g Mesh_" << smesh_index << "\n";
            }

            // Write vertices, normals, and texture coordinates
            for (size_t i = vert_pos; i < vert_pos + vert_count; ++i) {
                if (i >= vrtx->entry.size()) {
                    std::cerr << "[exportOBJ] Vertex index out of range: " << i << std::endl;
                    objFile.close();
                    return false;
                }
                const auto& vertex_entry = vrtx->entry[i];

                // Apply bone transformation if available
                std::array<float, 3> node_pos = {0.0f, 0.0f, 0.0f};
                if (!bone_positions.empty() && vertex_entry.bone_index >= 0 &&
                    static_cast<size_t>(vertex_entry.bone_index) < bone_positions.size()) {
                    node_pos = bone_positions[vertex_entry.bone_index];
                }

                // Compute world position
                float x = vertex_entry.position[0] * mscale + node_pos[0];
                float y = vertex_entry.position[1] * mscale + node_pos[1];
                float z = vertex_entry.position[2] * mscale + node_pos[2];

                // Write vertex position
                objFile << "v " << x << " " << y << " " << z << "\n";

                // Write vertex normal
                objFile << "vn " << vertex_entry.normal[0] << " " << vertex_entry.normal[1] << " " << vertex_entry.normal[2] << "\n";

                // Write texture coordinates
                objFile << "vt " << vertex_entry.texcoord0[0] << " " << vertex_entry.texcoord0[1] << "\n";
            }

            // Write faces
            for (size_t i = face_pos; i < face_pos + face_count; ++i) {
                if (i >= face->entry.size()) {
                    std::cerr << "[exportOBJ] Face index out of range: " << i << std::endl;
                    objFile.close();
                    return false;
                }
                const auto& face_indices = face->entry[i];

                // Adjust indices for OBJ (starting from 1) and considering vertex offsets
                size_t idx0 = face_indices[0] - vert_pos + global_vertex_offset;
                size_t idx1 = face_indices[2] - vert_pos + global_vertex_offset;
                size_t idx2 = face_indices[1] - vert_pos + global_vertex_offset;

                // Write face with vertex/texture/normal indices
                objFile << "f " << idx0 << "/" << idx0 << "/" << idx0 << " "
                        << idx1 << "/" << idx1 << "/" << idx1 << " "
                        << idx2 << "/" << idx2 << "/" << idx2 << "\n";
            }

            // Update global vertex offset
            global_vertex_offset += vert_count;
        }

        objFile.close();
        std::cout << "[exportOBJ] OBJ file exported successfully: " << filename << std::endl;
        return true;
    }


};



struct pngFile_t {
    // Computes the CRC32 checksum for a given data buffer.
    uint32_t calculateCRC32(const uint8_t* data, size_t length) {
        static uint32_t table[256];
        static bool have_table = false;
        if (!have_table) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t rem = i;
                for (uint8_t j = 0; j < 8; j++) {
                    if (rem & 1)
                        rem = (rem >> 1) ^ 0xEDB88320;
                    else
                        rem >>= 1;
                }
                table[i] = rem;
            }
            have_table = true;
        }

        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; i++) {
            uint8_t octet = data[i];
            crc = (crc >> 8) ^ table[(crc & 0xFF) ^ octet];
        }
        return ~crc;
    }

    // Computes the Adler-32 checksum for a given data buffer.
    uint32_t adler32(const uint8_t* data, size_t length) {
        const uint32_t MOD_ADLER = 65521;
        uint32_t a = 1, b = 0;
        for (size_t i = 0; i < length; ++i) {
            a = (a + data[i]) % MOD_ADLER;
            b = (b + a) % MOD_ADLER;
        }
        return (b << 16) | a;
    }

    // Compresses data using zlib's deflate with no compression (compression level 0).
    bool compressZlib(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        // Initialize zlib header for no compression
        output.push_back(0x78); // CMF: Compression Method and Flags
        output.push_back(0x01); // FLG: Flags

        // Implement a single stored block
        size_t blockSize = input.size();
        if (blockSize > 65535) {
            std::cerr << "compressZlib: Data too large for single block." << std::endl;
            return false;
        }

        uint8_t bfinal = 1; // This is the final block
        uint8_t btype = 0;  // No compression
        uint8_t header = (bfinal << 0) | (btype << 1);
        output.push_back(header);

        // Write LEN and NLEN in little-endian
        uint16_t len = static_cast<uint16_t>(blockSize);
        uint16_t nlen = ~len;
        output.push_back(len & 0xFF);
        output.push_back((len >> 8) & 0xFF);
        output.push_back(nlen & 0xFF);
        output.push_back((nlen >> 8) & 0xFF);

        // Write the actual data
        output.insert(output.end(), input.begin(), input.end());

        // Write Adler-32 checksum in big-endian
        uint32_t adler = adler32(input.data(), input.size());
        output.push_back((adler >> 24) & 0xFF);
        output.push_back((adler >> 16) & 0xFF);
        output.push_back((adler >> 8) & 0xFF);
        output.push_back(adler & 0xFF);

        return true;
    }

    // Helper function to write a 32-bit unsigned integer in big-endian to an output stream
    void writeUInt32BE(std::ostream& out, uint32_t value) {
        out.put((value >> 24) & 0xFF);
        out.put((value >> 16) & 0xFF);
        out.put((value >> 8) & 0xFF);
        out.put(value & 0xFF);
    }

    // Helper function to write a 32-bit unsigned integer in big-endian to a buffer
    void writeUInt32BE(std::vector<uint8_t>& buffer, uint32_t value) {
        buffer.push_back((value >> 24) & 0xFF);
        buffer.push_back((value >> 16) & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
        buffer.push_back(value & 0xFF);
    }

    // Helper function to write a 4-byte ASCII string as chunk type
    void writeChunkType(std::ostream& out, const char* type) {
        out.put(type[0]);
        out.put(type[1]);
        out.put(type[2]);
        out.put(type[3]);
    }

    // Helper function to write a 4-byte ASCII string as chunk type to buffer
    void writeChunkType(std::vector<uint8_t>& buffer, const char* type) {
        buffer.push_back(static_cast<uint8_t>(type[0]));
        buffer.push_back(static_cast<uint8_t>(type[1]));
        buffer.push_back(static_cast<uint8_t>(type[2]));
        buffer.push_back(static_cast<uint8_t>(type[3]));
    }

    bool savePNG(const char* filePath, const uint8_t* buf, unsigned int width, unsigned int height, unsigned char channels, bool flipV, bool reverse) {
        /*
        * This function creates a minimal PNG file with uncompressed image data.
        * It uses zlib with no compression, which is compliant with the PNG specification.
        */

        // Open the output file in binary mode
        std::ofstream outputFile(filePath, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to open output PNG file: " << filePath << std::endl;
            return false;
        }

        // PNG Signature (8 bytes)
        uint8_t pngSignature[8] = { 137,80,78,71,13,10,26,10 };
        outputFile.write(reinterpret_cast<const char*>(pngSignature), 8);

        // === IHDR Chunk ===
        uint32_t ihdrDataSize = 13;
        const char ihdrTypeStr[5] = "IHDR";
        uint32_t ihdrType = 0x49484452; // "IHDR" in ASCII

        // IHDR Data
        uint8_t ihdrData[13] = {0};
        ihdrData[0] = (width >> 24) & 0xFF;
        ihdrData[1] = (width >> 16) & 0xFF;
        ihdrData[2] = (width >> 8) & 0xFF;
        ihdrData[3] = (width) & 0xFF;

        ihdrData[4] = (height >> 24) & 0xFF;
        ihdrData[5] = (height >> 16) & 0xFF;
        ihdrData[6] = (height >> 8) & 0xFF;
        ihdrData[7] = (height) & 0xFF;

        ihdrData[8] = 8; // Bit depth
        ihdrData[9] = (channels == 4) ? 6 : 2; // Color type: 6 (RGBA) or 2 (RGB)
        ihdrData[10] = 0; // Compression method
        ihdrData[11] = 0; // Filter method
        ihdrData[12] = 0; // Interlace method

        // Write IHDR Chunk Length (Big Endian)
        writeUInt32BEStream(outputFile, ihdrDataSize);

        // Write IHDR Chunk Type
        writeChunkTypeStream(outputFile, ihdrTypeStr);

        // Write IHDR Chunk Data
        outputFile.write(reinterpret_cast<const char*>(ihdrData), ihdrDataSize);

        // Calculate IHDR CRC over chunk type + data
        std::vector<uint8_t> ihdrForCRC;
        ihdrForCRC.insert(ihdrForCRC.end(), ihdrData, ihdrData + ihdrDataSize);
        uint32_t ihdrCRC = calculateCRC32(reinterpret_cast<const uint8_t*>(ihdrTypeStr), 4);
        ihdrCRC = calculateCRC32(ihdrData, ihdrDataSize);
        ihdrCRC = calculateCRC32(reinterpret_cast<const uint8_t*>(ihdrTypeStr), 4 + ihdrDataSize);
        ihdrCRC = calculateCRC32(reinterpret_cast<const uint8_t*>(ihdrTypeStr), 4);
        // Correct CRC calculation: it should include chunk type + data
        std::vector<uint8_t> ihdrCRCBuffer;
        ihdrCRCBuffer.insert(ihdrCRCBuffer.end(), ihdrTypeStr, ihdrTypeStr + 4);
        ihdrCRCBuffer.insert(ihdrCRCBuffer.end(), ihdrData, ihdrData + ihdrDataSize);
        ihdrCRC = calculateCRC32(ihdrCRCBuffer.data(), ihdrCRCBuffer.size());

        // Write IHDR CRC (Big Endian)
        writeUInt32BEStream(outputFile, ihdrCRC);

        // === IDAT Chunk ===
        const char idatTypeStr[5] = "IDAT";
        uint32_t idatType = 0x49444154; // "IDAT" in ASCII

        // Prepare raw image data with filter byte (0 for no filter)
        size_t rowSize = width * channels + 1; // 1 byte for filter type
        size_t imageDataSize = rowSize * height;
        std::vector<uint8_t> rawImageData(imageDataSize, 0); // Initialize all filter bytes to 0

        // Copy pixel data into rawImageData with flipping and reversing as needed
        for (unsigned int y = 0; y < height; ++y) {
            unsigned int srcY = flipV ? (height - 1 - y) : y;
            for (unsigned int x = 0; x < width; ++x) {
                unsigned int srcX = reverse ? (width - 1 - x) : x;
                size_t srcIndex = (srcY * width + srcX) * channels;
                size_t destIndex = y * rowSize + 1 + x * channels;
                memcpy(&rawImageData[destIndex], &buf[srcIndex], channels);
            }
        }

        // Compress the image data using compressZlib
        std::vector<uint8_t> compressedData;
        if (!compressZlib(rawImageData, compressedData)) {
            std::cerr << "Failed to compress image data." << std::endl;
            return false;
        }

        // Write IDAT Chunk Length (Big Endian)
        writeUInt32BEStream(outputFile, static_cast<uint32_t>(compressedData.size()));

        // Write IDAT Chunk Type
        writeChunkTypeStream(outputFile, idatTypeStr);

        // Write IDAT Chunk Data
        outputFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());

        // Calculate IDAT CRC over chunk type + data
        std::vector<uint8_t> idatCRCBuffer;
        idatCRCBuffer.insert(idatCRCBuffer.end(), idatTypeStr, idatTypeStr + 4);
        idatCRCBuffer.insert(idatCRCBuffer.end(), compressedData.begin(), compressedData.end());
        uint32_t idatCRC = calculateCRC32(idatCRCBuffer.data(), idatCRCBuffer.size());

        // Write IDAT CRC (Big Endian)
        writeUInt32BEStream(outputFile, idatCRC);

        // === IEND Chunk ===
        const char iendTypeStr[5] = "IEND";
        uint32_t iendDataSize = 0;
        uint32_t iendType = 0x49454E44; // "IEND" in ASCII

        // Write IEND Chunk Length (Big Endian)
        writeUInt32BEStream(outputFile, iendDataSize);

        // Write IEND Chunk Type
        writeChunkTypeStream(outputFile, iendTypeStr);

        // No IEND Chunk Data

        // Calculate IEND CRC over chunk type only
        uint32_t iendCRC = calculateCRC32(reinterpret_cast<const uint8_t*>(iendTypeStr), 4);

        // Write IEND CRC (Big Endian)
        writeUInt32BEStream(outputFile, iendCRC);

        // Close the file
        outputFile.close();

        return true;
    }

    // Writes PNG data to a memory buffer
    bool writePNG(std::vector<uint8_t>& buffer, const uint8_t* buf, unsigned int width, unsigned int height, unsigned char channels, bool flipV, bool reverse) {
        /*
        * This function creates a minimal PNG data stream in a memory buffer.
        * It uses zlib with no compression, which is compliant with the PNG specification.
        */

        // Clear and reserve space for the buffer
        buffer.clear();
        buffer.reserve(8 + 25 + 25 + 12 + (width * height * channels) * 2); // Reserve ample space

        // PNG Signature (8 bytes)
        uint8_t pngSignature[8] = { 137,80,78,71,13,10,26,10 };
        buffer.insert(buffer.end(), pngSignature, pngSignature + 8);

        // === IHDR Chunk ===
        uint32_t ihdrDataSize = 13;
        const char ihdrTypeStr[5] = "IHDR";
        uint32_t ihdrType = 0x49484452; // "IHDR" in ASCII

        // IHDR Data
        uint8_t ihdrData[13] = {0};
        ihdrData[0] = (width >> 24) & 0xFF;
        ihdrData[1] = (width >> 16) & 0xFF;
        ihdrData[2] = (width >> 8) & 0xFF;
        ihdrData[3] = (width) & 0xFF;

        ihdrData[4] = (height >> 24) & 0xFF;
        ihdrData[5] = (height >> 16) & 0xFF;
        ihdrData[6] = (height >> 8) & 0xFF;
        ihdrData[7] = (height) & 0xFF;

        ihdrData[8] = 8; // Bit depth
        ihdrData[9] = (channels == 4) ? 6 : 2; // Color type: 6 (RGBA) or 2 (RGB)
        ihdrData[10] = 0; // Compression method
        ihdrData[11] = 0; // Filter method
        ihdrData[12] = 0; // Interlace method

        // Append IHDR Chunk Length (Big Endian)
        writeUInt32BEBuffer(buffer, ihdrDataSize);

        // Append IHDR Chunk Type
        writeChunkTypeBuffer(buffer, ihdrTypeStr);

        // Append IHDR Chunk Data
        buffer.insert(buffer.end(), ihdrData, ihdrData + ihdrDataSize);

        // Calculate IHDR CRC over chunk type + data
        std::vector<uint8_t> ihdrCRCBuffer;
        ihdrCRCBuffer.insert(ihdrCRCBuffer.end(), ihdrTypeStr, ihdrTypeStr + 4);
        ihdrCRCBuffer.insert(ihdrCRCBuffer.end(), ihdrData, ihdrData + ihdrDataSize);
        uint32_t ihdrCRC = calculateCRC32(ihdrCRCBuffer.data(), ihdrCRCBuffer.size());

        // Append IHDR CRC (Big Endian)
        writeUInt32BEBuffer(buffer, ihdrCRC);

        // === IDAT Chunk ===
        const char idatTypeStr[5] = "IDAT";
        uint32_t idatType = 0x49444154; // "IDAT" in ASCII

        // Prepare raw image data with filter byte (0 for no filter)
        size_t rowSize = width * channels + 1; // 1 byte for filter type
        size_t imageDataSize = rowSize * height;
        std::vector<uint8_t> rawImageData(imageDataSize, 0); // Initialize all filter bytes to 0

        // Copy pixel data into rawImageData with flipping and reversing as needed
        for (unsigned int y = 0; y < height; ++y) {
            unsigned int srcY = flipV ? (height - 1 - y) : y;
            for (unsigned int x = 0; x < width; ++x) {
                unsigned int srcX = reverse ? (width - 1 - x) : x;
                size_t srcIndex = (srcY * width + srcX) * channels;
                size_t destIndex = y * rowSize + 1 + x * channels;
                memcpy(&rawImageData[destIndex], &buf[srcIndex], channels);
            }
        }

        // Compress the image data using compressZlib
        std::vector<uint8_t> compressedData;
        if (!compressZlib(rawImageData, compressedData)) {
            std::cerr << "Failed to compress image data." << std::endl;
            return false;
        }

        // Append IDAT Chunk Length (Big Endian)
        writeUInt32BEBuffer(buffer, static_cast<uint32_t>(compressedData.size()));

        // Append IDAT Chunk Type
        writeChunkTypeBuffer(buffer, idatTypeStr);

        // Append IDAT Chunk Data
        buffer.insert(buffer.end(), compressedData.begin(), compressedData.end());

        // Calculate IDAT CRC over chunk type + data
        std::vector<uint8_t> idatCRCBuffer;
        idatCRCBuffer.insert(idatCRCBuffer.end(), idatTypeStr, idatTypeStr + 4);
        idatCRCBuffer.insert(idatCRCBuffer.end(), compressedData.begin(), compressedData.end());
        uint32_t idatCRC = calculateCRC32(idatCRCBuffer.data(), idatCRCBuffer.size());

        // Append IDAT CRC (Big Endian)
        writeUInt32BEBuffer(buffer, idatCRC);

        // === IEND Chunk ===
        const char iendTypeStr[5] = "IEND";
        uint32_t iendDataSize = 0;
        uint32_t iendType = 0x49454E44; // "IEND" in ASCII

        // Append IEND Chunk Length (Big Endian)
        writeUInt32BEBuffer(buffer, iendDataSize);

        // Append IEND Chunk Type
        writeChunkTypeBuffer(buffer, iendTypeStr);

        // No IEND Chunk Data

        // Calculate IEND CRC over chunk type only
        uint32_t iendCRC = calculateCRC32(reinterpret_cast<const uint8_t*>(iendTypeStr), 4);

        // Append IEND CRC (Big Endian)
        writeUInt32BEBuffer(buffer, iendCRC);

        return true;
    }

private:
    // Helper function to write a 32-bit unsigned integer in big-endian to an output stream
    void writeUInt32BEStream(std::ostream& out, uint32_t value) {
        out.put((value >> 24) & 0xFF);
        out.put((value >> 16) & 0xFF);
        out.put((value >> 8) & 0xFF);
        out.put(value & 0xFF);
    }

    // Helper function to write a 32-bit unsigned integer in big-endian to a buffer
    void writeUInt32BEBuffer(std::vector<uint8_t>& buffer, uint32_t value) {
        buffer.push_back((value >> 24) & 0xFF);
        buffer.push_back((value >> 16) & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
        buffer.push_back(value & 0xFF);
    }

    // Helper function to write a 4-byte ASCII string as chunk type to an output stream
    void writeChunkTypeStream(std::ostream& out, const char* type) {
        out.put(type[0]);
        out.put(type[1]);
        out.put(type[2]);
        out.put(type[3]);
    }

    // Helper function to write a 4-byte ASCII string as chunk type to a buffer
    void writeChunkTypeBuffer(std::vector<uint8_t>& buffer, const char* type) {
        buffer.push_back(static_cast<uint8_t>(type[0]));
        buffer.push_back(static_cast<uint8_t>(type[1]));
        buffer.push_back(static_cast<uint8_t>(type[2]));
        buffer.push_back(static_cast<uint8_t>(type[3]));
    }
};

struct tgaFile_t {
    // Header fields
    uint8_t  id_length;          // Length of ID field
    uint8_t  color_map_type;     // Color map type
    uint8_t  image_type;         // Image type code
    uint16_t color_map_origin;   // Color map origin
    uint16_t color_map_length;   // Color map length
    uint8_t  color_map_depth;    // Depth of color map entries
    uint16_t x_origin;           // X-origin of image
    uint16_t y_origin;           // Y-origin of image
    uint16_t width;              // Width of image
    uint16_t height;             // Height of image
    uint8_t  pixel_depth;        // Pixel depth
    uint8_t  image_descriptor;   // Image descriptor

    // Data fields
    std::vector<uint8_t> id_field;        // ID field data
    std::vector<uint8_t> color_map_data;  // Color map data
    std::vector<uint8_t> image_data;      // Image data (decoded)

    // Opens a TGA image file and reads its contents
    bool open(const char* filename) {
        // Open file in binary mode
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }

        // Get file size
        std::ifstream::pos_type fileSize = file.tellg();
        if (fileSize < 18) { // Minimum TGA header size
            std::cerr << "Error: File size is too small to be a valid TGA file." << std::endl;
            return false;
        }
        file.seekg(0, std::ios::beg);

        // Read entire file into buffer
        std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
        if (!file.read(reinterpret_cast<char*>(&buffer[0]), fileSize)) {
            std::cerr << "Error: Failed to read file " << filename << std::endl;
            return false;
        }
        file.close();

        // Parse the buffer
        return read(&buffer[0], buffer.size());
    }

    // Reads TGA image from a memory buffer
    bool read(const uint8_t* buffer, size_t size) {
        if (buffer == NULL || size < 18) {
            std::cerr << "Error: Invalid buffer or size too small." << std::endl;
            return false;
        }

        size_t offset = 0;

        // Read header
        id_length = buffer[offset++];
        color_map_type = buffer[offset++];
        image_type = buffer[offset++];

        color_map_origin = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        color_map_length = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        color_map_depth = buffer[offset++];

        x_origin = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        y_origin = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        width = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        height = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        pixel_depth = buffer[offset++];
        image_descriptor = buffer[offset++];

        // Validate header fields
        if (width == 0 || height == 0) {
            std::cerr << "Error: Image width or height is zero." << std::endl;
            return false;
        }
        if (pixel_depth != 8 && pixel_depth != 16 && pixel_depth != 24 && pixel_depth != 32) {
            std::cerr << "Error: Unsupported pixel depth: " << static_cast<int>(pixel_depth) << std::endl;
            return false;
        }

        // Read ID field
        if (id_length > 0) {
            if (offset + id_length > size) {
                std::cerr << "Error: ID field exceeds buffer size." << std::endl;
                return false;
            }
            id_field.resize(id_length);
            memcpy(&id_field[0], buffer + offset, id_length);
            offset += id_length;
        }

        // Read color map data
        if (color_map_type != 0 && color_map_length > 0) {
            size_t color_map_entry_size = color_map_depth / 8;
            size_t color_map_size = color_map_length * color_map_entry_size;
            if (offset + color_map_size > size) {
                std::cerr << "Error: Color map data exceeds buffer size." << std::endl;
                return false;
            }
            color_map_data.resize(color_map_size);
            memcpy(&color_map_data[0], buffer + offset, color_map_size);
            offset += color_map_size;
        }

        // Read image data
        size_t pixel_size = pixel_depth / 8;
        bool isRLE = (image_type & 0x08) != 0;

        if (isRLE) {
            size_t compressed_size = size - offset;
            if (!decodeRLEData(buffer + offset, compressed_size)) {
                std::cerr << "Error: Failed to decode RLE data." << std::endl;
                return false;
            }
        } else {
            size_t data_size = width * height * pixel_size;
            if (offset + data_size > size) {
                std::cerr << "Error: Image data exceeds buffer size." << std::endl;
                return false;
            }
            std::vector<uint8_t> raw_data(data_size);
            memcpy(&raw_data[0], buffer + offset, data_size);

            // For color-mapped images, expand indices to colors
            if (color_map_type != 0) {
                if (!expandColorMappedData(&raw_data[0], raw_data.size())) {
                    std::cerr << "Error: Failed to expand color-mapped data." << std::endl;
                    return false;
                }
            } else {
                image_data.swap(raw_data);
            }
        }

        return true;
    }

    // Writes TGA image to a memory buffer in 32-bit 8-8-8-8 format
    bool write(std::vector<uint8_t>& buffer) {
        // If current pixel_depth is not 32, convert image_data to 32-bit format
        if (pixel_depth != 32) {
            if (!convertTo32Bit()) {
                std::cerr << "Error: Failed to convert image data to 32-bit format." << std::endl;
                return false;
            }
        }

        // Prepare header fields for 32-bit uncompressed image
        id_length = 0; // No ID field
        color_map_type = 0; // No color map
        image_type = 2; // Uncompressed True-color image
        color_map_origin = 0;
        color_map_length = 0;
        color_map_depth = 0;
        pixel_depth = 32;
        image_descriptor = 0x28; // Top-left origin, 8 bits alpha channel

        // Start writing data into buffer
        buffer.clear();
        buffer.reserve(18 + image_data.size());

        // Write header
        buffer.push_back(id_length);
        buffer.push_back(color_map_type);
        buffer.push_back(image_type);

        buffer.push_back(color_map_origin & 0xFF);
        buffer.push_back((color_map_origin >> 8) & 0xFF);

        buffer.push_back(color_map_length & 0xFF);
        buffer.push_back((color_map_length >> 8) & 0xFF);

        buffer.push_back(color_map_depth);

        buffer.push_back(x_origin & 0xFF);
        buffer.push_back((x_origin >> 8) & 0xFF);

        buffer.push_back(y_origin & 0xFF);
        buffer.push_back((y_origin >> 8) & 0xFF);

        buffer.push_back(width & 0xFF);
        buffer.push_back((width >> 8) & 0xFF);

        buffer.push_back(height & 0xFF);
        buffer.push_back((height >> 8) & 0xFF);

        buffer.push_back(pixel_depth);
        buffer.push_back(image_descriptor);

        // No ID field
        // No color map data

        // Write image data
        buffer.insert(buffer.end(), image_data.begin(), image_data.end());

        return true;
    }

    // Saves TGA image to a file
    bool save(const char* filename) {
        std::vector<uint8_t> buffer;
        if (!write(buffer)) {
            std::cerr << "Error: Failed to write TGA data to memory buffer." << std::endl;
            return false;
        }

        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
            return false;
        }

        file.write(reinterpret_cast<const char*>(&buffer[0]), buffer.size());
        if (!file) {
            std::cerr << "Error: Failed to write data to file: " << filename << std::endl;
            return false;
        }

        file.close();
        return true;
    }

    /**
     * @brief Flips the image horizontally (mirrors along the X-axis).
     */
    void flipX() {
        if (pixel_depth != 32) {
            std::cerr << "flipX: Unsupported pixel depth. Expected 32-bit image." << std::endl;
            return;
        }

        size_t bytesPerPixel = 4;
        for (uint16_t y = 0; y < height; ++y) {
            for (uint16_t x = 0; x < width / 2; ++x) {
                size_t leftIndex = (y * width + x) * bytesPerPixel;
                size_t rightIndex = (y * width + (width - 1 - x)) * bytesPerPixel;

                // Swap the pixels
                for (size_t byte = 0; byte < bytesPerPixel; ++byte) {
                    std::swap(image_data[leftIndex + byte], image_data[rightIndex + byte]);
                }
            }
        }
    }

    /**
     * @brief Flips the image vertically (mirrors along the Y-axis).
     */
    void flipY() {
        if (pixel_depth != 32) {
            std::cerr << "flipY: Unsupported pixel depth. Expected 32-bit image." << std::endl;
            return;
        }

        size_t bytesPerPixel = 4;
        for (uint16_t y = 0; y < height / 2; ++y) {
            for (uint16_t x = 0; x < width; ++x) {
                size_t topIndex = (y * width + x) * bytesPerPixel;
                size_t bottomIndex = ((height - 1 - y) * width + x) * bytesPerPixel;

                // Swap the pixels
                for (size_t byte = 0; byte < bytesPerPixel; ++byte) {
                    std::swap(image_data[topIndex + byte], image_data[bottomIndex + byte]);
                }
            }
        }
    }

    /**
     * @brief Swaps the Red and Blue channels for each pixel (converts RGBA to BGRA or vice versa).
     */
    void swapRGBAtoBGRA() {
        if (pixel_depth != 32) {
            std::cerr << "swapRGBAtoBGRA: Unsupported pixel depth. Expected 32-bit image." << std::endl;
            return;
        }

        size_t bytesPerPixel = 4;
        size_t totalPixels = width * height;

        for (size_t i = 0; i < totalPixels; ++i) {
            size_t index = i * bytesPerPixel;
            // Swap Red (index + 2) and Blue (index + 0)
            std::swap(image_data[index + 0], image_data[index + 2]);
        }
    }

    /**
     * @brief Saves the TGA image as a PNG file.
     * @param filePath Path to save the PNG file.
     * @param flipV Whether to flip the image vertically.
     * @param reverse Whether to reverse the pixel order horizontally.
     * @return True on success, false on failure.
     */
    bool saveAsPNG2(const char* filePath, bool flipV = false, bool reverse = false) {
        pngFile_t png;
        return png.savePNG(filePath, &image_data[0], width, height, 4, flipV, reverse);
    }

    // CRC32 calculation
    uint32_t crc32(const unsigned char* data, size_t length) const {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j) {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc >>= 1;
            }
        }
        return ~crc;
    }

    // Helper functions for endian conversion
    uint32_t toBigEndian(uint32_t value) const {
        return htonl(value);
    }

    uint16_t toBigEndian(uint16_t value) const {
        return htons(value);
    }


    bool writeAsPNG(std::vector<uint8_t>& buffer, bool flipV = false, bool reverse = false) {
        // Calculate necessary sizes
        unsigned int bytesPerPixel = pixel_depth / 8;
        if (bytesPerPixel != 3 && bytesPerPixel != 4) {
            // Only support RGB and RGBA for simplicity
            return false;
        }
        unsigned int width_stride = width * bytesPerPixel;
        unsigned int image_size = width * height * bytesPerPixel;
        unsigned int data_size = image_size + height; // Including filter bytes

        // Flip vertically if required
        if (flipV) {
            for (unsigned int y = 0; y < height / 2; ++y) {
                unsigned int topIndex = y * width_stride;
                unsigned int bottomIndex = (height - 1 - y) * width_stride;
                for (unsigned int x = 0; x < width_stride; ++x) {
                    std::swap(image_data[topIndex + x], image_data[bottomIndex + x]);
                }
            }
        }

        // Initialize buffer for PNG data
        buffer.clear();

        // Write PNG Signature
        unsigned char png_signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
        buffer.insert(buffer.end(), png_signature, png_signature + 8);

        // Helper lambda to write a PNG chunk
        auto writeChunk = [&](const char* type, const std::vector<unsigned char>& data) -> bool {
            uint32_t length = toBigEndian(static_cast<uint32_t>(data.size()));
            buffer.insert(buffer.end(), reinterpret_cast<unsigned char*>(&length), reinterpret_cast<unsigned char*>(&length) + 4);
            buffer.insert(buffer.end(), type, type + 4);
            if (!data.empty()) {
                buffer.insert(buffer.end(), data.begin(), data.end());
            }
            // Calculate CRC over chunk type and data
            std::vector<unsigned char> crcData;
            crcData.reserve(4 + data.size());
            crcData.insert(crcData.end(), type, type + 4);
            crcData.insert(crcData.end(), data.begin(), data.end());
            uint32_t crc = toBigEndian(crc32(crcData.data(), crcData.size()));
            buffer.insert(buffer.end(), reinterpret_cast<unsigned char*>(&crc), reinterpret_cast<unsigned char*>(&crc) + 4);
            return true;
        };

        // Create IHDR data
        std::vector<unsigned char> ihdrData;
        ihdrData.reserve(13);
        uint32_t be_width = toBigEndian(static_cast<uint32_t>(width));
        uint32_t be_height = toBigEndian(static_cast<uint32_t>(height));
        ihdrData.insert(ihdrData.end(), reinterpret_cast<unsigned char*>(&be_width), reinterpret_cast<unsigned char*>(&be_width) + 4);
        ihdrData.insert(ihdrData.end(), reinterpret_cast<unsigned char*>(&be_height), reinterpret_cast<unsigned char*>(&be_height) + 4);
        ihdrData.push_back(8); // Bit depth
        ihdrData.push_back((bytesPerPixel == 3) ? 2 : 6); // Color type: 2=RGB, 6=RGBA
        ihdrData.push_back(0); // Compression method
        ihdrData.push_back(0); // Filter method
        ihdrData.push_back(0); // Interlace method

        // Write IHDR chunk
        if (!writeChunk("IHDR", ihdrData)) {
            return false;
        }

        // Create IDAT data (zlib stream with no compression)
        std::vector<unsigned char> idatData;

        // Zlib header for no compression: CMF=0x78, FLG=0x01
        idatData.push_back(0x78);
        idatData.push_back(0x01);

        // Initialize Adler-32 checksum variables
        const uint32_t MOD_ADLER = 65521;
        uint32_t a = 1;
        uint32_t b = 0;

        // Prepare image data with filter bytes
        std::vector<unsigned char> imageBytes;
        imageBytes.reserve(data_size);
        unsigned int p = 0; // Pointer for reverse reading if necessary

        for (unsigned int y = 0; y < height; ++y) {
            imageBytes.push_back(0x00); // No filter for each scanline
            // Update checksum for filter byte
            a = (a + 0x00) % MOD_ADLER;
            b = (b + a) % MOD_ADLER;

            for (unsigned int x = 0; x < width_stride; ++x) {
                unsigned char pixelByte = reverse ? image_data[--p] : image_data[p++];
                imageBytes.push_back(pixelByte);

                // Update Adler-32 checksum
                a = (a + pixelByte) % MOD_ADLER;
                b = (b + a) % MOD_ADLER;
            }
        }

        // Combine a and b to form the final Adler-32 checksum
        uint32_t adler32 = (b << 16) | a;

        // Split data into chunks of at most 65535 bytes
        size_t offset = 0;
        size_t remaining = imageBytes.size();
        while (remaining > 0) {
            // Determine block size (max 65535 bytes)
            size_t blockSize = std::min<size_t>(remaining, 65535);

            // BFINAL is 1 for the last block, 0 otherwise
            bool isFinalBlock = (remaining == blockSize);
            unsigned char bfinal = isFinalBlock ? 0x01 : 0x00;

            // Add deflate block header: BFINAL | BTYPE
            idatData.push_back(bfinal); // BTYPE = 00 (no compression)

            // Write LEN and NLEN in little endian
            uint16_t len = static_cast<uint16_t>(blockSize);
            uint16_t nlen = ~len;

            // Push len and nlen (little-endian)
            idatData.push_back(len & 0xFF);
            idatData.push_back((len >> 8) & 0xFF);
            idatData.push_back(nlen & 0xFF);
            idatData.push_back((nlen >> 8) & 0xFF);

            // Write uncompressed data block
            idatData.insert(idatData.end(), imageBytes.begin() + offset, imageBytes.begin() + offset + blockSize);

            // Update offset and remaining data size
            offset += blockSize;
            remaining -= blockSize;
        }

        // Write Adler-32 checksum at the end of the deflate stream (big endian)
        uint32_t be_adler = toBigEndian(adler32);
        idatData.insert(idatData.end(), reinterpret_cast<unsigned char*>(&be_adler), reinterpret_cast<unsigned char*>(&be_adler) + 4);

        // Write IDAT chunk
        if (!writeChunk("IDAT", idatData)) {
            return false;
        }

        // Write IEND chunk (no data)
        std::vector<unsigned char> iendData; // Empty
        if (!writeChunk("IEND", iendData)) {
            return false;
        }

        return true;
    }

    bool saveAsPNG(const std::string& filePath, bool flipV = false, bool reverse = false) {
        std::vector<uint8_t> buffer;

        // Generate the PNG buffer
        if (!writeAsPNG(buffer, flipV, reverse)) {
            return false;
        }

        // Open file for writing
        std::ofstream outFile(filePath, std::ios::binary);
        if (!outFile.is_open()) {
            return false;
        }

        // Write buffer to file
        outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        outFile.close();

        return true;
    }


    /**
     * @brief Writes the PNG image data to a memory buffer.
     * @param buffer Vector to store the PNG data.
     * @param flipV Whether to flip the image vertically.
     * @param reverse Whether to reverse the pixel order horizontally.
     * @return True on success, false on failure.
     */
    bool writeAsPNG2(std::vector<uint8_t>& buffer, bool flipV = false, bool reverse = false) {
        pngFile_t png;
        return png.writePNG(buffer, &image_data[0], width, height, 4, flipV, reverse);
    }

private:
    // Decodes RLE compressed data
    bool decodeRLEData(const uint8_t* data, size_t size) {
        size_t pixel_size = pixel_depth / 8;
        size_t total_pixels = width * height;
        size_t offset = 0;
        size_t pixels_read = 0;

        image_data.resize(total_pixels * pixel_size);

        while (pixels_read < total_pixels) {
            if (offset >= size) {
                std::cerr << "Error: RLE data ends prematurely." << std::endl;
                return false;
            }
            uint8_t packet_header = data[offset++];
            uint8_t packet_type = packet_header & 0x80;
            uint8_t pixel_count = (packet_header & 0x7F) + 1;

            if (packet_type) {
                // RLE packet
                if (offset + pixel_size > size) {
                    std::cerr << "Error: RLE packet data exceeds buffer size." << std::endl;
                    return false;
                }
                const uint8_t* pixel_data = data + offset;
                offset += pixel_size;

                for (uint8_t i = 0; i < pixel_count; ++i) {
                    if (pixels_read >= total_pixels)
                        break;
                    memcpy(&image_data[(pixels_read++) * pixel_size], pixel_data, pixel_size);
                }
            } else {
                // Raw packet
                size_t packet_data_size = pixel_count * pixel_size;
                if (offset + packet_data_size > size) {
                    std::cerr << "Error: Raw packet data exceeds buffer size." << std::endl;
                    return false;
                }
                memcpy(&image_data[pixels_read * pixel_size], data + offset, packet_data_size);
                offset += packet_data_size;
                pixels_read += pixel_count;
            }
        }

        // For color-mapped images, expand indices to colors
        if (color_map_type != 0) {
            std::vector<uint8_t> raw_indices = image_data;
            if (!expandColorMappedData(&raw_indices[0], raw_indices.size())) {
                std::cerr << "Error: Failed to expand color-mapped data from RLE." << std::endl;
                return false;
            }
        }

        return true;
    }

    // Expands color-mapped image data using the color map
    bool expandColorMappedData(const uint8_t* data, size_t size) {
        if (color_map_data.empty()) {
            std::cerr << "Error: Color map data is missing." << std::endl;
            return false;
        }

        size_t index_size = pixel_depth / 8;
        size_t color_map_entry_size = color_map_depth / 8;
        size_t total_pixels = width * height;

        if (size != total_pixels * index_size) {
            std::cerr << "Error: Data size does not match expected size for color-mapped image." << std::endl;
            return false;
        }

        image_data.resize(total_pixels * color_map_entry_size);

        for (size_t i = 0; i < total_pixels; ++i) {
            uint16_t index = 0;
            if (index_size == 1) {
                index = data[i];
            } else if (index_size == 2) {
                index = data[i * 2] | (data[i * 2 + 1] << 8);
            } else {
                std::cerr << "Error: Unsupported index size: " << index_size << std::endl;
                return false;
            }

            if (index < color_map_origin || index >= color_map_origin + color_map_length) {
                std::cerr << "Error: Color map index out of bounds: " << index << std::endl;
                return false;
            }

            size_t color_map_index = (index - color_map_origin) * color_map_entry_size;
            memcpy(&image_data[i * color_map_entry_size],
                   &color_map_data[color_map_index],
                   color_map_entry_size);
        }

        // Update pixel depth to match color map entry size
        pixel_depth = color_map_depth;

        return true;
    }

    // Converts image_data to 32-bit 8-8-8-8 format (BGRA)
    bool convertTo32Bit() {
        size_t total_pixels = width * height;
        size_t current_pixel_size = pixel_depth / 8;

        std::vector<uint8_t> new_image_data(total_pixels * 4); // 4 bytes per pixel

        for (size_t i = 0; i < total_pixels; ++i) {
            uint8_t r = 0, g = 0, b = 0, a = 255;
            size_t index = i * current_pixel_size;

            if (pixel_depth == 24) {
                // Assuming image_data is in BGR format
                b = image_data[index];
                g = image_data[index + 1];
                r = image_data[index + 2];
            } else if (pixel_depth == 16) {
                // Assuming 16-bit RGB555 format
                uint16_t pixel = image_data[index] | (image_data[index + 1] << 8);
                a = (pixel & 0x8000) ? 255 : 0;
                r = ((pixel & 0x7C00) >> 10) << 3;
                g = ((pixel & 0x03E0) >> 5) << 3;
                b = (pixel & 0x001F) << 3;
            } else if (pixel_depth == 8) {
                // Grayscale image
                b = g = r = image_data[index];
            } else if (pixel_depth == 32) {
                // Already in 32-bit format
                b = image_data[index];
                g = image_data[index + 1];
                r = image_data[index + 2];
                a = image_data[index + 3];
            } else {
                std::cerr << "Error: Unsupported pixel depth: " << static_cast<int>(pixel_depth) << std::endl;
                return false;
            }

            // Store in BGRA order
            new_image_data[i * 4]     = b;
            new_image_data[i * 4 + 1] = g;
            new_image_data[i * 4 + 2] = r;
            new_image_data[i * 4 + 3] = a;
        }

        image_data.swap(new_image_data);
        pixel_depth = 32;
        return true;
    }
};

struct texFile_t {

	/*
	// Struct provided by RedEye


	  Palette4 = 0x0,
	  Palette8 = 0x1,
	  RGB16 = 0x2,
	  ARGB16 = 0x2,
	  ARGB1555 = 0x2,
	  ARGB32 = 0x3,
	  ARGB8888 = 0x3,
	  ARGB4444 = 0x4,
	  RGB565 = 0x5,
	  Intensity8 = 0x6,
	  Bumpmap = 0x7,

	This is the actuall pixel formats
	Some of them duplicates
	Not sure what to think about them
	Mips are stored from biggest to smallest
	Palette is stored after mips
	if image has paletted then paletteOffset is non-zero
	Palette also starts with similar header, but smaller
	It's only 16 bytes
	Ident, version,conversionMode=0 and flags are used as conversionMode
	but pixelFormat is stored in flags
	and conversion mode is zero
	I was wrong a bit

	*/

    int32_t ident;
    int32_t version;
    int32_t image_type;
    int32_t overlayFlags;
    int32_t paletteOffset;
    int16_t scaleFactor;
    int16_t width;
    int16_t height;
    int16_t croppedWidth;
    int16_t croppedHeight;
    int16_t bytesPerPixel;
    std::vector<uint8_t> imageData;

    bool open(const std::string &filePath, std::ifstream &inputFile) {
        inputFile.open(filePath.c_str(), std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open file: " << filePath << std::endl;
            return false;
        }
        return true;
    }

    bool read(std::ifstream &inputFile, bool verbose = true) {
        // Read the header
        readValue(inputFile, ident);
        readValue(inputFile, version);
        readValue(inputFile, image_type);
        readValue(inputFile, overlayFlags);
        readValue(inputFile, paletteOffset);
        readValue(inputFile, scaleFactor);
        readValue(inputFile, width);
        readValue(inputFile, height);
        readValue(inputFile, croppedWidth);
        readValue(inputFile, croppedHeight);
        readValue(inputFile, bytesPerPixel);


        // Validate width and height
        const int MAX_TEXTURE_SIZE = 8192; // Define a reasonable maximum size
        if (width <= 0 || height <= 0 || width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE) {
            std::cerr << "Invalid texture dimensions: " << width << "x" << height << std::endl;
            return false;
        }

        // Print header values for debugging
        if (verbose) {std::cout << "Header: " << ident << " " << version << " " << width << "x" << height << " " << croppedWidth << "x" << croppedHeight << " " << image_type << std::endl;}

        // Check image type and parse accordingly
        switch (image_type) {
            case 67: // ARGB8888 format (32 bits per pixel)
                imageData.resize(width * height * 4);
                inputFile.read(reinterpret_cast<char*>(imageData.data()), width * height * 4);
                if (!inputFile) {
                    std::cerr << "Failed to read image data for ARGB8888 format." << std::endl;
                    return false;
                }
                break;

            case 2: // ARGB1555 format (16 bits per pixel)
                imageData.resize(width * height * 2);
                inputFile.read(reinterpret_cast<char*>(imageData.data()), width * height * 2);
                if (!inputFile) {
                    std::cerr << "Failed to read image data for ARGB1555 format." << std::endl;
                    return false;
                }
                convert1555To8888();
                break;

            default:
                std::cerr << "Unsupported image type: " << image_type << " encountered." << std::endl;
                std::cerr << "Please verify that the texture format is supported." << std::endl;
                return false;
        }

        return true;
    }

    void convert1555To8888() {
        std::vector<uint8_t> convertedData;
        convertedData.reserve(width * height * 4);

        for (size_t i = 0; i < imageData.size(); i += 2) {
            uint16_t pixel = imageData[i] | (imageData[i + 1] << 8);

            uint8_t a = (pixel & 0x8000) ? 255 : 0;
            uint8_t r = ((pixel & 0x7C00) >> 10) << 3;
            uint8_t g = ((pixel & 0x03E0) >> 5) << 3;
            uint8_t b = (pixel & 0x001F) << 3;

            convertedData.push_back(r);
            convertedData.push_back(g);
            convertedData.push_back(b);
            convertedData.push_back(a);
        }

        imageData = std::move(convertedData);
        bytesPerPixel = 4; // **Ensure bytesPerPixel is updated to 4**
    }

    bool toTga(tgaFile_t& tga) const {
        if (bytesPerPixel != 4) {
            std::cerr << "Error: Unsupported bytes per pixel for TGA conversion: " << bytesPerPixel << std::endl;
            return false;
        }

        tga.id_length = 0;
        tga.color_map_type = 0;
        tga.image_type = 2; // Uncompressed True-color image
        tga.color_map_origin = 0;
        tga.color_map_length = 0;
        tga.color_map_depth = 0;
        tga.x_origin = 0;
        tga.y_origin = 0;
        tga.width = width;
        tga.height = height;
        tga.pixel_depth = 32;
        tga.image_descriptor = 0x28; // Top-left origin, 8 bits alpha channel

        // No ID field
        tga.id_field.clear();

        // No color map
        tga.color_map_data.clear();

        // Image data: Keep in RGBA order
        tga.image_data = imageData; // Assuming imageData is already in RGBA order

        return true;
    }
    bool writeTGA(const std::string &filePath, bool flipX = false, bool flipY = false, bool swapRGBA = false) {
        std::ofstream outputFile(filePath.c_str(), std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to open output file: " << filePath << std::endl;
            return false;
        }

        // Use cropped width and height only when saving the TGA
        int actualWidth = (croppedWidth > 0 && croppedWidth < width) ? croppedWidth : width;
        int actualHeight = (croppedHeight > 0 && croppedHeight < height) ? croppedHeight : height;

        // Write TGA header (18 bytes)
        uint8_t tgaHeader[18] = { 0 };
        tgaHeader[2] = 2; // Uncompressed, true-color image
        tgaHeader[12] = actualWidth & 0xFF;
        tgaHeader[13] = (actualWidth >> 8) & 0xFF;
        tgaHeader[14] = actualHeight & 0xFF;
        tgaHeader[15] = (actualHeight >> 8) & 0xFF;
        tgaHeader[16] = 32; // Bits per pixel
        tgaHeader[17] = 0x20; // Image descriptor (top-left origin)
        outputFile.write(reinterpret_cast<const char*>(tgaHeader), sizeof(tgaHeader));

        // Write cropped image data with optional transformations
        for (int y = 0; y < actualHeight; ++y) {
            int row = flipY ? (actualHeight - 1 - y) : y;
            for (int x = 0; x < actualWidth; ++x) {
                int col = flipX ? (actualWidth - 1 - x) : x;
                size_t index = (row * width + col) * 4;

                uint8_t r = imageData[index + 0];
                uint8_t g = imageData[index + 1];
                uint8_t b = imageData[index + 2];
                uint8_t a = imageData[index + 3];

                if (swapRGBA) {
                    outputFile.put(b); // Blue
                    outputFile.put(g); // Green
                    outputFile.put(r); // Red
                    outputFile.put(a); // Alpha
                } else {
                    outputFile.put(r); // Red
                    outputFile.put(g); // Green
                    outputFile.put(b); // Blue
                    outputFile.put(a); // Alpha
                }
            }
        }

        outputFile.close();
        return true;
    }


    // Writes TEX image data to a TGA file
    bool writeTGA(const std::string& filePath) {
        tgaFile_t tga;
        if (!toTga(tga)) {
            std::cerr << "Error: Failed to convert TEX to TGA." << std::endl;
            return false;
        }

        // Save the TGA file
        return tga.save(filePath.c_str());
    }
};

struct resChunk_t {
    uint32_t chunk_type;
    uint32_t buffer_size;
    uint32_t unk1;
    uint32_t chunk_size;
    std::string filename;
    std::string filepath;
    tgaFile_t texture;
    mefFile_t model;

    resChunk_t() {}
    ~resChunk_t() {}

    // Reads a resource chunk from the input stream
    bool read(std::ifstream& f, std::string& current_name, std::string& current_path, bool verbose = false) {
        bool result = f.good();

        // Read chunk type, buffer size, unk1, chunk size
        readValue(f, chunk_type);
        readValue(f, buffer_size);
        readValue(f, unk1);
        readValue(f, chunk_size);

        // Read initial file position
        size_t buffer_pos = f.tellg();

        // Process based on chunk type

        switch (static_cast<MeshResourceType>(chunk_type)) {
        case MeshResourceType::NAME: { // 'NAME'
            if (verbose) { std::cout << "Chunk type: 'NAME'" << std::endl; }
            if (!readString(f, current_name)) {
                std::cerr << "Failed to read 'NAME' string." << std::endl;
                result = false;
            }
            if (verbose) { std::cout << "Read filename: " << current_name << std::endl; }
            break;
        }

        case MeshResourceType::PATH: { // 'PATH'
            if (verbose) { std::cout << "Chunk type: 'PATH'" << std::endl; }
            if (!readString(f, current_path)) {
                std::cerr << "Failed to read 'PATH' string." << std::endl;
                result = false;
            }
            if (verbose) { std::cout << "Read filepath: " << current_path << std::endl; }
            break;
        }

        case MeshResourceType::BODY: { // 'BODY'
            if (verbose) { std::cout << "Chunk type: 'BODY'" << std::endl; }
            filename = current_name;
            filepath = current_path;

            std::string file_type = getFilename::Type(filename);
            if (verbose) { std::cout << "Processing file type: " << file_type << std::endl; }

            if (matchPattern(file_type, ".tex")) {
                if (verbose) { std::cout << "File type matches '.tex'" << std::endl; }
                // Read TEX file data
                texFile_t texFile;
                if (!texFile.read(f)) {
                    std::cerr << "Failed to read TEX file: " << filename << std::endl;
                    result = false;
                } else {
                    if (verbose) { std::cout << "Successfully read TEX file: " << filename << std::endl; }

                    // Convert TEX to TGA and store in texture
                    if (!texFile.toTga(texture)) {
                        std::cerr << "Failed to convert TEX to TGA for file: " << filename << std::endl;
                        result = false;
                    } else {
                        if (verbose) { std::cout << "Successfully converted TEX to TGA for file: " << filename << std::endl; }
                        // Optional: Save as PNG for testing
                        // texture.saveAsPNG("test.png");
                    }
                }
            } else if (matchPattern(file_type, ".tga")) {
                if (verbose) { std::cout << "File type matches '.tga'" << std::endl; }
                // Read TGA file data
                std::vector<uint8_t> tgaData = readTGAData(f, chunk_size);
                if (verbose) { std::cout << "Read TGA data of size: " << tgaData.size() << std::endl; }

                if (tgaData.empty() || !texture.read(tgaData.data(), tgaData.size())) {
                    std::cerr << "Failed to read TGA file: " << filename << std::endl;
                    result = false;
                } else {
                    if (verbose) { std::cout << "Successfully read TGA file: " << filename << std::endl; }
                }
            } else if (matchPattern(file_type, ".mef")) {
                if (verbose) { std::cout << "File type matches '.mef'" << std::endl; }
                // Read MEF file data
                std::streampos current_pos = f.tellg();

                if (!model.readData(f)) {
                    std::cerr << "Failed to read MEF file: " << filename << std::endl;
                    result = false;
                } else {
                    if (verbose) { std::cout << "Successfully read MEF file: " << filename << std::endl; }
                    // The model data is stored in 'model'
                }
            } else {
                std::cerr << "Undocumented file type {" << filename << "} at position " << buffer_pos << std::endl;
            }

            // Reset current_name and current_path
            current_name.clear();
            current_path.clear();
            break;
        }

        default: {
            std::cerr << "Unexpected Type {" << intToFourCC(chunk_type) << "} at position " << buffer_pos << std::endl;
            // Skip the chunk data
            if (verbose) { std::cout << "Skipping chunk. New position after alignment: " << f.tellg() << std::endl; }
            result = false;
        }
        }
        size_t aligned_pos = buffer_pos + buffer_size + ((unk1 - (buffer_size % unk1)) % unk1);
        f.seekg(aligned_pos, std::ios::beg);

        if (verbose) { std::cout << "Finished processing chunk. Stream position: " << aligned_pos << std::endl; }

        return result;
    }

private:
    std::vector<uint8_t> readTGAData(std::ifstream& f, uint32_t size) {
        std::vector<uint8_t> buffer(size);
        if (!f.read(reinterpret_cast<char*>(&buffer[0]), size)) {
            std::cerr << "Error: Failed to read TGA data from stream." << std::endl;
            buffer.clear();
        }
        return buffer;
    }
};

struct resFile_t {
    uint32_t magic;
    uint32_t filesize;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t res_type;
    std::vector<resChunk_t> chunks;

    bool read(const std::string& filePath) {
        std::ifstream f(filePath.c_str(), std::ios::binary);
        if (!f.is_open()) {
            std::cerr << "Failed to open file: " << filePath << "\n";
            return false;
        }

        // Read magic (little endian)
        readValue(f, magic);
        if (magic != static_cast<uint32_t>(MeshResourceType::ILFF)) { // 'ILFF'
            std::cerr << "Unexpected magic value: " << std::hex << magic << "\n";
            f.close();
            return false;
        }

        // Read filesize (little endian)
        readValue(f, filesize);
        std::streampos end_pos = f.tellg() + static_cast<std::streamoff>(filesize) - 4;

        // Read unk1 and unk2 (little endian)
        readValue(f, unk1);
        readValue(f, unk2);

        // Read res_type (little endian)
        readValue(f, res_type);

        // Process based on res_type
        if (res_type == static_cast<uint32_t>(MeshResourceType::IRES)) { // 'IRES'
            std::string current_name, current_path;
            while (f.tellg() + 16 < end_pos && f.good()) {
                resChunk_t chunk;
                if (!chunk.read(f, current_name, current_path)) {
                    std::cerr << "Failed to read resChunk_t at position: " << f.tellg() << "\n";
                    return false; // Exiting the loop on read failure
                }
                // Emplace the object directly in the vector
                chunks.emplace_back(std::move(chunk));
            }
        } else {
            std::cout << "Unknown Asset In Container {" << res_type << "}" << std::endl;
        }

        f.close();
        return true;
    }
};




struct mtpInstanceTableEntry_t {
    uint32_t index;
    uint32_t count;
    std::vector<uint32_t> indices;

    bool read(std::ifstream &f) {
        // Read index (unsigned, little endian)
        readValue(f, index);

        // Read count (unsigned, little endian)
        readValue(f, count);

        // Read indices
        indices.clear();
        if (count > 0) {
            indices.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t idx;
                readValue(f, idx);
                indices.push_back(idx);
            }
        }
        return f.good();
    }
};

struct mtpInstanceTable_t {
    std::vector<mtpInstanceTableEntry_t> instances;

    bool read(std::ifstream &f, std::streampos endpos) {
        instances.clear();
        while (f.tellg() < endpos && f.good()) {
            mtpInstanceTableEntry_t entry;
            if (!entry.read(f)) {
                return false;
            }
            instances.push_back(entry);
        }
        return f.good();
    }
};

struct mtpIndexTableEntry_t {
    uint32_t index;
    int32_t flag;

    bool read(std::ifstream &f) {
        // Read index (unsigned, little endian)
        readValue(f, index);

        // Read flag (signed, little endian)
        readValue(f, flag);

        return f.good();
    }
};

struct mtpIndexTable_t {
    uint32_t count;
    std::vector<mtpIndexTableEntry_t> indices;

    bool read(std::ifstream &f) {
        // Read count (unsigned, little endian)
        readValue(f, count);

        // Read indices
        indices.clear();
        if (count > 0) {
            indices.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                mtpIndexTableEntry_t entry;
                if (!entry.read(f)) {
                    return false;
                }
                indices.push_back(entry);
            }
        }
        return f.good();
    }
};

struct mtpIntegerTable_t {
    uint32_t count;
    std::vector<uint32_t> values;
    std::vector<std::string> names;

    bool read(std::ifstream &f) {
        // Read count (unsigned, little endian)
        readValue(f, count);

        values.clear();
        names.clear();

        if (count > 0) {
            values.reserve(count);
            names.reserve(count);

            // Read values
            for (uint32_t i = 0; i < count; ++i) {
                uint32_t val;
                readValue(f, val);
                values.push_back(val);
            }

            // Read names
            for (uint32_t i = 0; i < count; ++i) {
                std::string name;
                if (!readString(f, name)) {
                    return false;
                }
                names.push_back(name);
            }
        }
        return f.good();
    }
};

struct mtpStringTable_t {
    uint32_t count;
    std::vector<std::string> names;

    bool read(std::ifstream &f) {
        // Read count (unsigned, little endian)
        readValue(f, count);

        names.clear();
        if (count > 0) {
            names.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                std::string name;
                if (!readString(f, name)) {
                    return false;
                }
                names.push_back(name);
            }
        }
        return f.good();
    }
};

struct mtpChunk_t {
    std::string type_debug; // Used for debugging
    uint32_t type;          // 'FORM' etc.
    uint32_t size;
    void* res;              // Can be specific type based on 'type'


    bool read(std::ifstream &f) {
        // Remember the current position to calculate end_pos
        std::streampos pos = f.tellg();

        // Read type (big endian)
        if (!readValueBE<uint32_t>(f, type)) {
            std::cerr << "Failed to read chunk type." << std::endl;
            return false;
        }

        // Read size (big endian)
        if (!readValueBE<uint32_t>(f, size)) {
            std::cerr << "Failed to read chunk size." << std::endl;
            return false;
        }

        // Convert type to string for debugging
        type_debug = intToFourCC(type, true);


        // calculate end position
        std::streampos end_pos = pos + static_cast<std::streamoff>(size) + 8; // Adjust as per padding

        // Process based on chunk type

        bool result = true;

        switch (type) {
            case FORM: {
                result = false; // Do not move file pointer for 'FORM' chunks
                break;
            }


            case BANM: { // 'BANM'
                mtpStringTable_t* table = new mtpStringTable_t();
                if (!table->read(f)) {
                    delete table;
                    return false;
                }
                res = static_cast<void*>(table);
                break;
            }

            case SNDS: { // 'SNDS'
                uint32_t value;
                readValue(f, value);
                res = new uint32_t(value);
                break;
            }

            case SVOL: { // 'SVOL'
                mtpStringTable_t* table = new mtpStringTable_t();
                if (!table->read(f)) {
                    delete table;
                    return false;
                }
                res = static_cast<void*>(table);
                break;
            }

            case MODS: { // 'MODS'
                mtpStringTable_t* table = new mtpStringTable_t();
                if (!table->read(f)) {
                    delete table;
                    return false;
                }
                res = static_cast<void*>(table);

                // Debugging output
                std::cout << "MODELS:\n";
                for (size_t i = 0; i < table->names.size(); ++i) {
                    std::cout << "\t" << (i) << ": \t\"" << table->names[i] << "\"\n";
                }
                break;
            }

            case VNAM: { // 'VNAM'
                mtpIntegerTable_t* intTable = new mtpIntegerTable_t();
                if (!intTable->read(f)) {
                    delete intTable;
                    return false;
                }
                res = static_cast<void*>(intTable);

                // Debugging output
                std::cout << "V NAME?:\n";
                for (size_t i = 0; i < intTable->names.size(); ++i) {
                    std::cout << "\t" << (i) << ": \t(" << intTable->values[i] << ") \"" << intTable->names[i] << "\"\n";
                }
                break;
            }

            case INST: { // 'INST'
                mtpInstanceTable_t* instTable = new mtpInstanceTable_t();
                instTable->read(f, end_pos);
                res = static_cast<void*>(instTable);

                // Debugging output
                std::cout << "INSTANCES:\n";
                size_t inst_pos = 0;
                for (size_t i = 0; i < instTable->instances.size(); ++i) {
                    std::cout << "\t" << i << ": \t" << instTable->instances[i].index << " ("
                              << "{";
                    for (size_t j = 0; j < instTable->instances[i].indices.size(); ++j) {
                        std::cout << instTable->instances[i].indices[j];
                        if (j != instTable->instances[i].indices.size() - 1) std::cout << ", ";
                    }
                    std::cout << "}) pos:" << inst_pos << "\n";
                    inst_pos += ((instTable->instances[i].indices.size() + 1) * 4);
                }
                break;
            }

            case TEXF: { // 'TEXF'
                mtpStringTable_t* table = new mtpStringTable_t();
                if (!table->read(f)) {
                    delete table;
                    return false;
                }
                res = static_cast<void*>(table);

                // Debugging output
                std::cout << "TEXTURES:\n";
                for (size_t i = 0; i < table->names.size(); ++i) {
                    std::cout << "\t" << (i) << ": \t\"" << table->names[i] << "\"\n";
                }
                break;
            }

            case PALF: { // 'PALF'
                uint32_t value;
                readValue(f, value);
                res = new uint32_t(value);
                break;
            }

            case GTT_: { // 'GTT '
                mtpIndexTable_t* indexTable = new mtpIndexTable_t();
                if (!indexTable->read(f)) {
                    delete indexTable;
                    return false;
                }
                res = static_cast<void*>(indexTable);

                // Debugging output
                std::cout << "GPU TEXTURES?:\n";
                for (size_t i = 0; i < indexTable->indices.size(); ++i) {
                    std::cout << "\t" << i << ": \t(" << indexTable->indices[i].flag
                              << ") " << indexTable->indices[i].index << "\n";
                }
                break;
            }

            default: { // Handle unexpected types
                std::string typeStr = intToFourCC(type, true);
                std::cout << "New Chunk Type {" << typeStr << "} at position " << f.tellg() << "\n";
                // Optionally, skip the unknown chunk
                f.seekg(pos + static_cast<std::streamoff>(size) + 8, std::ios::beg);
                res = nullptr;
                return false;
            }
        }
        if (result) {
            // Move to the end of the chunk (considering padding to 4-byte alignment)
            //std::streamoff padding = (4 - (size % 4)) % 4;
            //f.seekg(pos + static_cast<std::streamoff>(size) + padding, std::ios::beg);
            f.seekg(end_pos, std::ios::beg);
        }
        return f.good();
    }


};

struct mtpFile_t {
    mtpChunk_t data;
    uint32_t type;
    std::vector<mtpChunk_t> res;

    bool read(std::ifstream &f) {
        std::cout << "Starting MTP at " << f.tellg() << std::endl;

        if (!data.read(f)) {
            std::cerr << "Failed to read main mtpChunk_t.\n";
            return false;
        }

        // Check if the top-level chunk is 'FORM'
        if (data.type != static_cast<uint32_t>(MeshResourceType::FORM)) { // 'FORM'
            std::cout << "stopped at " << f.tellg() << std::endl;
            std::cerr << "Unexpected File Format\n";
            return false;
        }

        // Now, check the FormType

        uint32_t formType;
        std::cout << "Starting formType at " << f.tellg() << std::endl;
        if (!readValueBE<uint32_t>(f, formType)) {
            std::cerr << "Failed to read FormType within 'FORM' chunk." << std::endl;
            return false;
        }

        std::string formTypeStr = intToFourCC(formType, true);
        std::cout << "FormType: " << formTypeStr << std::endl;

        if (formType != static_cast<uint32_t>(MeshResourceType::MTP_)) { // 'MPT '
            std::cerr << "Unexpected FORM Type {" << intToFourCC(formType, true) << "}\n";
            return false;
        }

        // Read chunks until the end of the 'FORM' chunk
        std::streampos end_pos = f.tellg() + static_cast<std::streamoff>(data.size) - 4; // Subtract 4 bytes for FormType already read

        while (f.tellg() < end_pos && f.good()) {
            mtpChunk_t chunk;
            if (!chunk.read(f)) {
                std::cerr << "Failed to read a mtpChunk_t.\n";
                return false;
            }
            res.push_back(chunk);
            std::cout << "-----------------------------------------------------\n";
        }

        return f.good();
    }


};


























void assign_parent_indices(size_t bone_index,
    const std::vector<std::vector<size_t>>& bone_children,
    std::vector<int>& parent_indices) {
    for (size_t child_index : bone_children[bone_index]) {
        parent_indices[child_index] = static_cast<int>(bone_index);
        assign_parent_indices(child_index, bone_children, parent_indices);
    }
}

void swapBGRtoRGB(unsigned char* data, size_t size) {
    for (size_t i = 0; i < size; i += 4) { // Assuming 4 bytes per pixel (RGBA)
        std::swap(data[i], data[i + 2]); // Swap B and R
    }
}

class ImageBox : public Fl_Box {
public:
    ImageBox(int X, int Y, int W, int H) : Fl_Box(X, Y, W, H) {
        box(FL_DOWN_FRAME);
        align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    }

    void draw() override {
        // Clear the area before drawing the new image
        fl_color(FL_BACKGROUND_COLOR);
        fl_rectf(x(), y(), w(), h());

        // Call the parent draw function to handle image drawing
        Fl_Box::draw();
    }
};

class TextureViewerWindow : public Fl_Window {
public:
    TextureViewerWindow(int w, int h, const char* title, const char* filename = nullptr)
        : Fl_Window(w, h, title), currentImage(nullptr), zoomLevel(1.0f), autoZoom(true), alphaBlending(true) {
        begin();
        // Menu Bar
        menuBar = new Fl_Menu_Bar(0, 0, w, 25);
        menuBar->add("File/Open\tCtrl+O", FL_CTRL + 'o', menu_cb, (void*)this);
        menuBar->add("File/Export to TGA", 0, export_cb, (void*)this);
        menuBar->add("File/Exit\tEsc", FL_Escape, exit_cb, (void*)this);

        // Create the main group which holds the left panel and image preview
        mainGroup = new Fl_Group(0, 25, w, h - 50);

        // Create the left panel group (textureList and infoBox)
        leftPanel = new Fl_Group(0, 25, 200, h - 50);
        leftPanel->resizable(textureList);  // Make the textureList resizable within the group

        // Create the texture list
        textureList = new Fl_Select_Browser(0, 25, 200, h - 150);
        textureList->callback(list_cb, (void*)this);
        textureList->take_focus();

        // Set the resizable widget within leftPanel to textureList
        leftPanel->resizable(textureList);

        // Create the info box below the texture list
        infoBox = new Fl_Box(0, textureList->y() + textureList->h(), 200, 100);
        infoBox->box(FL_DOWN_FRAME);
        infoBox->labelfont(FL_COURIER);
        infoBox->labelsize(12);
        infoBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP);

        leftPanel->end();

        // Create the resizer (vertical)
        resizer = new Fl_Box(200, 25, 5, h - 50);
        resizer->box(FL_FLAT_BOX);
        resizer->color(fl_rgb_color(200, 200, 200));
        resizer->tooltip("Drag to resize the texture list");

        // Create the image box
        imageBox = new ImageBox(205, 25, w - 205, h - 50);
        imageBox->box(FL_DOWN_FRAME);

        // Set the resizable widget within mainGroup to imageBox
        mainGroup->resizable(imageBox);
        mainGroup->end();

        // Controls at the bottom
        autoZoomCheck = new Fl_Check_Button(0, h - 25, 100, 25, "Auto Zoom");
        autoZoomCheck->value(1);
        autoZoomCheck->callback(autozoom_cb, (void*)this);

        zoomSlider = new Fl_Slider(100, h - 25, w - 100, 25);
        zoomSlider->type(FL_HORIZONTAL);
        zoomSlider->minimum(0.1);
        zoomSlider->maximum(10.0);
        zoomSlider->step(0.1);
        zoomSlider->value(1.0);
        zoomSlider->callback(zoom_cb, (void*)this);

        end();

        // Set the resizable widget for the window to mainGroup
        resizable(mainGroup);

        if (filename) {
            parse_file(filename);
        }
    }

    ~TextureViewerWindow() {
        if (currentImage) {
            delete currentImage;
        }
    }

private:
    Fl_Menu_Bar* menuBar;
    Fl_Group* mainGroup;         // Added
    Fl_Group* leftPanel;         // Now a member variable
    Fl_Select_Browser* textureList;
    ImageBox* imageBox;
    Fl_Box* infoBox;
    Fl_Box* resizer;
    Fl_Image* currentImage;

    Fl_Check_Button* autoZoomCheck;
    Fl_Slider* zoomSlider;
    float zoomLevel;
    bool autoZoom;
    bool alphaBlending; // For alpha blending toggle

    struct TextureEntry {
        std::string name;
        tgaFile_t tga;
    };

    std::vector<TextureEntry> textures;

    // Callback functions
    static void menu_cb(Fl_Widget* w, void* data) {
        TextureViewerWindow* win = (TextureViewerWindow*)data;
        Fl_Menu_Bar* menu = (Fl_Menu_Bar*)w;
        const Fl_Menu_Item* m = menu->mvalue();
        if (strcmp(m->label(), "File/Open") == 0) {
            win->open_file();
        } else if (strcmp(m->label(), "File/Export to TGA") == 0) {
            win->export_tga();
        }
    }

    static void export_cb(Fl_Widget* w, void* data) {
        TextureViewerWindow* win = (TextureViewerWindow*)data;
        win->export_tga();
    }

    static void exit_cb(Fl_Widget* w, void* data) {
        exit(0);
    }

    static void list_cb(Fl_Widget* w, void* data) {
        TextureViewerWindow* win = static_cast<TextureViewerWindow*>(data);
        int index = win->textureList->value();
        if (index > 0 && index <= static_cast<int>(win->textures.size())) {
            win->display_texture(index - 1); // Fl_Select_Browser indices start from 1
        } else {
            std::cerr << "Selected index out of range: " << index << std::endl;
        }
    }

    static void autozoom_cb(Fl_Widget* w, void* data) {
        TextureViewerWindow* win = (TextureViewerWindow*)data;
        win->autoZoom = ((Fl_Check_Button*)w)->value();
        if (win->currentImage) {
            win->display_texture(win->textureList->value() - 1);
        }
    }

    static void zoom_cb(Fl_Widget* w, void* data) {
        TextureViewerWindow* win = (TextureViewerWindow*)data;
        win->zoomLevel = ((Fl_Slider*)w)->value();
        if (win->currentImage && !win->autoZoom) {
            win->display_texture(win->textureList->value() - 1);
        }
    }

    // Event handling for resizer and alpha toggle
    int handle(int event) override {
        static int dragging = 0;
        static int last_x = 0;

        switch (event) {
            case FL_PUSH:
                if (Fl::event_inside(resizer)) {
                    dragging = 1;
                    last_x = Fl::event_x();
                    Fl::grab(this); // Grab the window during dragging
                    return 1;
                }
                break;
            case FL_DRAG:
                if (dragging) {
                    int dx = Fl::event_x() - last_x;
                    last_x = Fl::event_x();
                    int new_list_width = textureList->w() + dx;
                    int min_list_width = 100;
                    int max_list_width = w() - 300; // Ensure there's space for image and controls
                    if (new_list_width < min_list_width) new_list_width = min_list_width;
                    if (new_list_width > max_list_width) new_list_width = max_list_width;

                    // Resize the textureList and infoBox
                    textureList->resize(textureList->x(), textureList->y(), new_list_width, textureList->h());
                    infoBox->resize(infoBox->x(), textureList->y() + textureList->h(), new_list_width, infoBox->h());

                    // Ensure the group containing textureList is resized as well
                    leftPanel->size(new_list_width, leftPanel->h());

                    // Move the resizer
                    resizer->position(textureList->x() + new_list_width, resizer->y());

                    // Resize and move the imageBox
                    imageBox->position(resizer->x() + resizer->w(), imageBox->y());
                    imageBox->size(w() - (resizer->x() + resizer->w()), imageBox->h());

                    // Adjust zoomed image if necessary
                    if (autoZoom && currentImage) {
                        display_texture(textureList->value() - 1);
                    }

                    redraw();  // Redraw the entire window to refresh the UI
                    return 1;
                }
                break;


            case FL_RELEASE:
                if (dragging) {
                    dragging = 0;
                    Fl::grab(0); // Release the grab
                    return 1;
                }
                break;
            case FL_SHORTCUT:
                if (Fl::event_key() == 'a' || Fl::event_key() == 'A') {
                    alphaBlending = !alphaBlending;
                    display_texture(textureList->value() - 1);
                    return 1;
                }
                break;
            case FL_KEYDOWN:
                if (Fl::event_key() == FL_Escape) {
                    exit(0);
                    return 1;
                } else if (Fl::event_key() == FL_Up || Fl::event_key() == FL_Down) {
                    int index = textureList->value();
                    if (Fl::event_key() == FL_Up) {
                        if (index > 1) {
                            textureList->select(index - 1);
                            display_texture(index - 2);
                        }
                    } else if (Fl::event_key() == FL_Down) {
                        if (index < textureList->size()) {
                            textureList->select(index + 1);
                            display_texture(index);
                        }
                    }
                    return 1;
                } else if (Fl::event_key() == (FL_CTRL + 'o')) {
                    open_file();
                    return 1;
                }
                break;
            default:
                break;
        }
        return Fl_Window::handle(event);
    }

    // Function to open a file using Fl_Native_File_Chooser
    void open_file() {
        Fl_Native_File_Chooser chooser;
        chooser.title("Open File");
        chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
        chooser.filter("RES Files\t*.res\nAll Files\t*.*");
        if (chooser.show() == 0) {
            std::string filename = chooser.filename();
            parse_file(filename);
        }
    }

    // Function to parse the resource file and populate the texture list
    void parse_file(const std::string& filename) {
        textures.clear();
        textureList->clear();

        resFile_t resFile;
        if (!resFile.read(filename)) {
            fl_alert("Failed to parse RES file: %s", filename.c_str());
            return;
        }

        for (size_t i = 0; i < resFile.chunks.size(); ++i) {
            const resChunk_t& chunk = resFile.chunks[i];
            if (chunk.chunk_type != 0x59444F42) { // 'BODY'
                continue; // Only process 'BODY' chunks
            }

            if (chunk.texture.image_data.empty()) {
                continue; // Skip if texture data is empty
            }

            TextureEntry entry;
            entry.name = getFilename::File(chunk.filename.empty() ? "Texture " + std::to_string(i + 1) : chunk.filename);
            entry.tga = chunk.texture;
            textures.push_back(entry);
        }

        if (textures.empty()) {
            fl_alert("No valid textures found in the RES file.");
            return;
        }

        for (size_t i = 0; i < textures.size(); ++i) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%zu: %s", i + 1, textures[i].name.c_str());
            textureList->add(buffer);
        }

        if (!textures.empty()) {
            textureList->select(1);
            display_texture(0);
        }
    }

    // Function to display the selected texture with metadata and alpha blending
    void display_texture(int index) {
        if (index < 0 || index >= static_cast<int>(textures.size())) return;

        if (currentImage) {
            delete currentImage;
            currentImage = nullptr;
        }

        // Clear the previous image from the imageBox
        imageBox->image(nullptr);
        imageBox->redraw();

        TextureEntry& entry = textures[index];

        int width = entry.tga.width;
        int height = entry.tga.height;
        int depth = 4; // Assuming RGBA data

        // Ensure that image_data size matches width * height * depth
        if (entry.tga.image_data.size() != static_cast<size_t>(width * height * depth)) {
            fl_alert("Texture data size mismatch for texture: %s", entry.name.c_str());
            return;
        }

        // Create FLTK image from texture data
        Fl_RGB_Image* textureImage = new Fl_RGB_Image(entry.tga.image_data.data(), width, height, depth);
        textureImage->alloc_array = 0; // FLTK does not manage the data

        // Create checkerboard background
        Fl_RGB_Image* bgImage = create_checkerboard(width, height);

        // Composite the texture onto the background using alpha blending
        unsigned char* composite_data = new unsigned char[width * height * 3];
        unsigned char* tex_data = (unsigned char*)textureImage->data()[0];
        unsigned char* bg_data = (unsigned char*)bgImage->data()[0];

        for (int i = 0; i < width * height; ++i) {
            unsigned char alpha = tex_data[i * 4 + 3];
            float alpha_f = alpha / 255.0f;
            composite_data[i * 3 + 0] = static_cast<unsigned char>(tex_data[i * 4 + 0] * alpha_f + bg_data[i * 3 + 0] * (1 - alpha_f)); // B
            composite_data[i * 3 + 1] = static_cast<unsigned char>(tex_data[i * 4 + 1] * alpha_f + bg_data[i * 3 + 1] * (1 - alpha_f)); // G
            composite_data[i * 3 + 2] = static_cast<unsigned char>(tex_data[i * 4 + 2] * alpha_f + bg_data[i * 3 + 2] * (1 - alpha_f)); // R
        }

        Fl_RGB_Image* compositeImage = new Fl_RGB_Image(composite_data, width, height, 3);
        compositeImage->alloc_array = 1; // FLTK will manage the data

        // Clean up
        delete textureImage;
        delete bgImage;

        currentImage = compositeImage;

        // Scale the image to fit the imageBox
        int box_w = imageBox->w();
        int box_h = imageBox->h();

        Fl_Image* scaledImage = nullptr;

        if (autoZoom) {
            float scale_w = static_cast<float>(box_w) / width;
            float scale_h = static_cast<float>(box_h) / height;
            float scale = std::min(scale_w, scale_h);

            int new_w = static_cast<int>(width * scale);
            int new_h = static_cast<int>(height * scale);

            scaledImage = currentImage->copy(new_w, new_h);
        } else {
            int new_w = static_cast<int>(width * zoomLevel);
            int new_h = static_cast<int>(height * zoomLevel);

            scaledImage = currentImage->copy(new_w, new_h);
        }

        delete currentImage;
        currentImage = scaledImage;
        imageBox->image(currentImage);
        imageBox->redraw(); // Make sure to redraw the imageBox to display the new image

        // Set the infoBox label with the metadata
        std::string info = "Name: " + entry.name + "\nWidth: " + std::to_string(width) + "\nHeight: " + std::to_string(height) + "\nPixel Depth: " + std::to_string(entry.tga.pixel_depth) + " bits\nImage Type: " + (entry.tga.image_type == 2 ? "Uncompressed True-Color" : "Other");
        infoBox->copy_label(info.c_str());

        // Force the whole window to redraw
        this->redraw();
    }

    // Function to export the selected texture to a TGA file
    void export_tga() {
        if (currentImage) {
            int index = textureList->value() - 1;
            if (index < 0 || index >= static_cast<int>(textures.size())) {
                fl_alert("No texture selected to export.");
                return;
            }

            Fl_Native_File_Chooser chooser;
            chooser.title("Save As");
            chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
            chooser.filter("TGA Files\t*.tga\nAll Files\t*.*");

            // Set default filename
            std::string default_filename = textures[index].name;
            // Replace any invalid filename characters if necessary
            // For simplicity, let's remove any colons or slashes
            std::replace(default_filename.begin(), default_filename.end(), ':', '_');
            std::replace(default_filename.begin(), default_filename.end(), '/', '_');
            std::replace(default_filename.begin(), default_filename.end(), '\\', '_');

            // Append .tga if not present
            if (default_filename.find('.') == std::string::npos) {
                default_filename += ".tga";
            }

            chooser.preset_file(default_filename.c_str());

            if (chooser.show() == 0) {
                std::string filename = chooser.filename();
                // Ensure the filename ends with .tga
                size_t dot = filename.find_last_of('.');
                if (dot == std::string::npos || filename.substr(dot) != ".tga") {
                    filename += ".tga"; // Append .tga if no extension or wrong extension
                }

                if (textures[index].tga.save(filename.c_str())) {
                    fl_message("Saved texture as %s", filename.c_str());
                } else {
                    fl_alert("Failed to save texture as %s", filename.c_str());
                }
            }
        } else {
            fl_alert("No texture selected to export.");
        }
    }

    // Function to create a checkerboard background for simulating transparency
    Fl_RGB_Image* create_checkerboard(int w, int h, int block_size = 10) {
        unsigned char* data = new unsigned char[w * h * 3];
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                bool even = ((x / block_size) % 2 == (y / block_size) % 2);
                unsigned char color = even ? 200 : 255;
                int idx = (y * w + x) * 3;
                data[idx] = data[idx + 1] = data[idx + 2] = color;
            }
        }
        Fl_RGB_Image* bg = new Fl_RGB_Image(data, w, h, 3);
        bg->alloc_array = 1; // FLTK will manage the data
        return bg;
    }


    // Function to handle window resizing
    void resize(int X, int Y, int W, int H) override {
        Fl_Window::resize(X, Y, W, H);  // Call parent class resize

        // Adjust positions and sizes of widgets
        int menuBarHeight = menuBar->h();
        int controlHeight = autoZoomCheck->h();
        int infoBoxHeight = 100; // Fixed height for the infoBox

        int mainGroupY = menuBarHeight;
        int mainGroupHeight = H - menuBarHeight - controlHeight;

        // Resize mainGroup
        mainGroup->resize(0, mainGroupY, W, mainGroupHeight);

        // Adjust leftPanel
        leftPanel->resize(leftPanel->x(), mainGroupY, leftPanel->w(), mainGroupHeight);

        // Calculate available height for the textureList
        int textureListHeight = mainGroupHeight - infoBoxHeight;

        // Resize the textureList to fill the available space
        textureList->resize(textureList->x(), textureList->y(), leftPanel->w(), textureListHeight);

        // Adjust the infoBox position below the textureList
        infoBox->resize(infoBox->x(), textureList->y() + textureList->h(), leftPanel->w(), infoBoxHeight);

        // Adjust the resizer position and size
        resizer->resize(leftPanel->x() + leftPanel->w(), mainGroupY, resizer->w(), mainGroupHeight);

        // Adjust the imageBox to take up the remaining space
        imageBox->resize(resizer->x() + resizer->w(), mainGroupY, W - (resizer->x() + resizer->w()), mainGroupHeight);

        // Adjust positions and sizes of the controls at the bottom
        autoZoomCheck->position(0, H - controlHeight);
        zoomSlider->position(100, H - controlHeight);
        zoomSlider->size(W - 100, controlHeight);

        // If auto-zoom is enabled and there's an image displayed, re-display the texture
        if (autoZoom && currentImage) {
            display_texture(textureList->value() - 1);  // Adjust the image display on resize
        }
    }



};




bool loadMeshFromMEF(const mefFile_t& mefFile, MyGlWindow* glWindow) {
    // Clear existing meshes
    glWindow->meshes.clear();

    // Extract mesh data directly from mefFile
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> tverts; // Texture coordinates
    std::vector<glm::ivec3> faces;
    std::vector<int> materialIDs;  // Material IDs for each face
    std::vector<Materialm> materials;  // Materials

    float mscale = 0.0003934f; // Scaling factor if needed

    // Retrieve necessary chunks
    const mefMeshChunk_t* hier_chunk = mefFile.get_content("HIER");
    const mefMeshChunk_t* bnam_chunk = mefFile.get_content("BNAM");
    const mefMeshChunk_t* vrtx_chunk = mefFile.get_content("VRTX");
    const mefMeshChunk_t* face_chunk = mefFile.get_content("FACE");
    const mefMeshChunk_t* rend_chunk = mefFile.get_content("REND");

    // Check for required chunks
    if (!vrtx_chunk || !face_chunk || !rend_chunk) {
        std::cerr << "Required chunks are missing." << std::endl;
        return false;
    }

    // Build bone hierarchy if available
    std::vector<std::string> bone_names;
    std::vector<std::array<float, 3>> bone_positions;
    std::vector<int> parent_indices;

    if (hier_chunk && bnam_chunk) {
        mefMeshHier_t* hier = dynamic_cast<mefMeshHier_t*>(hier_chunk->res);
        mefMeshBNam_t* bnam = dynamic_cast<mefMeshBNam_t*>(bnam_chunk->res);

        if (hier && bnam && hier->num_children.size() == bnam->names.size()) {
            size_t num_bones = hier->num_children.size();
            bone_names = bnam->names;
            bone_positions.resize(num_bones);
            parent_indices.resize(num_bones, -1);

            // Build parent indices based on hierarchy
            std::vector<std::vector<size_t>> bone_children(num_bones);
            size_t currentIndex = 1;
            for (size_t i = 0; i < num_bones; ++i) {
                uint8_t numChildren = hier->num_children[i];
                for (uint8_t j = 0; j < numChildren; ++j) {
                    if (currentIndex < num_bones) {
                        bone_children[i].push_back(currentIndex);
                        ++currentIndex;
                    }
                    else {
                        std::cerr << "Warning: currentIndex (" << currentIndex
                            << ") exceeds number of bones (" << num_bones << ")." << std::endl;
                        break;
                    }
                }
            }

            // Recursively assign parent indices using a helper function
            assign_parent_indices(0, bone_children, parent_indices);

            // Compute bone positions
            for (size_t i = 0; i < num_bones; ++i) {
                std::array<float, 3> head_pos = {
                    hier->position[i][0] * mscale,
                    hier->position[i][1] * mscale,
                    hier->position[i][2] * mscale
                };
                if (parent_indices[i] >= 0) {
                    const auto& parent_pos = bone_positions[parent_indices[i]];
                    bone_positions[i][0] = head_pos[0] + parent_pos[0];
                    bone_positions[i][1] = head_pos[1] + parent_pos[1];
                    bone_positions[i][2] = head_pos[2] + parent_pos[2];
                }
                else {
                    bone_positions[i] = head_pos;
                }
            }
        }
        else {
            std::cerr << "Invalid HIER or BNAM data." << std::endl;
        }
    }
    else {
        std::cout << "Bone hierarchy not available. Using mesh without bone transformations." << std::endl;
    }

    // Cast to appropriate types
    const mefMeshVrtx_t* vrtx = dynamic_cast<const mefMeshVrtx_t*>(vrtx_chunk->res);
    const mefMeshFace_t* face = dynamic_cast<const mefMeshFace_t*>(face_chunk->res);
    const mefMeshRend_t* rend = dynamic_cast<const mefMeshRend_t*>(rend_chunk->res);


    if (!vrtx || !face || !rend) {
        std::cerr << "Failed to cast resource chunks." << std::endl;
        return false;
    }

    // Process each sub-mesh in REND
    const auto& submeshes = rend->entry;
    size_t globalVertexOffset = 0; // To keep track of vertex indices across sub-meshes
    int currentMaterialID = 0;     // To assign unique material IDs

    for (size_t smeshIndex = 0; smeshIndex < submeshes.size(); ++smeshIndex) {
        const auto& smesh = submeshes[smeshIndex];

        size_t facePos = smesh.face_pos / 3; // Assuming face_pos is index into face entries
        size_t faceCount = smesh.face_count;
        size_t vertPos = smesh.vertex_pos;
        size_t vertCount = smesh.vertex_count;

        // **Assign a unique material ID for this sub-mesh**
        int materialID = currentMaterialID++;
        Materialm newMaterial;
        newMaterial.applyRandomColors(); // Assign random colors since we may not have textures

        materials.push_back(newMaterial);

        // **Process vertices for this sub-mesh**
        for (size_t i = vertPos; i < vertPos + vertCount; ++i) {
            if (i >= vrtx->entry.size()) {
                std::cerr << "Vertex index out of range: " << i << std::endl;
                return false;
            }
            const auto& vertex_entry = vrtx->entry[i];

            // Apply bone transformation if available
            glm::vec3 node_pos(0.0f, 0.0f, 0.0f);
            if (!bone_positions.empty() && vertex_entry.bone_index >= 0 &&
                static_cast<size_t>(vertex_entry.bone_index) < bone_positions.size()) {
                const auto& bone_pos = bone_positions[vertex_entry.bone_index];
                node_pos = glm::vec3(-bone_pos[0], bone_pos[1], bone_pos[2]);
            }

            // Compute world position
            float x = vertex_entry.position[0] * -mscale + node_pos.x;
            float y = vertex_entry.position[1] * mscale + node_pos.y;
            float z = vertex_entry.position[2] * mscale + node_pos.z;

            // Swap y and z components if needed (this depends on your coordinate system)
            vertices.emplace_back(x, z, y);

            // Normals
            normals.emplace_back(vertex_entry.normal[0], vertex_entry.normal[2], vertex_entry.normal[1]);

            // Texture coordinates (set to zero if not used)
            tverts.emplace_back(vertex_entry.texcoord0[0], vertex_entry.texcoord0[1]);
        }

        // **Process faces for this sub-mesh**
        for (size_t i = facePos; i < facePos + faceCount; ++i) {
            if (i >= face->entry.size()) {
                std::cerr << "Face index out of range: " << i << std::endl;
                return false;
            }
            const auto& face_indices = face->entry[i];

            // Adjust indices to be zero-based and consider global vertex offset
            faces.emplace_back(
                face_indices[0] - vertPos + globalVertexOffset,
                face_indices[2] - vertPos + globalVertexOffset,
                face_indices[1] - vertPos + globalVertexOffset
            );

            // **Assign the material ID to this face**
            materialIDs.push_back(materialID);
        }

        // Update global vertex offset
        globalVertexOffset += vertCount;
    }

    // Make the OpenGL context current before adding the mesh
	glWindow->make_current();

	// Add the mesh to the viewer
	glWindow->addMesh(vertices, faces, materialIDs, tverts, materials, normals);

    //Mesh newMesh(vertices, faces, materialIDs, tverts, materials, normals);
    //glWindow->meshes.push_back(newMesh);

	// Set up the meshes (create VAOs, VBOs, etc.)
	glWindow->setupMeshes();

	// Optionally, zoom to extents
	glWindow->zoomExtents();

    return true;
}

bool loadMeshFromMEFWithMaterial( const mefFile_t& mefFile, MyGlWindow* glWindow, const std::vector<std::string>& textureNames, const std::unordered_map<int, std::vector<int>>& modelToTextureIndices, const std::unordered_map<std::string, tgaFile_t>& textureMap, int modelIndex ) {
    // Clear existing meshes
    glWindow->clearMeshes();

    // Extract mesh data directly from mefFile
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> tverts; // Texture coordinates
    std::vector<glm::ivec3> faces;
    std::vector<int> materialIDs;  // Material IDs for each face
    std::vector<Materialm> materials;  // Materials

    float mscale = 0.0003934f; // Scaling factor if needed

    // Retrieve necessary chunks
    const mefMeshChunk_t* hier_chunk = mefFile.get_content("HIER");
    const mefMeshChunk_t* bnam_chunk = mefFile.get_content("BNAM");
    const mefMeshChunk_t* vrtx_chunk = mefFile.get_content("VRTX");
    const mefMeshChunk_t* face_chunk = mefFile.get_content("FACE");
    const mefMeshChunk_t* rend_chunk = mefFile.get_content("REND");

    // Check for required chunks
    if (!vrtx_chunk || !face_chunk || !rend_chunk) {
        std::cerr << "Required chunks are missing." << std::endl;
        return false;
    }

    // Build bone hierarchy if available
    std::vector<std::string> bone_names;
    std::vector<std::array<float, 3>> bone_positions;
    std::vector<int> parent_indices;

    if (hier_chunk && bnam_chunk) {
        mefMeshHier_t* hier = dynamic_cast<mefMeshHier_t*>(hier_chunk->res);
        mefMeshBNam_t* bnam = dynamic_cast<mefMeshBNam_t*>(bnam_chunk->res);

        if (hier && bnam && hier->num_children.size() == bnam->names.size()) {
            size_t num_bones = hier->num_children.size();
            bone_names = bnam->names;
            bone_positions.resize(num_bones);
            parent_indices.resize(num_bones, -1);

            // Build parent indices based on hierarchy
            std::vector<std::vector<size_t>> bone_children(num_bones);
            size_t currentIndex = 1;
            for (size_t i = 0; i < num_bones; ++i) {
                uint8_t numChildren = hier->num_children[i];
                for (uint8_t j = 0; j < numChildren; ++j) {
                    if (currentIndex < num_bones) {
                        bone_children[i].push_back(currentIndex);
                        parent_indices[currentIndex] = static_cast<int>(i); // Assign parent
                        ++currentIndex;
                    }
                    else {
                        std::cerr << "Warning: currentIndex (" << currentIndex
                            << ") exceeds number of bones (" << num_bones << ")." << std::endl;
                        break;
                    }
                }
            }

            // Compute bone positions
            for (size_t i = 0; i < num_bones; ++i) {
                std::array<float, 3> head_pos = {
                    hier->position[i][0] * mscale,
                    hier->position[i][1] * mscale,
                    hier->position[i][2] * mscale
                };
                if (parent_indices[i] >= 0) {
                    const auto& parent_pos = bone_positions[parent_indices[i]];
                    bone_positions[i][0] = head_pos[0] + parent_pos[0];
                    bone_positions[i][1] = head_pos[1] + parent_pos[1];
                    bone_positions[i][2] = head_pos[2] + parent_pos[2];
                }
                else {
                    bone_positions[i] = { head_pos[0], head_pos[1], head_pos[2] };
                }
            }
        }
        else {
            std::cerr << "Invalid HIER or BNAM data." << std::endl;
        }
    }
    else {
        std::cout << "Bone hierarchy not available. Using mesh without bone transformations." << std::endl;
    }

    // Cast to appropriate types
    const mefMeshVrtx_t* vrtx = dynamic_cast<const mefMeshVrtx_t*>(vrtx_chunk->res);
    const mefMeshFace_t* face = dynamic_cast<const mefMeshFace_t*>(face_chunk->res);
    const mefMeshRend_t* rend = dynamic_cast<const mefMeshRend_t*>(rend_chunk->res);

    if (!vrtx || !face || !rend) {
        std::cerr << "Failed to cast resource chunks." << std::endl;
        return false;
    }

    // Retrieve texture indices for the current model
    auto texIndicesIter = modelToTextureIndices.find(modelIndex);
    std::vector<int> texIndices;
    if (texIndicesIter != modelToTextureIndices.end()) {
        texIndices = texIndicesIter->second;
    }
    else {
        std::cerr << "No texture indices found for model index: " << modelIndex << std::endl;
        // Optionally, continue without textures
    }

    // Load textures and create materials
    // Assuming one material per texture
    // Alternatively, if multiple textures per material, adjust accordingly
    // For simplicity, assign one texture per material
    for (int texIndex : texIndices) {
        if (texIndex >= 0 && texIndex < static_cast<int>(textureNames.size())) {
            std::string texName = textureNames[texIndex];
            auto texIter = textureMap.find(texName);
            if (texIter != textureMap.end()) {
                Materialm material;
                // Convert TGA data to OpenGL texture
                const tgaFile_t& tga = texIter->second;

                material.loadTextureFromMemory(
                    tga.image_data.data(),
                    tga.width,
                    tga.height,
                    4 // channels
                );
                materials.push_back(material);
            }
            else {
                std::cerr << "Texture not found in textureMap: " << texName << std::endl;
                // Optionally, use a default material
                Materialm defaultMaterial;
                defaultMaterial.applyRandomColors(); // Assign random colors if texture is missing
                materials.push_back(defaultMaterial);
            }
        }
        else {
            std::cerr << "Texture index out of range: " << texIndex << std::endl;
            // Optionally, use a default material
            Materialm defaultMaterial;
            defaultMaterial.applyRandomColors(); // Assign random colors if texture index is invalid
            materials.push_back(defaultMaterial);
        }
    }

    // If no textures were found or assigned, create a default material
    if (materials.empty()) {
        Materialm defaultMaterial;
        defaultMaterial.applyRandomColors(); // Assign random colors
        materials.push_back(defaultMaterial);
    }

    // Process each sub-mesh in REND
    const auto& submeshes = rend->entry;
    size_t globalVertexOffset = 0; // To keep track of vertex indices across sub-meshes

    for (size_t smeshIndex = 0; smeshIndex < submeshes.size(); ++smeshIndex) {
        const auto& smesh = submeshes[smeshIndex];

        size_t facePos = smesh.face_pos / 3; // Assuming face_pos is index into face entries
        size_t faceCount = smesh.face_count;
        size_t vertPos = smesh.vertex_pos;
        size_t vertCount = smesh.vertex_count;

        // **Assign a material based on the texture indices**
        // Assign a material ID per submesh, based on the texture indices
        int materialID = 0; // Default material
        if (smeshIndex < static_cast<size_t>(materials.size())) {
            materialID = smeshIndex; // Each submesh has its own material
        }
        else if (!materials.empty()) {
            materialID = static_cast<int>(materials.size() - 1); // Use the last material
        }
        else {
            // Should not reach here, but ensure materialID is valid
            materialID = 0;
        }

        // **Process vertices for this sub-mesh**
        for (size_t i = vertPos; i < vertPos + vertCount; ++i) {
            if (i >= vrtx->entry.size()) {
                std::cerr << "Vertex index out of range: " << i << std::endl;


                glWindow->clearMeshes();
                return false;
            }
            const auto& vertex_entry = vrtx->entry[i];

            // Apply bone transformation if available
            glm::vec3 node_pos(0.0f, 0.0f, 0.0f);
            if (!bone_positions.empty() && vertex_entry.bone_index >= 0 &&
                static_cast<size_t>(vertex_entry.bone_index) < bone_positions.size()) {
                const auto& bone_pos = bone_positions[vertex_entry.bone_index];
                node_pos = glm::vec3(-bone_pos[0], bone_pos[1], bone_pos[2]);
            }

            // Compute world position
            float x = vertex_entry.position[0] * -mscale + node_pos.x;
            float y = vertex_entry.position[1] * mscale + node_pos.y;
            float z = vertex_entry.position[2] * mscale + node_pos.z;

            // Swap y and z components if needed (this depends on your coordinate system)
            vertices.emplace_back(x, z, y);

            // Normals
            normals.emplace_back(vertex_entry.normal[0], vertex_entry.normal[2], vertex_entry.normal[1]);

            // Texture coordinates (set to zero if not used)
            tverts.emplace_back(vertex_entry.texcoord0[0], vertex_entry.texcoord0[1]);
        }

        // **Process faces for this sub-mesh**
        for (size_t i = facePos; i < facePos + faceCount; ++i) {
            if (i >= face->entry.size()) {
                std::cerr << "Face index out of range: " << i << std::endl;
                return false;
            }
            const auto& face_indices = face->entry[i];

            // Adjust indices to be zero-based and consider global vertex offset
            faces.emplace_back(
                face_indices[0] - vertPos + globalVertexOffset,
                face_indices[2] - vertPos + globalVertexOffset,
                face_indices[1] - vertPos + globalVertexOffset
            );

            // **Assign the material ID to this face**
            materialIDs.push_back(materialID);
        }

        // Update global vertex offset
        globalVertexOffset += vertCount;
    }

    // Make the OpenGL context current before adding the mesh
    glWindow->make_current();

    // Add the mesh to the viewer, passing materials
    glWindow->addMesh(vertices, faces, materialIDs, tverts, materials, normals);

    // Set up the meshes (create VAOs, VBOs, etc.)
    glWindow->setupMeshes();

    // Optionally, zoom to extents
    glWindow->zoomExtents();

    return true;
}

bool loadMEFFile(const char* fileName, MyGlWindow* glWindow) {
    // Open the file in binary mode
    std::ifstream file(fileName, std::ios::binary);

    // Check if the file opened successfully
    if (!file.is_open()) {
        std::cerr << "Failed to open input mesh file: " << fileName << std::endl;
        return false;
    }

    // Create a mefFile_t instance
    mefFile_t mefFile;

    // Read the file content
    if (!mefFile.readData(file)) {
        std::cerr << "Failed to read content from input file: " << fileName << std::endl;
        file.close();
        return false;
    }
    file.close();

    // Load the mesh using the new function
    if (!loadMeshFromMEF(mefFile, glWindow)) {
        std::cerr << "Failed to load mesh from MEF file." << std::endl;
        return false;
    }

    return true;
}



bool loadMTP( const char* fileName, Fl_Browser* modelList, MyGlWindow* glWindow ) {
    std::string basePath = getFilename::Path(fileName);   // Get the base path of the MTP file
    std::string baseName = getFilename::File(fileName);   // Get the base name (without extension)

    std::cout << "File: \t" << fileName << std::endl;
    std::cout << "Base Path: \t" << basePath << std::endl;
    std::cout << "Base Name: \t" << baseName << std::endl;

    // Open the MTP file
    std::ifstream mtpFileStream(fileName, std::ios::binary);
    if (!mtpFileStream) {
        std::cerr << "Error: Unable to open MTP file: " << fileName << std::endl;
        return false;
    }

    mtpFile_t mtp;
    if (!mtp.read(mtpFileStream)) {
        std::cerr << "Error: Failed to read MTP file: " << fileName << std::endl;
        return false;
    }
    mtpFileStream.close();

    // Declare and initialize modelToTextureIndices
    std::unordered_map<int, std::vector<int>> modelToTextureIndices;

    // Parse the INST chunk to populate modelToTextureIndices
    const mtpInstanceTable_t* instTable = nullptr;
    for (const auto& chunk : mtp.res) {
        if (chunk.type_debug == "INST") {
            instTable = static_cast<mtpInstanceTable_t*>(chunk.res);
            break;
        }
    }

    if (instTable) {
        for (const auto& instance : instTable->instances) {
            int modelIndex = instance.index; // Index into MODS (model names)

            // Convert vector<unsigned int> to vector<int> and store in modelToTextureIndices
            std::vector<int> textureIndices(instance.indices.begin(), instance.indices.end());
            modelToTextureIndices[modelIndex] = textureIndices; // Texture indices from TEXF
        }
    } else {
        std::cerr << "INST chunk not found in MTP file." << std::endl;
        return false;
    }


    // Extract model and texture names from MTP data
    std::vector<std::string> modelNames;
    std::vector<std::string> textureNames;

    for (const auto& chunk : mtp.res) {
        if (chunk.type_debug == "MODS") {
            mtpStringTable_t* modTable = static_cast<mtpStringTable_t*>(chunk.res);
            modelNames = modTable->names;
        } else if (chunk.type_debug == "TEXF") {
            mtpStringTable_t* texTable = static_cast<mtpStringTable_t*>(chunk.res);
            textureNames = texTable->names;
        }
    }

    if (modelNames.empty()) {
        std::cerr << "No models found in MTP file." << std::endl;
        return false;
    }

    if (textureNames.empty()) {
        std::cerr << "No textures found in MTP file." << std::endl;
        return false;
    }

    // Construct the path to the model and texture resource files
    std::string modelResPath = basePath + "models/" + baseName + ".res";
    std::string textureResPath = basePath + "textures/" + baseName + ".res";

    std::cout << "Model RES Path: \t" << modelResPath << std::endl;
    std::cout << "Texture RES Path: \t" << textureResPath << std::endl;

    if (!os::doesFileExist(modelResPath)) {
        std::cerr << "Model RES file does not exist: " << modelResPath << std::endl;
        return false;
    }

    if (!os::doesFileExist(textureResPath)) {
        std::cerr << "Texture RES file does not exist: " << textureResPath << std::endl;
        return false;
    }

    // Read the model resource file
    resFile_t modelResFile;
    if (!modelResFile.read(modelResPath.c_str())) {
        std::cerr << "Failed to read model RES file: " << modelResPath << std::endl;
        return false;
    }

    // Read the texture resource file
    resFile_t textureResFile;
    std::unordered_map<std::string, tgaFile_t> textureMap;
    if (os::doesFileExist(textureResPath)) {
        if (!textureResFile.read(textureResPath.c_str())) {
            std::cerr << "Failed to read texture RES file: " << textureResPath << std::endl;
            return false;
        } else {
            // Populate the textureMap
            for (const auto& chunk : textureResFile.chunks) {
                if (!chunk.texture.image_data.empty()) {
                    std::string fullFilename = chunk.filename; // e.g., "LOCAL:textures/100_04_1_argb8888.tex"

                    // Normalize the filename
                    // Remove prefix "LOCAL:textures/" and suffix ".tex"
                    std::string baseName = fullFilename;

                    // Remove prefix if it exists
//                    const std::string prefix = "LOCAL:textures/";
//                    if (baseName.find(prefix) == 0) {
//                        baseName = baseName.substr(prefix.length());
//                    }
//
//                    // Remove suffix if it exists
//                    const std::string suffix = ".tex";
//                    if (baseName.size() >= suffix.size() && baseName.compare(baseName.size() - suffix.size(), suffix.size(), suffix) == 0) {
//                        baseName = baseName.substr(0, baseName.size() - suffix.size());
//                    }
                    baseName = getFilename::File(baseName);
                    // Now baseName should be "100_04_1_argb8888"

                    // Insert into textureMap using normalized name
                    textureMap[baseName] = chunk.texture;
                }
            }
        }
    }

    // Extract MEF models from the model resource file
    std::vector<mefFile_t> mefModels;
    int modelIndex = 0;
    for (const auto& chunk : modelResFile.chunks) {
        if (chunk.chunk_type != static_cast<uint32_t>(MeshResourceType::BODY)) { // 'BODY'
            continue; // Only process 'BODY' chunks
        }

        if (!chunk.model.content.empty()) {
            mefModels.push_back(chunk.model);
            ++modelIndex;
        }
    }

    if (mefModels.empty()) {
        std::cerr << "Failed to find any meshes in the model RES file." << std::endl;
        return false;
    }

    // Populate the list box with model names
    modelIndex = 0;
    for (const auto& mefModel : mefModels) {
        std::string modelName;
        if (modelIndex < static_cast<int>(modelNames.size())) {
            modelName = modelNames[modelIndex];
        } else {
            modelName = "Model " + std::to_string(modelIndex + 1);
        }
        modelList->add(modelName.c_str());
        ++modelIndex;
    }

    // Show the window before loading models to ensure glWindow is ready
    Fl::check();

    // Select the first item in the list
    modelList->select(1);

    // Clear existing meshes
    glWindow->clearMeshes();

    // Load the first model by default with materials
    if (!loadMeshFromMEFWithMaterial(
            mefModels[0],
            glWindow,
            textureNames,
            modelToTextureIndices,
            textureMap,
            0 // modelIndex for the first model
        )) {
        std::cerr << "Failed to load the first mesh with materials from MEF file." << std::endl;
        return false;
    }

    // Set up the callback for model selection
    struct CallbackData {
        std::vector<mefFile_t> models;
        MyGlWindow* glWindow;
        std::vector<std::string> textureNames;
        std::unordered_map<int, std::vector<int>> modelToTextureIndices;
        std::unordered_map<std::string, tgaFile_t> textureMap;
    };

    // Initialize CallbackData
    CallbackData* cbData = new CallbackData{
        std::move(mefModels),
        glWindow,
        textureNames,
        modelToTextureIndices,
        textureMap
    };

    modelList->callback([](Fl_Widget* widget, void* data) {
        Fl_Browser* browser = static_cast<Fl_Browser*>(widget);
        CallbackData* cbData = static_cast<CallbackData*>(data);
        int selectedIndex = browser->value(); // 1-based index

        if (selectedIndex > 0 && selectedIndex <= static_cast<int>(cbData->models.size())) {
            // Clear existing meshes
            cbData->glWindow->clearMeshes();

            // Load the selected mesh with materials
            if (!loadMeshFromMEFWithMaterial(
                    cbData->models[selectedIndex - 1],
                    cbData->glWindow,
                    cbData->textureNames,
                    cbData->modelToTextureIndices,
                    cbData->textureMap,
                    selectedIndex - 1 // modelIndex
                )) {
                std::cerr << "Failed to load selected mesh with materials." << std::endl;
            } else {
                cbData->glWindow->redraw();
            }
        }
    }, cbData);

    return true;
}

bool checkFileSignature(ifstream &file, unsigned int expectedSignature, unsigned long offset = 0) {
    // Move to the required offset
    file.seekg(offset, ios::beg);
    if (!file) {
        cerr << "Error: Unable to seek to offset " << offset << "." << endl;
        return false;
    }

    unsigned int fileSignature = 0;
    file.read(reinterpret_cast<char*>(&fileSignature), sizeof(fileSignature));
    if (!file) {
        cerr << "Error: Unable to read signature from file." << endl;
        return false;
    }

    return fileSignature == expectedSignature;
}

int determineFileType(const char* fileName) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        cerr << "Error: Unable to open file: " << fileName << endl;
        return 1;
    }

    // Checking first 4 bytes for .mef, .res, or .mtp
    if (checkFileSignature(file, static_cast<uint32_t>(MeshResourceType::NewO))) {
        cout << "The file is of type .mef (ascii)." << endl;

        // Read the MEF file
        mefFile_t mefFile;

        // Seek back to the beginning of the file
        file.seekg(0, std::ios::beg);

        // Read the MEF data
        if (!mefFile.readData(file)) {
            std::cerr << "Failed to read MEF file: " << fileName << std::endl;
            return 1;
        }
        file.close();

        // Initialize FLTK and OpenGL
        Fl::visual(FL_DOUBLE | FL_RGB | FL_ALPHA | FL_DEPTH);
        Fl_Window* window = new Fl_Window(800, 600, "3D Model Viewer");

        // Create the GL window inside the main window
        MyGlWindow* glWindow = new MyGlWindow(0, 0, 800, 600, nullptr);
        window->resizable(glWindow);
        window->end();

        // Show the main window
        window->show();

        // Load the mesh using the loadMeshFromMEF function
        if (!loadMeshFromMEF(mefFile, glWindow)) {
            std::cerr << "Failed to load mesh from MEF file." << std::endl;
            return 1;
        }

        // Run the application
        return Fl::run();

    } else if (checkFileSignature(file, static_cast<uint32_t>(MeshResourceType::ILFF))) {
        cout << "The file is of type .res or .mef, INSPECTING FURTHER..." << endl;

        if (checkFileSignature(file, static_cast<uint32_t>(MeshResourceType::MECO), 16)) {
            cout << "The file is a .mef file." << endl;

            mefFile_t mefFile;

            // Seek back to the beginning of the file
            file.seekg(0, std::ios::beg);

            // Read the MEF data
            if (!mefFile.readData(file)) {
                std::cerr << "Failed to read MEF file: " << fileName << std::endl;
                return 1;
            }
            file.close();

            // Initialize FLTK and OpenGL
            Fl::visual(FL_DOUBLE | FL_RGB | FL_ALPHA | FL_DEPTH);
            Fl_Window* window = new Fl_Window(800, 600, "3D Model Viewer");

            // Create the GL window inside the main window
            MyGlWindow* glWindow = new MyGlWindow(0, 0, 800, 600, nullptr);
            window->resizable(glWindow);
            window->end();

            // Show the main window
            window->show();

            // Load the mesh using the loadMeshFromMEF function
            if (!loadMeshFromMEF(mefFile, glWindow)) {
                std::cerr << "Failed to load mesh from MEF file." << std::endl;
                return 1;
            }

            // Run the application
            return Fl::run();
            }
        else if (checkFileSignature(file, static_cast<uint32_t>(MeshResourceType::IRES), 16)) {

            cout << "The file is of type .res either texture or model, INSPECTING FURTHER..." << endl;

            if (checkFileSignature(file, 0x74786574, 42)) { // 'text'
                cout << "The .res file contains a .tex file." << endl;
                TextureViewerWindow win(800, 600, "Texture Viewer", fileName);
                win.show();
                return Fl::run();
            } else {
                cout << "The .res file contains MEF models." << endl;

                // Read the .res file
                resFile_t resFile;
                if (!resFile.read(fileName)) {
                    std::cerr << "Failed to parse RES file: " << fileName << std::endl;
                    return 1;
                }

                // Initialize FLTK and OpenGL
                Fl::visual(FL_DOUBLE | FL_RGB | FL_ALPHA | FL_DEPTH);

                // Create the main window
                Fl_Window* window = new Fl_Window(1000, 600, "3D Model Viewer");

                // Create a group to hold the list box and the viewer
                Fl_Group* group = new Fl_Group(0, 0, window->w(), window->h());
                group->begin();

                // Create the list box
                Fl_Browser* modelList = new Fl_Browser(0, 0, 200, window->h());
                modelList->type(FL_HOLD_BROWSER); // Allow single selection

                // Create the GL window (3D viewer)
                MyGlWindow* glWindow = new MyGlWindow(200, 0, window->w() - 200, window->h(), nullptr);

                group->resizable(glWindow); // Make the GL window resizable
                group->end();

                window->resizable(group);
                window->end();

                // Show the main window
                window->show();

                // Extract and list MEF models
                std::vector<mefFile_t> mefModels;
                int modelIndex = 0;
                for (const auto& chunk : resFile.chunks) {
                    if (chunk.chunk_type != static_cast<uint32_t>(MeshResourceType::BODY)) { // 'BODY'
                        continue; // Only process 'BODY' chunks
                    }

                    if (!chunk.model.content.empty()) {
                        mefModels.push_back(chunk.model);

                        // Use model names if available; otherwise, use indices
                        std::string modelName = "Model " + std::to_string(++modelIndex);
                        modelList->add(modelName.c_str());
                    }
                }

                if (mefModels.empty()) {
                    std::cerr << "Failed to find any meshes in the RES file." << std::endl;
                    return 1;
                }

                // Select the first item in the list
                modelList->select(1);

                // Load the first model by default
                if (!loadMeshFromMEF(mefModels[0], glWindow)) {
                    std::cerr << "Failed to load the first mesh from MEF file." << std::endl;
                    return 1;
                }

                // Set up the callback for model selection
                struct CallbackData {
                    std::vector<mefFile_t>* models;
                    MyGlWindow* glWindow;
                };

                CallbackData* cbData = new CallbackData{ &mefModels, glWindow };

                modelList->callback([](Fl_Widget* widget, void* data) {
                    Fl_Browser* browser = static_cast<Fl_Browser*>(widget);
                    CallbackData* cbData = static_cast<CallbackData*>(data);
                    int selectedIndex = browser->value(); // 1-based index

                    if (selectedIndex > 0 && selectedIndex <= static_cast<int>(cbData->models->size())) {
                        // Load the selected mesh
                        if (!loadMeshFromMEF((*cbData->models)[selectedIndex - 1], cbData->glWindow)) {
                            std::cerr << "Failed to load selected mesh." << std::endl;
                        } else {
                            cbData->glWindow->redraw();
                        }
                    }
                }, cbData);

                // Run the application
                int result = Fl::run();

                // Clean up
                delete cbData;
                return result;
            }

        } else {
            cout << "The .res file contains an unknown file type." << endl;
            return 1;
        }
    } else if (checkFileSignature(file, 0x4D524F46)) {
        cout << "The file is of type .mtp." << endl;


        // Initialize FLTK and OpenGL
        Fl::visual(FL_DOUBLE | FL_RGB | FL_ALPHA | FL_DEPTH);

        // Create the main window
        Fl_Window* window = new Fl_Window(1000, 600, "3D Model Viewer");

        // Create a group to hold the list box and the viewer
        Fl_Group* group = new Fl_Group(0, 0, window->w(), window->h());
        group->begin();

        // Create the list box
        Fl_Browser* modelList = new Fl_Browser(0, 0, 200, window->h());
        modelList->type(FL_HOLD_BROWSER); // Allow single selection

        // Create the GL window (3D viewer)
        MyGlWindow* glWindow = new MyGlWindow(200, 0, window->w() - 200, window->h(), nullptr);

        group->resizable(glWindow); // Make the GL window resizable
        group->end();

        window->resizable(group);
        window->end();

        // Show the main window
        window->show();

        // Load the MTP file
        if (!loadMTP(fileName, modelList, glWindow)) {
            cerr << "Failed to load MTP file: " << fileName << endl;
            return 1;
        }

        // Run the application
        return Fl::run();
    } else {
        cout << "Unknown file type." << endl;
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    std::string filename;

    if (argc < 2) {
        // Define the filter string for the Open File dialog
        // The format is:
        // "Description1\0Filter1\0Description2\0Filter2\0... \0"
        // Each description and filter pair is separated by a null character ('\0')
        // The entire string is terminated by an additional null character
        const char filter[] =
            "Supported Files (*.mef;*.res;*.mtp)\0*.mef;*.res;*.mtp\0"
            "All Files (*.*)\0*.*\0\0";

        // Open the File Open dialog
        filename = os::getOpenFileName(filter, NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);

        if (filename.empty()) {
            std::cerr << "No file selected. Exiting." << std::endl;
            return 1;
        }
    }
    else {
        filename = argv[1];
    }

    // Proceed with determining the file type
    return determineFileType(filename.c_str());
}
