//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "GLTextureUtil.h"
#include <Saba/Base/File.h>
#include <Saba/Base/Path.h>
#include <Saba/Base/Log.h>

#include <iostream>

#define ENABLE_GLI 0

#if ENABLE_GLI
#include <gli/load.hpp>
#include <gli/convert.hpp>
#include <gli/generate_mipmaps.hpp>
#include <gli/gli.hpp>
#include <gli/core/flip.hpp>
#endif // ENABLE_GLI

#define	STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

#define TINYDDSLOADER_IMPLEMENTATION
#include <tinyddsloader.h>

using namespace tinyddsloader;

#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc 1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif /* GL_EXT_texture_compression_s3tc */

namespace saba
{
	namespace
	{
#if ENABLE_GLI
		bool LoadTextureFromGLI(const GLTextureObject& tex, gli::texture& gliTex)
		{
			if (gliTex.empty())
			{
				return false;
			}

			gli::gl GL(gli::gl::PROFILE_GL33);
			gli::gl::format const Format = GL.translate(gliTex.format(), gliTex.swizzles());
			GLenum Target = GL.translate(gliTex.target());

			glBindTexture(Target, tex);
			glTexParameteri(Target, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(Target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(gliTex.levels() - 1));
			glTexParameteri(Target, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
			glTexParameteri(Target, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
			glTexParameteri(Target, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
			glTexParameteri(Target, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

			glm::tvec3<GLsizei> const Extent(gliTex.extent());
			GLsizei const FaceTotal = static_cast<GLsizei>(gliTex.layers() * gliTex.faces());

			switch (gliTex.target())
			{
			case gli::TARGET_1D:
				glTexStorage1D(
					Target, static_cast<GLint>(gliTex.levels()), Format.Internal, Extent.x);
				break;
			case gli::TARGET_1D_ARRAY:
			case gli::TARGET_2D:
				glTexStorage2D(
					Target, static_cast<GLint>(gliTex.levels()), Format.Internal,
					Extent.x, gliTex.target() == gli::TARGET_2D ? Extent.y : FaceTotal);
				break;
			case gli::TARGET_CUBE:
				glTexStorage2D(
					Target, static_cast<GLint>(gliTex.levels()), Format.Internal,
					Extent.x, Extent.y);
				break;
			case gli::TARGET_2D_ARRAY:
			case gli::TARGET_3D:
			case gli::TARGET_CUBE_ARRAY:
				glTexStorage3D(
					Target, static_cast<GLint>(gliTex.levels()), Format.Internal,
					Extent.x, Extent.y,
					gliTex.target() == gli::TARGET_3D ? Extent.z : FaceTotal);
				break;
			default:
				return false;
			}

			for (std::size_t Layer = 0; Layer < gliTex.layers(); ++Layer)
				for (std::size_t Face = 0; Face < gliTex.faces(); ++Face)
					for (std::size_t Level = 0; Level < gliTex.levels(); ++Level)
					{
						GLsizei const LayerGL = static_cast<GLsizei>(Layer);
						glm::tvec3<GLsizei> Extent(gliTex.extent(Level));
						Target = gli::is_target_cube(gliTex.target())
							? static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + Face)
							: Target;

						switch (gliTex.target())
						{
						case gli::TARGET_1D:
							if (gli::is_compressed(gliTex.format()))
								glCompressedTexSubImage1D(
									Target, static_cast<GLint>(Level), 0, Extent.x,
									Format.Internal, static_cast<GLsizei>(gliTex.size(Level)),
									gliTex.data(Layer, Face, Level));
							else
								glTexSubImage1D(
									Target, static_cast<GLint>(Level), 0, Extent.x,
									Format.External, Format.Type,
									gliTex.data(Layer, Face, Level));
							break;
						case gli::TARGET_1D_ARRAY:
						case gli::TARGET_2D:
						case gli::TARGET_CUBE:
							if (gli::is_compressed(gliTex.format()))
								glCompressedTexSubImage2D(
									Target, static_cast<GLint>(Level),
									0, 0,
									Extent.x,
									gliTex.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
									Format.Internal, static_cast<GLsizei>(gliTex.size(Level)),
									gliTex.data(Layer, Face, Level));
							else
								glTexSubImage2D(
									Target, static_cast<GLint>(Level),
									0, 0,
									Extent.x,
									gliTex.target() == gli::TARGET_1D_ARRAY ? LayerGL : Extent.y,
									Format.External, Format.Type,
									gliTex.data(Layer, Face, Level));
							break;
						case gli::TARGET_2D_ARRAY:
						case gli::TARGET_3D:
						case gli::TARGET_CUBE_ARRAY:
							if (gli::is_compressed(gliTex.format()))
								glCompressedTexSubImage3D(
									Target, static_cast<GLint>(Level),
									0, 0, 0,
									Extent.x, Extent.y,
									gliTex.target() == gli::TARGET_3D ? Extent.z : LayerGL,
									Format.Internal, static_cast<GLsizei>(gliTex.size(Level)),
									gliTex.data(Layer, Face, Level));
							else
								glTexSubImage3D(
									Target, static_cast<GLint>(Level),
									0, 0, 0,
									Extent.x, Extent.y,
									gliTex.target() == gli::TARGET_3D ? Extent.z : LayerGL,
									Format.External, Format.Type,
									gliTex.data(Layer, Face, Level));
							break;
						default:
							return false;
						}
					}
			return true;
		}

		bool LoadTextureFromGLI(const GLTextureObject& tex, const char * filename)
		{
			File file;
			if (!file.Open(filename))
			{
				return false;
			}
			std::vector<char> data;
			if (!file.ReadAll(&data))
			{
				return false;
			}

			gli::texture gliTex = gli::load(&data[0], data.size());
			if (gliTex.empty())
			{
				return false;
			}
			gliTex = gli::flip(gliTex);
			if (!LoadTextureFromGLI(tex, gliTex))
			{
				return false;
			}
			return true;
		}
#endif // ENABLE_GLI

		struct GLSwizzle {
			GLenum m_r, m_g, m_b, m_a;
		};

		struct GLFormat {
			DDSFile::DXGIFormat m_dxgiFormat;
			GLenum m_type;
			GLenum m_format;
			GLSwizzle m_swizzle;
		};

		bool TranslateFormat(DDSFile::DXGIFormat fmt, GLFormat* outFormat)
		{
			static const GLSwizzle sws[] = {
				{ GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA },
				{ GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA },
				{ GL_BLUE, GL_GREEN, GL_RED, GL_ONE },
				{ GL_RED, GL_GREEN, GL_BLUE, GL_ONE },
			};
			using DXGIFmt = DDSFile::DXGIFormat;
			static const GLFormat formats[] = {
				{ DXGIFmt::R8G8B8A8_UNorm, GL_UNSIGNED_BYTE, GL_RGBA, sws[0] },
				{ DXGIFmt::B8G8R8A8_UNorm, GL_UNSIGNED_BYTE, GL_RGBA, sws[1] },
				{ DXGIFmt::B8G8R8X8_UNorm, GL_UNSIGNED_BYTE, GL_RGBA, sws[2] },
				{ DXGIFmt::BC1_UNorm, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, sws[0] },
				{ DXGIFmt::BC2_UNorm, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, sws[0] },
				{ DXGIFmt::BC3_UNorm, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, sws[0] },
			};
			for (const auto& format : formats)
			{
				if (format.m_dxgiFormat == fmt) 
				{
					if (outFormat) 
					{
						*outFormat = format;
					}
					return true;
				}
			}
			return false;
		}

		bool IsCompressed(GLenum fmt) {
			switch (fmt) 
			{
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				return true;
			default:
				return false;
			}
		}

		bool LoadGLTexture(GLuint tex, DDSFile& dds)
		{
			GLenum target = GL_INVALID_ENUM;
			bool isArray = false;
			if (dds.GetTextureDimension() == DDSFile::TextureDimension::Texture1D)
			{
				if (dds.GetArraySize() > 1)
				{
					target = GL_TEXTURE_1D_ARRAY;
					isArray = true;
				}
				else
				{
					target = GL_TEXTURE_1D;
				}
			}
			else if (dds.GetTextureDimension() == DDSFile::TextureDimension::Texture2D)
			{
				if (dds.GetArraySize() > 1)
				{
					if (dds.IsCubemap())
					{
						if (dds.GetArraySize() > 6)
						{
							target = GL_TEXTURE_CUBE_MAP_ARRAY;
							isArray = true;
						}
						else
						{
							target = GL_TEXTURE_CUBE_MAP;
						}
					}
					else
					{
						target = GL_TEXTURE_2D_ARRAY;
						isArray = true;
					}
				}
				else
				{
					target = GL_TEXTURE_2D;
				}
			}
			else if (dds.GetTextureDimension() == DDSFile::TextureDimension::Texture3D)
			{
				target = GL_TEXTURE_3D;
			}

			GLFormat format;
			if (!TranslateFormat(dds.GetFormat(), &format))
			{
				return false;
			}

			glBindTexture(target, tex);
			glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, dds.GetMipCount() - 1);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_R, format.m_swizzle.m_r);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_G, format.m_swizzle.m_g);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_B, format.m_swizzle.m_b);
			glTexParameteri(target, GL_TEXTURE_SWIZZLE_A, format.m_swizzle.m_a);

			switch (target)
			{
			case GL_TEXTURE_1D:
				glTexStorage1D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth());
				break;
			case GL_TEXTURE_1D_ARRAY:
				glTexStorage2D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth(), dds.GetArraySize());
				break;
			case GL_TEXTURE_2D:
				glTexStorage2D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth(), dds.GetHeight());
				break;
			case GL_TEXTURE_CUBE_MAP:
				glTexStorage2D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth(), dds.GetHeight());
				break;
			case GL_TEXTURE_2D_ARRAY:
				glTexStorage3D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth(), dds.GetHeight(), dds.GetArraySize());
				break;
			case GL_TEXTURE_3D:
				glTexStorage3D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth(), dds.GetHeight(), dds.GetDepth());
				break;
			case GL_TEXTURE_CUBE_MAP_ARRAY:
				glTexStorage3D(target, dds.GetMipCount(), format.m_format,
					dds.GetWidth(), dds.GetHeight(), dds.GetArraySize());
				break;
			default:
				glBindTexture(target, 0);
				return false;
			}
			dds.Flip();

			uint32_t numFaces = dds.IsCubemap() ? 6 : 1;
			for (uint32_t layer = 0; layer < dds.GetArraySize(); layer++)
			{
				for (uint32_t face = 0; face < numFaces; face++)
				{
					for (uint32_t level = 0; level < dds.GetMipCount(); level++)
					{
						GLenum target2 = dds.IsCubemap() ? (GL_TEXTURE_CUBE_MAP_POSITIVE_X + face) : target;
						auto imageData = dds.GetImageData(level, layer * numFaces);
						switch (target)
						{
						case GL_TEXTURE_1D:
							if (IsCompressed(format.m_format))
							{
								glCompressedTexSubImage1D(
									target2, level, 0, imageData->m_width,
									format.m_format, imageData->m_memSlicePitch,
									imageData->m_mem);
							}
							else
							{
								glTexSubImage1D(target2, level, 0,
									imageData->m_width, format.m_format,
									format.m_type, imageData->m_mem);
							}
							break;
						case GL_TEXTURE_1D_ARRAY:
						case GL_TEXTURE_2D:
						case GL_TEXTURE_CUBE_MAP:
							if (IsCompressed(format.m_format))
							{
								auto w = imageData->m_width;
								auto h = isArray ? layer : imageData->m_height;
								glCompressedTexSubImage2D(
									target2, level, 0, 0, w, h, format.m_format,
									imageData->m_memSlicePitch, imageData->m_mem);
							}
							else
							{
								auto w = imageData->m_width;
								auto h = isArray ? layer : imageData->m_height;
								glTexSubImage2D(target2, level, 0, 0, w, h,
									format.m_format, format.m_type,
									imageData->m_mem);
							}
							break;
						default:
							glBindTexture(target, 0);
							return false;
						}
					}
				}
			}

			glBindTexture(target, 0);
			return true;
		}

		bool LoadTextureFromDDS(const GLTextureObject& tex, const char * filename)
		{
			File file;
			if (!file.Open(filename))
			{
				return false;
			}
			std::vector<uint8_t> data;
			if (!file.ReadAll(&data))
			{
				return false;
			}

			tinyddsloader::DDSFile dds;
			if (tinyddsloader::Result::Success != dds.Load(std::move(data)))
			{
				return false;
			}

			if (!LoadGLTexture(tex, dds))
			{
				return false;
			}

			return true;
		}

		bool LoadTextureFromStb(const GLTextureObject& tex, const char * filename, bool genMipMap, bool rgba)
		{
			glBindTexture(GL_TEXTURE_2D, tex);

			stbi_set_flip_vertically_on_load(true);

			File file;
			if (!file.Open(filename))
			{
				return false;
			}
			int x, y, comp;
			int reqComp = 0;
			int ret = stbi_info_from_file(file.GetFilePointer(), &x, &y, &comp);
			if (ret == 0)
			{
				return false;
			}

			if (comp != 4)
			{
				if (rgba)
				{
					reqComp = STBI_rgb_alpha;
				}
				else
				{
					reqComp = STBI_rgb;
				}
			}
			else
			{
				reqComp = STBI_rgb_alpha;
			}

			file.Seek(0, File::SeekDir::Begin);
			if (stbi_is_hdr_from_file(file.GetFilePointer()))
			{
				file.Seek(0, File::SeekDir::Begin);
				float* image = stbi_loadf_from_file(file.GetFilePointer(), &x, &y, &comp, reqComp);
				if (image == nullptr)
				{
					return false;
				}

				if (reqComp == STBI_rgb_alpha)
				{
					glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, x, y, 0, GL_RGBA, GL_FLOAT, image);
				}
				else
				{
					glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, x, y, 0, GL_RGB, GL_FLOAT, image);
				}

				stbi_image_free(image);
			}
			else
			{
				file.Seek(0, File::SeekDir::Begin);
				uint8_t* image = stbi_load_from_file(file.GetFilePointer(), &x, &y, &comp, reqComp);
				if (image == nullptr)
				{
					return false;
				}

				if (reqComp == STBI_rgb_alpha)
				{
					glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
				}
				else
				{
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
				}

				stbi_image_free(image);
			}

			if (genMipMap)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			return true;
		}
	}

	GLTextureObject CreateTextureFromFile(const char * filename, bool genMipMap, bool rgba)
	{
		GLTextureObject tex;
		if (!tex.Create())
		{
			SABA_ERROR("Texture Create fail.");
			return GLTextureObject();
		}

		bool ret = LoadTextureFromFile(tex, filename, genMipMap);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (!ret)
		{
			return GLTextureObject();
		}

		return tex;
	}

	GLTextureObject CreateTextureFromFile(const std::string & filename, bool genMipMap, bool rgba)
	{
		return CreateTextureFromFile(filename.c_str(), genMipMap, rgba);
	}

	bool LoadTextureFromFile(const GLTextureObject& tex, const char* filename, bool genMipMap, bool rgba)
	{
		SABA_INFO("LoadTexture: [{}]", filename);
		std::string ext = PathUtil::GetExt(filename);
		bool successed = false;
		if (ext == "dds")
		{
#if ENABLE_GLI
			successed = LoadTextureFromGLI(tex, filename);
			if (!successed)
			{
				SABA_WARN("LoadTextureFromGLI fail.");
			}
#else // ENABLE_GLI
			successed = LoadTextureFromDDS(tex, filename);
			if (!successed)
			{
				SABA_WARN("LoadTextureFromDDS fail.");
			}
#endif // ENABLE_GLI
		}
		else
		{
			successed = LoadTextureFromStb(tex, filename, genMipMap, rgba);
			if (!successed)
			{
				SABA_WARN("LoadTextureFromStb fail.");
			}
		}

		if (successed)
		{
			SABA_INFO("LoadTexture: [{}] Success", filename);
		}
		else
		{
			SABA_WARN("LoadTexture: [{}] Fail", filename);
		}

		return successed;
	}

	bool LoadTextureFromFile(const GLTextureObject & tex, const std::string & filename, bool genMipMap, bool rgba)
	{
		return LoadTextureFromFile(tex, filename.c_str(), genMipMap, rgba);
	}

	bool IsAlphaTexture(GLuint tex)
	{
		int alpha;
		glBindTexture(GL_TEXTURE_2D, tex);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE, &alpha);
		glBindTexture(GL_TEXTURE_2D, 0);
		return alpha != 0;
	}

}

