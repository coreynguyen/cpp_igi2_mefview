// main.cpp

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include "viewport3d.h" // Ensure this header includes all necessary declarations
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstdint> // Include for fixed-width integer types
#include <array>   // Include for std::array
#include <vector>
#include <sstream> // Include for stringstream
#include <string>
#include <cstring> // For memset
#include <algorithm>  // For std::find_if
#include <cmath>
#include <unordered_map>
#include <windows.h>

// Template functions for readValueing and writing data
template <typename T>
void readValue(std::ifstream &f, T &data) {
    f.read(reinterpret_cast<char*>(&data), sizeof(T));
}

template <typename T>
void writeValue(std::ofstream &f, const T &data) {
    f.write(reinterpret_cast<const char*>(&data), sizeof(T));
}

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
    MECO = 0x4D45434F         // 'MECO'
};

struct MecoMeshResource {
    virtual ~MecoMeshResource() {}

    // Virtual function for reading
    virtual void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) = 0;

    // Virtual function for writing
    virtual void writeData(std::ofstream &f) const = 0;

    // Virtual function for converting to string
    virtual std::string to_string() const = 0;
};

struct MecoDateStamp_t {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint32_t millisecond;

    MecoDateStamp_t() {
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

struct MecoMeshSphere_t {
    float origin[3];  // Array to hold x, y, z coordinates
    float radius;

    MecoMeshSphere_t() {
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


struct MecoMeshHier_t : public MecoMeshResource {
    std::vector<uint8_t> num_children;   // Store number of children as bytes
    std::vector<std::array<float, 3>> position; // Store positions as arrays of floats

    MecoMeshHier_t() {}

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
            std::cerr << "MecoMeshHier_t: Count is Zero" << std::endl;
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


struct MecoMeshBNam_t : public MecoMeshResource {
    std::vector<std::string> names; // Vector to hold bone names

    MecoMeshBNam_t() {}

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
            std::cerr << "MecoMeshBNam_t: Count is Zero" << std::endl;
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


struct MecoMesh_t : public MecoMeshResource {
    float unk01;
    MecoDateStamp_t date;
    uint32_t model_type;
    uint32_t unk10;
    uint32_t unk11;
    uint32_t unk12;
    std::array<MecoMeshSphere_t, 3> unk16;
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

    MecoMesh_t()
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


struct MecoMeshAttaEntry_t {
    std::string name;          // Name (up to 16 bytes)
    float unk42[3][4];        // 3x4 array
    uint32_t unk43;           // Unknown 32-bit integer
    int32_t bone_index;       // Bone index (signed)

    MecoMeshAttaEntry_t() {
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

struct MecoMeshAtta_t : public MecoMeshResource {
    std::vector<MecoMeshAttaEntry_t> entries; // Vector to hold entries

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


struct MecoMeshFace_t : public MecoMeshResource {
    std::vector<std::array<uint16_t, 3>> entry; // Using array for fixed size faces

    MecoMeshFace_t() {
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

struct MecoMeshMVtx_t : public MecoMeshResource {
    std::vector<std::array<float, 4>> entry; // Using array for fixed size vertices

    MecoMeshMVtx_t() {
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
        oss << "MecoMeshMVtx_t(entry=["; // Format output for string representation
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


struct MecoMeshRD3D_t : public MecoMeshResource {
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

    MecoMeshRD3D_t() {
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

    // Override readData method to match MecoMeshResource signature
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

    // Override writeData method to match MecoMeshResource signature
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

    // Override to_string method to match MecoMeshResource signature
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


struct MecoMeshRendEntry_t {
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

    MecoMeshRendEntry_t() {
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
        std::cout << "[MecoMeshRendEntry_t::read] Starting to read entry with size " << size << " bytes." << std::endl;

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
            std::cout << "[MecoMeshRendEntry_t::read] Read lightmap_index: " << lightmap_index << std::endl;
        }
        else if (size == 32) {
            readValue(f, texture_bump_index);
            readValue(f, texture_reflection_index);
            readValue(f, texture_reflection_detail);
            readValue(f, texture_bump_detail);
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_bump_index: " << texture_bump_index << std::endl;
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_reflection_index: " << texture_reflection_index << std::endl;
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_reflection_detail: " << static_cast<int>(texture_reflection_detail) << std::endl;
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_bump_detail: " << static_cast<int>(texture_bump_detail) << std::endl;
        }
        else if (size > 32) {
            // Read additional fields when size > 32
            readValue(f, texture_bump_index);
            readValue(f, texture_reflection_index);
            readValue(f, texture_reflection_detail);
            readValue(f, texture_bump_detail);

            std::cout << "[MecoMeshRendEntry_t::read] Read texture_bump_index: " << texture_bump_index << std::endl;
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_reflection_index: " << texture_reflection_index << std::endl;
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_reflection_detail: " << static_cast<int>(texture_reflection_detail) << std::endl;
            std::cout << "[MecoMeshRendEntry_t::read] Read texture_bump_detail: " << static_cast<int>(texture_bump_detail) << std::endl;

            // Calculate remaining bytes for reserved extra data
            uint32_t extra_size = size - 32;
            uint32_t num_extra = extra_size / 4;
            if (extra_size % 4 != 0) {
                std::cerr << "[MecoMeshRendEntry_t::read] Warning: Extra data size (" << extra_size << " bytes) is not a multiple of 4 bytes." << std::endl;
            }

            reserved_extra.resize(num_extra);
            for (auto &extra : reserved_extra) {
                readValue(f, extra);
                std::cout << "[MecoMeshRendEntry_t::read] Read reserved_extra: " << extra << std::endl;
            }
            std::cout << "[MecoMeshRendEntry_t::read] Warning: Unknown size " << size << " bytes. Extra reserved data captured." << std::endl;
        }
        else if (size > 28) {
            // Handle sizes between 28 and 32 (if any)
            readValue(f, lightmap_index);
            std::cout << "[MecoMeshRendEntry_t::read] Read lightmap_index: " << lightmap_index << std::endl;

            uint32_t extra_size = size - 28;
            uint32_t num_extra = extra_size / 4;
            if (extra_size % 4 != 0) {
                std::cerr << "[MecoMeshRendEntry_t::read] Warning: Extra data size (" << extra_size << " bytes) is not a multiple of 4 bytes." << std::endl;
            }

            reserved_extra.resize(num_extra);
            for (auto &extra : reserved_extra) {
                readValue(f, extra);
                std::cout << "[MecoMeshRendEntry_t::read] Read reserved_extra: " << extra << std::endl;
            }
            std::cout << "[MecoMeshRendEntry_t::read] Warning: Unknown size " << size << " bytes. Extra reserved data captured." << std::endl;
        }
        else {
            std::cerr << "[MecoMeshRendEntry_t::read] Error: Specified size " << size << " bytes is too small or unknown." << std::endl;
        }

        // Debug output for the read entry
        std::cout << "[MecoMeshRendEntry_t::read] Finished reading entry:\n" << to_string() << std::endl;
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

struct MecoMeshRend_t : public MecoMeshResource {
    std::vector<MecoMeshRendEntry_t> entry;

    MecoMeshRend_t() = default;

    // Override the readData method
    void readData(std::ifstream &f, uint32_t data_size, int model_type = 0) override {
        entry.clear();

        // Determine stride based on model_type
        uint32_t stride = 32; // Default stride
        if (model_type == 3) {
            stride = 28;
        }

        // Debug information
        std::cout << "[MecoMeshRend_t::readData] Starting to read Rend data." << std::endl;
        std::cout << "data_size: " << data_size << " bytes" << std::endl;
        std::cout << "model_type: " << model_type << std::endl;
        std::cout << "stride: " << stride << " bytes" << std::endl;

        // Check if data_size is sufficient
        if (data_size < stride) {
            std::cerr << "[MecoMeshRend_t::readData] Error: Data size (" << data_size << " bytes) is smaller than stride (" << stride << " bytes)." << std::endl;
            return;
        }

        // Calculate number of entries
        uint32_t count = data_size / stride;
        uint32_t remaining_bytes = data_size % stride;

        std::cout << "count: " << count << std::endl;
        std::cout << "remaining_bytes: " << remaining_bytes << std::endl;

        if (remaining_bytes != 0) {
            std::cerr << "[MecoMeshRend_t::readData] Warning: Data size (" << data_size << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
        }

        if (count == 0) {
            std::cerr << "[MecoMeshRend_t::readData] Warning: No entries to read." << std::endl;
            return;
        }

        std::cout << "[MecoMeshRend_t::readData] Reading " << count << " Rend entries with stride " << stride << " bytes." << std::endl;

        // Resize the entry vector and read each entry
        entry.resize(count);
        for (uint32_t i = 0; i < count; ++i) {
            std::cout << "[MecoMeshRend_t::readData] Reading Rend Entry " << i << std::endl;
            entry[i].read(f, stride);
        }

        std::cout << "[MecoMeshRend_t::readData] Successfully read Rend entries." << std::endl;
    }

    // Override the writeData method
    void writeData(std::ofstream &f) const override {
        // Assuming a fixed stride of 32 for writing
        uint32_t stride = 32;
        std::cout << "[MecoMeshRend_t::writeData] Writing Rend entries with stride " << stride << " bytes." << std::endl;
        for (size_t i = 0; i < entry.size(); ++i) {
            std::cout << "[MecoMeshRend_t::writeData] Writing Rend Entry " << i << std::endl;
            entry[i].write(f, stride); // Use fixed stride
        }
        std::cout << "[MecoMeshRend_t::writeData] Finished writing Rend entries." << std::endl;
    }

    std::string to_string() const override {
        std::ostringstream oss;
        for (size_t i = 0; i < entry.size(); ++i) {
            oss << "Rend Entry " << i << ":\n" << entry[i].to_string() << "\n";
        }
        return oss.str();
    }
};



struct MecoMeshVrtxEntry_t {
    std::array<float, 3> position;    // x, y, z coordinates
    std::array<float, 2> texcoord0;  // First texture coordinates
    std::array<float, 3> normal;      // Normal vector
    float weight;                     // Weight for vertex
    std::array<float, 2> texcoord1;  // Second texture coordinates
    uint16_t vertex_index;            // Vertex index
    uint16_t bone_index;              // Bone index

    MecoMeshVrtxEntry_t()
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

struct MecoMeshVrtx_t : public MecoMeshResource {
    std::vector<MecoMeshVrtxEntry_t> entry; // Vector to hold vertex entries

    MecoMeshVrtx_t() {}

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
            oss << vertex.to_string() << "\n"; // Ensure you implement to_string in MecoMeshVrtxEntry_t
        }
        return oss.str();
    }
};


struct MecoMeshCSphEntry_t {
    float position[3]; // Array to hold x, y, z coordinates
    float radius;
    uint16_t unk72;
    uint16_t unk73;
    uint16_t unk74;
    uint16_t unk75;

    MecoMeshCSphEntry_t() {
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
        oss << "MecoMeshCSphEntry_t(position=["
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

struct MecoMeshCSph_t : public MecoMeshResource {
    std::vector<MecoMeshCSphEntry_t> entry; // Vector to hold multiple entries

    // Updated to match the new virtual method signature
    void readData(std::ifstream &f, uint32_t count = 0, int model_type = 0) override {
        entry.clear(); // Clear existing entries
        if (count > 0) {
            for (uint32_t i = 0; i < count; ++i) {
                MecoMeshCSphEntry_t sph_entry;
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
        oss << "MecoMeshCSph_t(entry=[";
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

struct MecoMeshCVtxEntry_t {
    float position[3];  // Array to hold x, y, z coordinates
    uint32_t bone_index; // Assuming bone_index is a uint32_t
    uint32_t unk78;      // Assuming unk78 is a uint32_t

    MecoMeshCVtxEntry_t() {
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
        oss << "MecoMeshCVtxEntry_t(position=["
            << position[0] << ", "
            << position[1] << ", "
            << position[2] << "], bone_index="
            << bone_index << ", unk78="
            << unk78 << ")";
        return oss.str();
    }
};

// Struct for vertex list
struct MecoMeshCVtx_t : public MecoMeshResource {
    std::vector<MecoMeshCVtxEntry_t> entry;

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
        oss << "MecoMeshCVtx_t(entry=[";
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
struct MecoMeshCFceEntry_t {
    uint16_t face[3];        // Array to hold vertex indices for the face
    int16_t material_index;   // Assuming signed short for material_index
    int16_t unk80;            // Assuming signed short for unk80
    int16_t unk81;            // Assuming signed short for unk81

    MecoMeshCFceEntry_t() {
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
        oss << "MecoMeshCFceEntry_t(face=["
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
struct MecoMeshCFce_t : public MecoMeshResource {
    std::vector<MecoMeshCFceEntry_t> entry;

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
        oss << "MecoMeshCFce_t(entry=[";
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

struct MecoMeshCMshEntry_t {
    uint32_t num_faces;
    uint32_t num_vertices;
    uint32_t num_materials;
    uint32_t num_spheres;
    uint32_t reserved[4]; // Array to hold reserved values

    MecoMeshCMshEntry_t() : num_faces(0), num_vertices(0), num_materials(0), num_spheres(0) {
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
        oss << "MecoMeshCMshEntry_t(num_faces=" << num_faces
            << ", num_vertices=" << num_vertices
            << ", num_materials=" << num_materials
            << ", num_spheres=" << num_spheres
            << ", reserved=[" << reserved[0] << ", " << reserved[1]
            << ", " << reserved[2] << ", " << reserved[3] << "])";
        return oss.str();
    }
};

struct MecoMeshCMsh_t : public MecoMeshResource {
    std::vector<MecoMeshCMshEntry_t> entry;

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
        oss << "MecoMeshCMsh_t(entry=[";
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

struct MecoMeshCMatEntry_t {
    uint32_t unk48;
    int16_t unk49;  // Assuming signed short
    uint16_t unk50; // Assuming unsigned short
    uint32_t unk51;
    uint32_t unk52;

    MecoMeshCMatEntry_t() : unk48(0), unk49(0), unk50(0), unk51(0), unk52(0) {}

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
        oss << "MecoMeshCMatEntry_t(unk48=" << unk48
            << ", unk49=" << unk49
            << ", unk50=" << unk50
            << ", unk51=" << unk51
            << ", unk52=" << unk52 << ")";
        return oss.str();
    }
};

struct MecoMeshCMat_t : public MecoMeshResource {
    std::vector<MecoMeshCMatEntry_t> entry;

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
        oss << "MecoMeshCMat_t(entry=[";
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

struct MecoMeshMrphEntry_t {
    uint32_t index;
    float delta[3];

    MecoMeshMrphEntry_t() {
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
        oss << "MecoMeshMrphEntry_t(index=" << index << ", delta=["
            << delta[0] << ", " << delta[1] << ", " << delta[2] << "])";
        return oss.str();
    }
};
struct MecoMeshMrph_t : public MecoMeshResource {
    std::vector<std::vector<MecoMeshMrphEntry_t>> entry;

    MecoMeshMrph_t() : entry(16) {}  // Initialize entry with 16 vectors

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
                    MecoMeshMrphEntry_t mrph_entry;
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
        oss << "MecoMeshMrph_t(entry=[";
        for (const auto& e : entry) {
            for (const auto& entry_item : e) {
                oss << entry_item.to_string() << ", ";
            }
        }
        oss << "])";
        return oss.str();
    }
};


struct MecoMeshChunk_t {
    uint32_t type;
    uint32_t data;
    uint32_t flag;
    uint32_t size;
    MecoMeshResource* res;  // Pointer to base class for different resource types
    std::string type_debug;

    // Constructor
    MecoMeshChunk_t()
        : type(0), data(0), flag(0), size(0), res(nullptr), type_debug("") {}

    // Destructor
    ~MecoMeshChunk_t() {
        if (res) {
            delete res;
            res = nullptr;
        }
    }

    // Template method to create a resource and read its data
    template <typename T>
    MecoMeshResource* createResource(std::ifstream &f, uint32_t count = 0, int model_type = 0) {
        T* resource = new T();
        resource->readData(f, count, model_type); // Corrected to call readData
        return resource;
    }

    // Template method to clone a resource
    template <typename T>
    MecoMeshResource* cloneResource(const MecoMeshResource* other_res) {
        const T* derived_res = dynamic_cast<const T*>(other_res);
        return derived_res ? new T(*derived_res) : nullptr;
    }

    // Copy Constructor
    MecoMeshChunk_t(const MecoMeshChunk_t& other)
        : type(other.type), data(other.data), flag(other.flag),
          size(other.size), type_debug(other.type_debug), res(nullptr) { // Initialize res to nullptr

        if (other.res) {
            // Clone the resource based on the type
            switch (static_cast<MeshResourceType>(other.type)) {
                case MeshResourceType::MESH:
                    res = cloneResource<MecoMesh_t>(other.res);
                    break;
                case MeshResourceType::ATTA:
                    res = cloneResource<MecoMeshAtta_t>(other.res);
                    break;
                case MeshResourceType::MVTX:
                    res = cloneResource<MecoMeshMVtx_t>(other.res);
                    break;
                case MeshResourceType::RD3D:
                    res = cloneResource<MecoMeshRD3D_t>(other.res);
                    break;
                case MeshResourceType::HIER:
                    res = cloneResource<MecoMeshHier_t>(other.res);
                    break;
                case MeshResourceType::BNAM:
                    res = cloneResource<MecoMeshBNam_t>(other.res);
                    break;
                case MeshResourceType::FACE:
                    res = cloneResource<MecoMeshFace_t>(other.res);
                    break;
                case MeshResourceType::REND:
                    res = cloneResource<MecoMeshRend_t>(other.res);
                    break;
                case MeshResourceType::VRTX:
                    res = cloneResource<MecoMeshVrtx_t>(other.res);
                    break;
                case MeshResourceType::CVTX:
                    res = cloneResource<MecoMeshCVtx_t>(other.res);
                    break;
                case MeshResourceType::CFCE:
                    res = cloneResource<MecoMeshCFce_t>(other.res);
                    break;
                case MeshResourceType::CMAT:
                    res = cloneResource<MecoMeshCMat_t>(other.res);
                    break;
                case MeshResourceType::CSPH:
                    res = cloneResource<MecoMeshCSph_t>(other.res);
                    break;
                case MeshResourceType::MRPH:
                    res = cloneResource<MecoMeshMrph_t>(other.res);
                    break;
                case MeshResourceType::CMSH:
                    res = cloneResource<MecoMeshCMsh_t>(other.res);
                    break;
                default:
                    res = nullptr; // Unknown type
                    break;
            }
        }
    }

    // Copy Assignment Operator
    MecoMeshChunk_t& operator=(const MecoMeshChunk_t& other) {
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
                        res = cloneResource<MecoMesh_t>(other.res);
                        break;
                    case MeshResourceType::ATTA:
                        res = cloneResource<MecoMeshAtta_t>(other.res);
                        break;
                    case MeshResourceType::RD3D:
                        res = cloneResource<MecoMeshRD3D_t>(other.res);
                        break;
                    case MeshResourceType::MVTX:
                        res = cloneResource<MecoMeshMVtx_t>(other.res);
                        break;
                    case MeshResourceType::HIER:
                        res = cloneResource<MecoMeshHier_t>(other.res);
                        break;
                    case MeshResourceType::BNAM:
                        res = cloneResource<MecoMeshBNam_t>(other.res);
                        break;
                    case MeshResourceType::FACE:
                        res = cloneResource<MecoMeshFace_t>(other.res);
                        break;
                    case MeshResourceType::REND:
                        res = cloneResource<MecoMeshRend_t>(other.res);
                        break;
                    case MeshResourceType::VRTX:
                        res = cloneResource<MecoMeshVrtx_t>(other.res);
                        break;
                    case MeshResourceType::CVTX:
                        res = cloneResource<MecoMeshCVtx_t>(other.res);
                        break;
                    case MeshResourceType::CFCE:
                        res = cloneResource<MecoMeshCFce_t>(other.res);
                        break;
                    case MeshResourceType::CMAT:
                        res = cloneResource<MecoMeshCMat_t>(other.res);
                        break;
                    case MeshResourceType::CSPH:
                        res = cloneResource<MecoMeshCSph_t>(other.res);
                        break;
                    case MeshResourceType::MRPH:
                        res = cloneResource<MecoMeshMrph_t>(other.res);
                        break;
                    case MeshResourceType::CMSH:
                        res = cloneResource<MecoMeshCMsh_t>(other.res);
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
    void read(std::ifstream &f, MecoMesh_t &header, MecoMeshRD3D_t &render3D, bool verbose = false, bool stopOnNewChunk = false) {
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
            std::cout << "[MecoMeshChunk_t::read] Chunk { " << type_debug << " } @ " << pos << std::endl;
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
                    res = new MecoMesh_t(header);

                    std::cout << "[MecoMeshChunk_t::read] " << type_debug << ": Count# \t" << data << "\n" << res->to_string() << "\n----------------------------" << std::endl;
                    break;
                }
                case MeshResourceType::ATTA: {
                    uint32_t count = data / sizeof(MecoMeshAttaEntry_t); // Adjust count based on actual data size
                    res = createResource<MecoMeshAtta_t>(f, count, 0); // Assuming model_type is not needed
                    break;
                }
                case MeshResourceType::MVTX: {
                    uint32_t count = data / (sizeof(float) * 4); // Assuming each vertex has 4 floats
                    res = createResource<MecoMeshMVtx_t>(f, count, 0); // Assuming model_type is not needed
                    break;
                }
                case MeshResourceType::RD3D: {
                    render3D.readData(f, data, header.model_type); // Assuming model_type is 0 for RD3D
                    res = new MecoMeshRD3D_t(render3D);
                    break;
                }
                case MeshResourceType::HIER: {
                    uint32_t count = header.num_bones;
                    std::cout << "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB " << count << std::endl;
                    res = createResource<MecoMeshHier_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::BNAM: {
                    uint32_t count = data / 16; // BNAME element size
                    res = createResource<MecoMeshBNam_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::FACE: {
                    uint32_t count = (header.sum_c_faces > 0) ? header.sum_c_faces : data / (sizeof(uint16_t) * 3); // Each face has 3 uint16_t
                    res = createResource<MecoMeshFace_t>(f, count, 0); // Assuming model_type is 0
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
                        std::cerr << "[MecoMeshChunk_t::read] Invalid stride calculated for REND chunk." << std::endl;
                        break;
                    }
                    uint32_t count = data / stride;
                    std::cout << "REND DATA: \t" << data << std::endl;
                    std::cout << "REND stride: \t" << stride << std::endl;
                    std::cout << "REND count: \t" << count << std::endl;
                    uint32_t remaining_bytes = data % stride;
                    std::cout << "REND remaining_bytes: \t" << remaining_bytes << std::endl;
                    if (remaining_bytes != 0) {
                        std::cerr << "[MecoMeshChunk_t::read] Warning: 'REND' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }

                    std::cout << "[MecoMeshChunk_t::read] Reading " << count << " Rend entries with stride " << stride << " bytes." << std::endl;

                    res = createResource<MecoMeshRend_t>(f, data, header.model_type);
                    std::cout << "[MecoMeshChunk_t::read] " << type_debug << ": Count# \t" << count << "\n" << res->to_string() << "\n----------------------------" << std::endl;

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
                        std::cerr << "[MecoMeshChunk_t::read] Warning: 'VRTX' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<MecoMeshVrtx_t>(f, count, header.model_type);
                    break;
                }
                case MeshResourceType::CVTX: {
                    uint32_t stride = (header.sum_c_verts > 0) ? data / header.sum_c_verts : 0;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[MecoMeshChunk_t::read] Invalid stride calculated for CVTX chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[MecoMeshChunk_t::read] Warning: 'CVTX' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<MecoMeshCVtx_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::CFCE: {
                    uint32_t stride = (header.sum_c_faces > 0) ? data / header.sum_c_faces : 0;
                    uint32_t count = (stride > 0) ? data / stride : 0;
                    if (stride == 0) {
                        std::cerr << "[MecoMeshChunk_t::read] Invalid stride calculated for CFCE chunk." << std::endl;
                        break;
                    }
                    if (data % stride != 0) {
                        std::cerr << "[MecoMeshChunk_t::read] Warning: 'CFCE' chunk size (" << data << " bytes) is not a multiple of stride (" << stride << " bytes)." << std::endl;
                    }
                    res = createResource<MecoMeshCFce_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::CMAT: {
                    uint32_t count = data / sizeof(MecoMeshCMatEntry_t); // Adjust count based on material size
                    res = createResource<MecoMeshCMat_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::CSPH: {
                    uint32_t count = data / sizeof(MecoMeshCSphEntry_t); // Sphere size is sizeof(MecoMeshCSphEntry_t)
                    res = createResource<MecoMeshCSph_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                case MeshResourceType::MRPH: {
                    // Assuming 'MRPH' handles multiple morph targets internally
                    res = createResource<MecoMeshMrph_t>(f, 0, 0); // Assuming count and model_type are handled internally
                    break;
                }
                case MeshResourceType::CMSH: {
                    uint32_t count = data / sizeof(MecoMeshCMshEntry_t); // CMSH size
                    res = createResource<MecoMeshCMsh_t>(f, count, 0); // Assuming model_type is 0
                    break;
                }
                default:
                    std::cerr << "[MecoMeshChunk_t::read] Unexpected Chunk {" << type_debug << "} @ " << pos << ", Parsing Halted." << std::endl;
                    if (stopOnNewChunk) {
                        return; // Stop if required
                    }
            }

            // Print verbose information if necessary
            if (verbose && res) {
                std::cout << "[MecoMeshChunk_t::read] " << type_debug << ": Count# \t" << data << "\n" << res->to_string() << "\n----------------------------" << std::endl;
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
        oss << "MecoMeshChunk_t(type=0x" << std::hex << type << std::dec
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


struct MecoFile_t {
    uint32_t file_type;     // File type
    uint32_t file_size;     // File size
    uint32_t file_unk1;     // Unknown field 1
    uint32_t file_unk2;     // Unknown field 2
    uint32_t content_type;  // Content type
    std::vector<MecoMeshChunk_t> content; // Vector to hold content

    MecoFile_t()
        : file_type(static_cast<uint32_t>(MeshResourceType::IFLF)), // Replaced with enum value
          file_size(0),
          file_unk1(4),
          file_unk2(0),
          content_type(static_cast<uint32_t>(MeshResourceType::MECO)) { // Replaced with enum value
    }

    // Function to get content by block name
    MecoMeshChunk_t* get_content(const std::string& block_name) {
        MecoMeshChunk_t* data = nullptr;
        for (auto& c : content) {
            // Assuming MecoMeshChunk_t has a member 'type_debug'
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
                    MecoMesh_t mHeader;      // Initialize mesh header
                    MecoMeshRD3D_t mRender3D; // Initialize render 3D data

                    readValue(f, content_type);

                    // Check for correct content type
                    if (content_type == static_cast<uint32_t>(MeshResourceType::MECO)) {  // 'MECO'
                        while (f.tellg() < file_end) {
                            MecoMeshChunk_t chunk;

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
        oss << "MecoFile_t(file_type=" << file_type
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
        MecoMeshChunk_t* hier_chunk = get_content("HIER");
        MecoMeshChunk_t* bnam_chunk = get_content("BNAM");
        MecoMeshChunk_t* vrtx_chunk = get_content("VRTX");
        MecoMeshChunk_t* face_chunk = get_content("FACE");
        MecoMeshChunk_t* rend_chunk = get_content("REND");

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
            MecoMeshHier_t* hier = dynamic_cast<MecoMeshHier_t*>(hier_chunk->res);
            MecoMeshBNam_t* bnam = dynamic_cast<MecoMeshBNam_t*>(bnam_chunk->res);

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
        MecoMeshVrtx_t* vrtx = dynamic_cast<MecoMeshVrtx_t*>(vrtx_chunk->res);
        MecoMeshFace_t* face = dynamic_cast<MecoMeshFace_t*>(face_chunk->res);
        MecoMeshRend_t* rend = dynamic_cast<MecoMeshRend_t*>(rend_chunk->res);

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
        objFile << "# Exported by MecoFile_t::exportOBJ\n";

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

void assign_parent_indices(size_t bone_index,
    const std::vector<std::vector<size_t>>& bone_children,
    std::vector<int>& parent_indices) {
    for (size_t child_index : bone_children[bone_index]) {
        parent_indices[child_index] = static_cast<int>(bone_index);
        assign_parent_indices(child_index, bone_children, parent_indices);
    }
}

int main(int argc, char** argv) {
    // Initialize FLTK and OpenGL
    Fl::visual(FL_DOUBLE | FL_RGB | FL_ALPHA | FL_DEPTH);
    Fl_Window* window = new Fl_Window(800, 600, "3D Model Viewer");

    // Create the GL window inside the main window
    MyGlWindow* glWindow = new MyGlWindow(0, 0, 800, 600, nullptr);
    window->resizable(glWindow);
    window->end();

    // Show the main window
    window->show();

    // Check for command-line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh_file>" << std::endl;
        return 1;
    }

    // File path to open
    std::string file_path = argv[1];

    // Open the file in binary mode
    std::ifstream file(file_path.c_str(), std::ios::binary);

    // Check if the file opened successfully
    if (!file.is_open()) {
        std::cerr << "Failed to open input mesh file: " << file_path << std::endl;
        return 1;
    }

    // Create a MecoFile_t instance
    MecoFile_t mecoFile;

    // Read the file content
    if (!mecoFile.readData(file)) {
        std::cerr << "Failed to read content from input file: " << file_path << std::endl;
        file.close();
        return 1;
    }
    file.close();

    // Extract mesh data (your existing code)
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> tverts; // Texture coordinates
    std::vector<glm::ivec3> faces;
    std::vector<int> materialIDs;  // Material IDs for each face
    std::vector<Materialm> materials;  // Materials

    float mscale = 0.0003934f; // Scaling factor if needed

    // Retrieve necessary chunks
    MecoMeshChunk_t* hier_chunk = mecoFile.get_content("HIER");
    MecoMeshChunk_t* bnam_chunk = mecoFile.get_content("BNAM");
    MecoMeshChunk_t* vrtx_chunk = mecoFile.get_content("VRTX");
    MecoMeshChunk_t* face_chunk = mecoFile.get_content("FACE");
    MecoMeshChunk_t* rend_chunk = mecoFile.get_content("REND");

    // Check for required chunks
    if (!vrtx_chunk || !face_chunk || !rend_chunk) {
        std::cerr << "Required chunks are missing." << std::endl;
        return 1;
    }

    // Build bone hierarchy if available (your existing code)
    std::vector<std::string> bone_names;
    std::vector<std::array<float, 3>> bone_positions;
    std::vector<int> parent_indices;

    if (hier_chunk && bnam_chunk) {
        MecoMeshHier_t* hier = dynamic_cast<MecoMeshHier_t*>(hier_chunk->res);
        MecoMeshBNam_t* bnam = dynamic_cast<MecoMeshBNam_t*>(bnam_chunk->res);

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
    MecoMeshVrtx_t* vrtx = dynamic_cast<MecoMeshVrtx_t*>(vrtx_chunk->res);
    MecoMeshFace_t* face = dynamic_cast<MecoMeshFace_t*>(face_chunk->res);
    MecoMeshRend_t* rend = dynamic_cast<MecoMeshRend_t*>(rend_chunk->res);

    if (!vrtx || !face || !rend) {
        std::cerr << "Failed to cast resource chunks." << std::endl;
        return 1;
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
        newMaterial.applyRandomColors(); // Ensure each material has random colors
        materials.push_back(newMaterial);

        // **Process vertices for this sub-mesh**
        for (size_t i = vertPos; i < vertPos + vertCount; ++i) {
            if (i >= vrtx->entry.size()) {
                std::cerr << "Vertex index out of range: " << i << std::endl;
                return 1;
            }
            const auto& vertex_entry = vrtx->entry[i];

            // Apply bone transformation if available
            glm::vec3 node_pos(0.0f, 0.0f, 0.0f);
            if (!bone_positions.empty() && vertex_entry.bone_index >= 0 &&
                static_cast<size_t>(vertex_entry.bone_index) < bone_positions.size()) {
                const auto& bone_pos = bone_positions[vertex_entry.bone_index];
                node_pos = glm::vec3(bone_pos[0], bone_pos[1], bone_pos[2]);
            }

            // Compute world position
            float x = vertex_entry.position[0] * mscale + node_pos.x;
            float y = vertex_entry.position[1] * mscale + node_pos.y;
            float z = vertex_entry.position[2] * mscale + node_pos.z;

            // Swap y and z components if needed (this depends on your coordinate system)
            vertices.emplace_back(x, z, y);

            // Normals
            normals.emplace_back(vertex_entry.normal[0], vertex_entry.normal[2], vertex_entry.normal[1]);

            // Texture coordinates
            tverts.emplace_back(vertex_entry.texcoord0[0], vertex_entry.texcoord0[1]);
        }

        // **Process faces for this sub-mesh**
        for (size_t i = facePos; i < facePos + faceCount; ++i) {
            if (i >= face->entry.size()) {
                std::cerr << "Face index out of range: " << i << std::endl;
                return 1;
            }
            const auto& face_indices = face->entry[i];

            // Adjust indices to be zero-based and consider global vertex offset
            faces.emplace_back(
                face_indices[0] - vertPos + globalVertexOffset,
                face_indices[1] - vertPos + globalVertexOffset,
                face_indices[2] - vertPos + globalVertexOffset
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

    // Set up the meshes (create VAOs, VBOs, etc.)
    glWindow->setupMeshes();

    // Optionally, zoom to extents
    glWindow->zoomExtents();

    // Run the application
    return Fl::run();
}

