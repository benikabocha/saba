//
// Copyright(c) 2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef TINYXLOADER_H_
#define TINYXLOADER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <streambuf>
#include <algorithm>
#include <iterator>
#include <memory>
#include <stack>
#include <cctype>
#include <sstream>

namespace tinyxfile {

struct Vector3 {
    float x, y, z;
};

struct Vector2 {
    float x, y;
};

struct Matrix {
    float m[4 * 4];
};

struct ColorRGB {
    float r, g, b;
};

struct ColorRGBA {
    float r, g, b, a;
};

struct Face {
    int v1, v2, v3;
};

struct Material {
    ColorRGBA	m_diffuse;
    float		m_specularPower;
    ColorRGB	m_specular;
    ColorRGB	m_emissive;
    std::string	m_texture;
};

struct Mesh {
    std::string				m_name;

    std::vector<Vector3>	m_positions;
    std::vector<Face>		m_positionFaces;

    std::vector<Vector3>	m_normals;
    std::vector<Face>		m_normalFaces;

    std::vector<Vector2>	m_textureCoords;

    std::vector<Material>	m_materials;
    std::vector<int>		m_faceMaterials;
};

struct Frame {
    Frame();

    std::string		m_name;

    Matrix			m_transform;
    Mesh			m_mesh;

    Frame*              m_parent;
    std::vector<Frame*>	m_childFrames;
};

struct XFile {
    using FrameUPtr = std::unique_ptr<Frame>;
    
    std::vector<FrameUPtr>  m_frames;
};

class XFileLoader {
public:
    std::string GetErrorMessage() const;

    bool Load(const char* filepath, XFile* xfile);
    bool Load(std::istream& input, XFile* xfile);

private:
    bool ReadMesh(Mesh& mesh);
    bool ReadMeshNormals(Mesh& mesh);
    bool ReadMeshTextureCoords(Mesh& mesh);
    bool ReadMeshMaterialList(Mesh& mesh);
    bool ReadFrame(const std::string& name, Frame* parent);

private:
    std::string ReadLine();
    void SkipWS();
    std::string NextTerm();
    bool ReadInt(int& val, int lastCh);
    bool ReadFloat(float& val, int lastCh);
    bool ReadString(std::string& val, int lastCh);
    bool CheckDelimiter(int lastCh);
    bool TryNextBlock(std::string& blockType, std::string& blockName);
    bool NextBlock(std::string& blockType, std::string& blockName);
    void SkipBlock();
    bool TryToInt(const std::string& text, int& val);
    bool TryToFloat(const std::string& text, float& val);
    std::ostream& Error();

private:
    std::istreambuf_iterator<char>	m_it;
    std::istreambuf_iterator<char>	m_end;
    std::stringstream				m_error;
    int								m_lineCount;

    using FrameUPtr = std::unique_ptr<Frame>;
    std::vector<FrameUPtr>	m_frames;
};

} // namespace tinyxfile

#ifdef TINYXLOADER_IMPLEMENTATION

namespace tinyxfile {
std::string XFileLoader::GetErrorMessage() const {
    return m_error.str();
}

bool XFileLoader::Load(const char* filepath, XFile* xfile) {
    if (xfile == nullptr) {
        m_error << "xfile is nullptr.\n";
        return false;
    }

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        return false;
    }
    return Load(ifs, xfile);
}

bool XFileLoader::Load(std::istream& input, XFile* xfile) {
    if (xfile == nullptr) {
        m_error << "xfile is nullptr.\n";
        return false;
    }

    m_frames.clear();

    m_lineCount = 0;
    m_it = std::istreambuf_iterator<char>(input);

    m_lineCount++;
    std::string header = ReadLine();
    if (header != "xof 0303txt 0032") {
        Error() << "Header error.\n";
        return false;
    }
    m_lineCount++;
    while (m_it != m_end) {
        std::string blockType;
        std::string blockName;

        if (!NextBlock(blockType, blockName)) {
            break;
        }

        if (blockType == "Mesh") {
            auto frame = std::make_unique<Frame>();
            frame->m_mesh.m_name = blockName;
            if (!ReadMesh(frame->m_mesh)) {
                return false;
            }
            m_frames.emplace_back(std::move(frame));
        } else if (blockType == "Frame") {
            if (!ReadFrame(blockName, nullptr)) {
                return false;
            }
        } else {
            SkipBlock();
        }

        SkipWS();
    }

    xfile->m_frames = std::move(m_frames);

    return true;
}

bool XFileLoader::ReadMesh(Mesh& mesh) {
    int numPositions;
    if (!ReadInt(numPositions, ';')) {
        return false;
    }
    mesh.m_positions.resize(numPositions);
    for (int i = 0; i < numPositions; i++) {
        if (!ReadFloat(mesh.m_positions[i].x, ';') ||
            !ReadFloat(mesh.m_positions[i].y, ';') ||
            !ReadFloat(mesh.m_positions[i].z, ';')) {
            return false;
        }

        if (i != numPositions - 1) {
            if (!CheckDelimiter(',')) {
                return false;
            }
        } else {
            if (!CheckDelimiter(';')) {
                return false;
            }
        }
    }

    int numPositionFaces;
    if (!ReadInt(numPositionFaces, ';')) {
        return false;
    }
    mesh.m_positionFaces.resize(numPositionFaces);
    for (int i = 0; i < numPositionFaces; i++) {
        int numVertices;
        if (!ReadInt(numVertices, ';')) {
            return false;
        }
        if (numVertices != 3) {
            return false;
        }

        if (!ReadInt(mesh.m_positionFaces[i].v1, ',') ||
            !ReadInt(mesh.m_positionFaces[i].v2, ',') ||
            !ReadInt(mesh.m_positionFaces[i].v3, ';')) {
            return false;
        }

        if (i != numPositionFaces - 1) {
            if (!CheckDelimiter(',')) {
                return false;
            }
        } else {
            if (!CheckDelimiter(';')) {
                return false;
            }
        }
    }

    SkipWS();

    while (m_it != m_end && '}' != (*m_it)) {
        std::string blockType;
        std::string blockName;
        if (!NextBlock(blockType, blockName)) {
            return false;
        }

        if (blockType == "MeshNormals") {
            if (!ReadMeshNormals(mesh)) {
                return false;
            }
        } else if (blockType == "MeshTextureCoords") {
            if (!ReadMeshTextureCoords(mesh)) {
                return false;
            }
        } else if (blockType == "MeshMaterialList") {
            if (!ReadMeshMaterialList(mesh)) {
                return false;
            }
        } else {
            SkipBlock();
        }
    }

    SkipBlock();

    return true;
}

bool XFileLoader::ReadMeshNormals(Mesh& mesh) {
    int numNormals;
    if (!ReadInt(numNormals, ';')) {
        return false;
    }
    mesh.m_normals.resize(numNormals);
    for (int i = 0; i < numNormals; i++) {
        if (!ReadFloat(mesh.m_normals[i].x, ';') ||
            !ReadFloat(mesh.m_normals[i].y, ';') ||
            !ReadFloat(mesh.m_normals[i].z, ';')) {
            return false;
        }

        if (i != numNormals - 1) {
            if (!CheckDelimiter(',')) {
                return false;
            }
        } else {
            if (!CheckDelimiter(';')) {
                return false;
            }
        }
    }

    int numNormalFaces;
    if (!ReadInt(numNormalFaces, ';')) {
        return false;
    }
    mesh.m_normalFaces.resize(numNormalFaces);
    for (int i = 0; i < numNormalFaces; i++) {
        int numVertices;
        if (!ReadInt(numVertices, ';')) {
            return false;
        }
        if (numVertices != 3) {
            return false;
        }

        if (!ReadInt(mesh.m_normalFaces[i].v1, ',') ||
            !ReadInt(mesh.m_normalFaces[i].v2, ',') ||
            !ReadInt(mesh.m_normalFaces[i].v3, ';')) {
            return false;
        }

        if (i != numNormalFaces - 1) {
            if (!CheckDelimiter(',')) {
                return false;
            }
        } else {
            if (!CheckDelimiter(';')) {
                return false;
            }
        }
    }

    SkipBlock();

    return true;
}

bool XFileLoader::ReadMeshTextureCoords(Mesh& mesh) {
    int numCorrds;
    if (!ReadInt(numCorrds, ';')) {
        return false;
    }
    mesh.m_textureCoords.resize(numCorrds);
    for (int i = 0; i < numCorrds; i++) {
        if (!ReadFloat(mesh.m_textureCoords[i].x, ';') ||
            !ReadFloat(mesh.m_textureCoords[i].y, ';')) {
            return false;
        }

        if (i != numCorrds - 1) {
            if (!CheckDelimiter(',')) {
                return false;
            }
        } else {
            if (!CheckDelimiter(';')) {
                return false;
            }
        }
    }

    SkipBlock();

    return true;
}

bool XFileLoader::ReadMeshMaterialList(Mesh& mesh) {
    int numMaterials;
    if (!ReadInt(numMaterials, ';')) {
        return false;
    }
    mesh.m_materials.resize(numMaterials);

    int numFaces;
    if (!ReadInt(numFaces, ';')) {
        return false;
    }
    mesh.m_faceMaterials.resize(numFaces);
    for (int i = 0; i < numFaces; i++) {
        int lastCh = (i == numFaces - 1) ? ';' : ',';
        if (!ReadInt(mesh.m_faceMaterials[i], lastCh)) {
            return false;
        }
    }

    for (int i = 0; i < numMaterials; i++) {
        std::string blockType;
        std::string blockName;
        if (!NextBlock(blockType, blockName)) {
            return false;
        }

        if (blockType != "Material") {
            Error() << "Failed to find material block.";
            return false;
        }

        if (!ReadFloat(mesh.m_materials[i].m_diffuse.r, ';') ||
            !ReadFloat(mesh.m_materials[i].m_diffuse.g, ';') ||
            !ReadFloat(mesh.m_materials[i].m_diffuse.b, ';') ||
            !ReadFloat(mesh.m_materials[i].m_diffuse.a, ';') ||
            !CheckDelimiter(';')) {
            return false;
        }

        if (!ReadFloat(mesh.m_materials[i].m_specularPower, ';')) {
            return false;
        }

        if (!ReadFloat(mesh.m_materials[i].m_specular.r, ';') ||
            !ReadFloat(mesh.m_materials[i].m_specular.g, ';') ||
            !ReadFloat(mesh.m_materials[i].m_specular.b, ';') ||
            !CheckDelimiter(';')) {
            return false;
        }

        if (!ReadFloat(mesh.m_materials[i].m_emissive.r, ';') ||
            !ReadFloat(mesh.m_materials[i].m_emissive.g, ';') ||
            !ReadFloat(mesh.m_materials[i].m_emissive.b, ';') ||
            !CheckDelimiter(';')) {
            return false;
        }

        if (TryNextBlock(blockType, blockName)) {
            if (blockType == "TextureFilename") {
                if (!ReadString(mesh.m_materials[i].m_texture, ';')) {
                    return false;
                }
            }
            SkipBlock();
        }

        SkipBlock();
    }

    SkipBlock();
    return true;
}

bool XFileLoader::ReadFrame(const std::string& name, Frame* parent) {
    auto newFrame = std::make_unique<Frame>();
    auto frame = newFrame.get();

    m_frames.emplace_back(std::move(newFrame));
    frame->m_name = name;
    if (parent != nullptr) {
        frame->m_parent = parent;
        parent->m_childFrames.push_back(frame);
    }

    std::string blockType;
    std::string blockName;
    if (!NextBlock(blockType, blockName)) {
        return false;
    }
    if (blockType != "FrameTransformMatrix") {
        Error() << "Failed to find FrameTransformMatrix block.\n";
        return false;
    }
    if (!ReadFloat(frame->m_transform.m[0], ',') ||
        !ReadFloat(frame->m_transform.m[1], ',') ||
        !ReadFloat(frame->m_transform.m[2], ',') ||
        !ReadFloat(frame->m_transform.m[3], ',') ||
        !ReadFloat(frame->m_transform.m[4], ',') ||
        !ReadFloat(frame->m_transform.m[5], ',') ||
        !ReadFloat(frame->m_transform.m[6], ',') ||
        !ReadFloat(frame->m_transform.m[7], ',') ||
        !ReadFloat(frame->m_transform.m[8], ',') ||
        !ReadFloat(frame->m_transform.m[9], ',') ||
        !ReadFloat(frame->m_transform.m[10], ',') ||
        !ReadFloat(frame->m_transform.m[11], ',') ||
        !ReadFloat(frame->m_transform.m[12], ',') ||
        !ReadFloat(frame->m_transform.m[13], ',') ||
        !ReadFloat(frame->m_transform.m[14], ',') ||
        !ReadFloat(frame->m_transform.m[15], ';')) {
        return false;
    }
    SkipBlock();

    while (TryNextBlock(blockType, blockName)) {
        if (blockType == "Frame") {
            if (!ReadFrame(blockName, frame)) {
                return false;
            }
        } else if (blockType == "Mesh") {
            frame->m_mesh.m_name = blockName;
            if (!ReadMesh(frame->m_mesh)) {
                return false;
            }
        } else {
            SkipBlock();
        }
    }

    SkipBlock();

    return true;
}

std::string XFileLoader::ReadLine() {
    std::string line;
    auto outputIt = std::back_inserter(line);
    while (m_it != m_end && (*m_it) != '\r' && (*m_it) != '\n') {
        (*outputIt) = (*m_it);
        ++outputIt;
        ++m_it;
    }
    int ch = *m_it;
    ++m_it;
    if (m_it != m_end && '\r' == ch) {
        ++m_it;
        if (m_it != m_end && '\n' == (*m_it)) {
            ++m_it;
        }
    }
    return line;
}

void XFileLoader::SkipWS() {
    while (m_it != m_end) {
        int ch = (*m_it);
        if ('\r' == ch || '\n' == ch) {
            m_lineCount++;
            ++m_it;
            if (m_it != m_end && '\r' == ch && '\n' == (*m_it)) {
                ++m_it;
            }
        } else if (' ' == ch || '\t' == ch) {
            ++m_it;
        } else {
            break;
        }
    }
}

std::string XFileLoader::NextTerm() {
    SkipWS();

    std::string term;
    auto outputIt = std::back_inserter(term);
    while (m_it != m_end) {
        int ch = (*m_it);
        if ('{' != ch &&
            '}' != ch &&
            ';' != ch &&
            ',' != ch &&
            !std::isspace(ch)) {
            (*outputIt) = (*m_it);
            ++outputIt;
            ++m_it;
        } else {
            SkipWS();
            break;
        }
    }

    return term;
}

bool XFileLoader::ReadInt(int& val, int lastCh) {
    auto term = NextTerm();
    if (m_it != m_end && lastCh != (*m_it)) {
        Error() << "Failed to find delimiter.[" << (char)lastCh << "]\n";
        return false;
    }
    ++m_it;
    return TryToInt(term, val);
}

bool XFileLoader::ReadFloat(float& val, int lastCh) {
    auto term = NextTerm();
    if (m_it != m_end && lastCh != (*m_it)) {
        Error() << "Failed to find delimiter.[" << (char)lastCh << "]\n";
        return false;
    }
    ++m_it;
    return TryToFloat(term, val);
}

bool XFileLoader::ReadString(std::string& val, int lastCh) {
    val.clear();
    SkipWS();
    if (m_it != m_end && '\"' == (*m_it)) {
        auto outputIt = std::back_inserter(val);
        ++m_it;
        while (m_it != m_end && '\"' != (*m_it)) {
            (*outputIt) = (*m_it);
            ++outputIt;
            ++m_it;
        }
        if (m_it != m_end && '\"' == (*m_it)) {
            ++m_it;
            SkipWS();
        } else {
            Error() << "Failed to parse string.\n";
            return false;
        }
        if (m_it != m_end && lastCh != (*m_it)) {
            Error() << "Failed to find delimiter.[" << (char)lastCh << "]\n";
            return false;
        }
    } else {
        Error() << "Failed to parse string.\n";
        return false;
    }

    return true;
}

bool XFileLoader::CheckDelimiter(int lastCh) {
    auto term = NextTerm();
    if (m_it != m_end && lastCh != (*m_it)) {
        Error() << "Failed to find delimiter.[" << (char)lastCh << "]\n";
        return false;
    }
    if (!term.empty()) {
        Error() << "Invalid format.\n";
        return false;
    }
    ++m_it;
    return true;
}

bool XFileLoader::TryNextBlock(std::string& blockType, std::string& blockName) {
    blockType.clear();
    blockName.clear();

    if (m_it != m_end) {
        blockType = NextTerm();
        if (m_it != m_end && '{' == (*m_it)) {
            ++m_it;
            return true;
        }

        blockName = NextTerm();
        if (m_it != m_end && '{' == (*m_it)) {
            ++m_it;
            return true;
        }
    }

    return false;
}

bool XFileLoader::NextBlock(std::string& blockType, std::string& blockName) {
    if (!TryNextBlock(blockType, blockName)) {
        Error() << "Failed to find block begin.\n";
        return false;
    }
    return true;
}

void XFileLoader::SkipBlock() {
    while (m_it != m_end) {
        int ch = (*m_it);
        if ('}' == ch) {
            ++m_it;
            SkipWS();
            break;
        } else if ('{' == ch) {
            ++m_it;
            SkipBlock();
        } else if (std::isspace(ch)) {
            SkipWS();
        } else {
            ++m_it;
        }
    }
}

bool XFileLoader::TryToInt(const std::string& text, int& val) {
    try {
        val = std::stoi(text);
    } catch (std::exception& e) {
        Error() << e.what() << "\n";
        return false;
    }
    return true;
}

bool XFileLoader::TryToFloat(const std::string& text, float& val) {
    try {
        val = std::stof(text);
    } catch (std::exception& e) {
        Error() << e.what() << "\n";
        return false;
    }
    return true;
}

std::ostream& XFileLoader::Error() {
    m_error << "(" << m_lineCount << ") : ";
    return m_error;
}

Frame::Frame()
    : m_parent(nullptr)
{
    m_transform.m[0] = 1;
    m_transform.m[1] = 0;
    m_transform.m[2] = 0;
    m_transform.m[3] = 0;

    m_transform.m[4] = 0;
    m_transform.m[5] = 1;
    m_transform.m[6] = 0;
    m_transform.m[7] = 0;

    m_transform.m[8] = 0;
    m_transform.m[9] = 0;
    m_transform.m[10] = 1;
    m_transform.m[11] = 0;

    m_transform.m[12] = 0;
    m_transform.m[13] = 0;
    m_transform.m[14] = 0;
    m_transform.m[15] = 1;
}


} // namespace tinyxfile

#endif // !TINYXLOADER_IMPLEMENTATION

#endif // !TINYXLOADER_H_
