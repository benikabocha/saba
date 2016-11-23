#ifndef SABA_GL_TEXTUREUTIL_H_
#define SABA_GL_TEXTUREUTIL_H_

#include "GLObject.h"

namespace saba
{
	GLTextureObject CreateTextureFromFile(const char* filename, bool genMipMap = true, bool rgba = false);

	bool LoadTextureFromFile(const GLTextureObject& tex, const char* filename, bool genMipMap = true, bool rgba = false);

	bool IsAlphaTexture(GLuint tex);
}

#endif // !SABA_GL_TEXTUREUTIL_H_

