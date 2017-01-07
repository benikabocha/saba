//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_TEXTUREUTIL_H_
#define SABA_GL_TEXTUREUTIL_H_

#include "GLObject.h"

#include <string>

namespace saba
{
	GLTextureObject CreateTextureFromFile(const char* filename, bool genMipMap = true, bool rgba = false);
	GLTextureObject CreateTextureFromFile(const std::string& filename, bool genMipMap = true, bool rgba = false);

	bool LoadTextureFromFile(const GLTextureObject& tex, const char* filename, bool genMipMap = true, bool rgba = false);
	bool LoadTextureFromFile(const GLTextureObject& tex, const std::string& filename, bool genMipMap = true, bool rgba = false);

	bool IsAlphaTexture(GLuint tex);
}

#endif // !SABA_GL_TEXTUREUTIL_H_

