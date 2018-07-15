//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef GLVERTEXUTIL_H_
#define GLVERTEXUTIL_H_

#include "GLObject.h"

#include <vector>
#include <cstdint>
#include <memory>
#include <algorithm>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace saba
{
	template <typename T>
	GLBufferObject CreateVBO(const T* buf, size_t count, GLenum usage = GL_STATIC_DRAW)
	{
		GLBufferObject vbo;
		vbo.Create();

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(T) * count, buf, usage);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return vbo;
	}

	template <typename T>
	GLBufferObject CreateVBO(const std::vector<T>& buf, GLenum usage = GL_STATIC_DRAW)
	{
		return CreateVBO(&buf[0], buf.size(), usage);
	}

	template <typename T>
	void UpdateVBO(GLuint vbo, const T* buf, size_t count)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(T) * count, buf);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	template <typename T>
	void UpdateVBO(GLuint vbo, const std::vector<T>& buf)
	{
		UpdateVBO(vbo, &buf[0], buf.size());
	}

	template <typename T>
	GLBufferObject CreateIBO(const T* buf, size_t count, GLenum usage = GL_STATIC_DRAW)
	{
		GLBufferObject ibo;
		ibo.Create();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(T) * count, buf, usage);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		return ibo;
	}

	template <typename T>
	GLBufferObject CreateIBO(const std::vector<T>& buf, GLenum usage = GL_STATIC_DRAW)
	{
		return CreateIBO(&buf[0], buf.size(), usage);
	}

	template <typename T>
	struct GLTypeTraits
	{
		static GLenum GetType() { return GL_INVALID_ENUM; }
	};

	template <>
	struct GLTypeTraits<int8_t>
	{
		static GLenum GetType() { return GL_BYTE; }
	};

	template <>
	struct GLTypeTraits<uint8_t>
	{
		static GLenum GetType() { return GL_UNSIGNED_BYTE; }
	};

	template <>
	struct GLTypeTraits<int16_t>
	{
		static GLenum GetType() { return GL_SHORT; }
	};

	template <>
	struct GLTypeTraits<uint16_t>
	{
		static GLenum GetType() { return GL_UNSIGNED_SHORT; }
	};

	template <>
	struct GLTypeTraits<int32_t>
	{
		static GLenum GetType() { return GL_INT; }
	};

	template <>
	struct GLTypeTraits<uint32_t>
	{
		static GLenum GetType() { return GL_UNSIGNED_INT; }
	};

	template <>
	struct GLTypeTraits<float>
	{
		static GLenum GetType() { return GL_FLOAT; }
	};

	template <>
	struct GLTypeTraits<double>
	{
		static GLenum GetType() { return GL_DOUBLE; }
	};

	template <typename T>
	struct GLElementTraits
	{
		static GLenum GetType() { return GLTypeTraits<T>::GetType(); }
		static GLsizei GetNum() { return 1; }
	};

	template <typename T, glm::precision P>
	struct GLElementTraits< glm::tvec2<T, P> >
	{
		static GLenum GetType() { return GLTypeTraits<T>::GetType(); }
		static GLsizei GetNum() { return 2; }
	};

	template <typename T, glm::precision P>
	struct GLElementTraits< glm::tvec3<T, P> >
	{
		static GLenum GetType() { return GLTypeTraits<T>::GetType(); }
		static GLsizei GetNum() { return 3; }
	};

	template <typename T, glm::precision P>
	struct GLElementTraits< glm::tvec4<T, P> >
	{
		static GLenum GetType() { return GLTypeTraits<T>::GetType(); }
		static GLsizei GetNum() { return 4; }
	};

	struct VertexBinder
	{
		VertexBinder()
			: m_elementNum(0)
			, m_elementType(GL_INVALID_ENUM)
			, m_stride(0)
			, m_offset(0)
		{
		}

		VertexBinder(GLint elmeNum, GLenum elemType, GLsizei stride, GLsizei offset)
			: m_elementNum(elmeNum)
			, m_elementType(elemType)
			, m_stride(stride)
			, m_offset(offset)
		{
		}

		void Bind(GLint attr, GLuint vbo) const
		{
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glVertexAttribPointer(attr, m_elementNum, m_elementType, GL_FALSE, m_stride, (const void*)m_offset);
		}

		GLint	m_elementNum;
		GLenum	m_elementType;
		GLsizei	m_stride;
		size_t	m_offset;
	};

	template <typename T>
	VertexBinder MakeVertexBinder()
	{
		return VertexBinder(
			GLElementTraits<T>::GetNum(),
			GLElementTraits<T>::GetType(),
			sizeof(T),
			0
		);
	}

	class MeshBuilder
	{
	public:
		struct Polygon
		{
			Polygon()
			{
				m_indices[0] = -1;
				m_indices[1] = -1;
				m_indices[2] = -1;
			}

			Polygon(int vi0, int vi1, int vi2)
			{
				m_indices[0] = vi0;
				m_indices[1] = vi1;
				m_indices[2] = vi2;
			}

			int m_indices[3];
		};

		struct Face
		{
			Face() = default;
			Face(size_t polygon, int material)
				: m_polygon(polygon)
				, m_material(material)
			{
			}

			size_t	m_polygon;
			int		m_material;
		};

		struct SubMesh
		{
			size_t	m_startIndex;
			size_t	m_numVertices;
			int		m_material;
		};

	private:
		class Component
		{
		public:
			virtual ~Component() {}

			void ResizePolygonSize(size_t polygonSize)
			{
				m_polygons.resize(polygonSize);
			}

			void AddPolygon()
			{
				m_polygons.push_back(Polygon());
			}

			void SetPolygon(size_t polygonID, const Polygon& polygon)
			{
				m_polygons[polygonID] = polygon;
			}

			virtual GLBufferObject CreateVBO(const std::vector<Face>& faces) = 0;

			virtual VertexBinder MakeVertexBinder() = 0;

		protected:
			std::vector<Polygon>	m_polygons;
		};

		template <typename T>
		class ComponentT : public Component
		{
		public:
			std::vector<T>* GetVertices()
			{
				return &m_vertices;
			}

			void SetDefaultValue(const T& defaultValue)
			{
				m_defaultValue = defaultValue;
			}

			GLBufferObject CreateVBO(const std::vector<Face>& faces) override
			{
				std::vector<T> bufferData;
				bufferData.reserve(faces.size() * 3);
				for (const auto& face : faces)
				{
					auto polyID = face.m_polygon;
					const auto& poly = m_polygons[polyID];

					for (int i = 0; i < 3; i++)
					{
						auto idx = poly.m_indices[i];
						if (idx == -1)
						{
							bufferData.push_back(m_defaultValue);
						}
						else
						{
							bufferData.push_back(m_vertices[idx]);
						}
					}
				}

				return saba::CreateVBO(bufferData);
			}

			VertexBinder MakeVertexBinder() override
			{
				return saba::MakeVertexBinder<T>();
			}

		private:
			T				m_defaultValue;
			std::vector<T>	m_vertices;
		};

		using ComponentPtr = std::unique_ptr<Component>;

	public:
		MeshBuilder()
			: m_polygonCount(0)
		{
		}

		void Clear()
		{
			m_components.clear();
			m_faces.clear();
			m_polygonCount = 0;
		}

		template <typename T>
		size_t AddComponent(std::vector<T>** outVertices, const T& defaultValue = T(0))
		{
			auto compo = std::make_unique< ComponentT<T> >();
			compo->SetDefaultValue(defaultValue);
			compo->ResizePolygonSize(m_polygonCount);
			*outVertices = compo->GetVertices();
			size_t retIdx = m_components.size();
			m_components.emplace_back(std::move(compo));
			return retIdx;
		}

		size_t AddPolygon()
		{
			for (auto& compo : m_components)
			{
				compo->AddPolygon();
			}

			Face face(m_polygonCount, -1);
			m_faces.push_back(face);

			auto ret = m_polygonCount;
			m_polygonCount++;
			return ret;
		}

		void SetPolygon(size_t polyID, size_t compoID, const Polygon& polygon)
		{
			m_components[compoID]->SetPolygon(polyID, polygon);
		}

		void SetPolygon(size_t polyID, size_t compoID, int vi0, int vi1, int vi2)
		{
			SetPolygon(polyID, compoID, Polygon(vi0, vi1, vi2));
		}

		void SetFaceMaterial(size_t polygonID, int materialID)
		{
			m_faces[polygonID].m_material = materialID;
		}

		void SortFaces()
		{
			std::sort(
				m_faces.begin(),
				m_faces.end(),
				[](const Face& a, const Face& b) {return a.m_material < b.m_material;}
			);
		}

		GLBufferObject CreateVBO(size_t compoID)
		{
			return m_components[compoID]->CreateVBO(m_faces);
		}

		VertexBinder MakeVertexBinder(size_t compoID)
		{
			return m_components[compoID]->MakeVertexBinder();
		}

		void MakeSubMeshList(std::vector<SubMesh>* subMeshes)
		{
			if (subMeshes == nullptr)
			{
				return;
			}

			if (m_faces.empty())
			{
				return;
			}

			SubMesh subMesh;
			subMesh.m_startIndex = 0;
			subMesh.m_numVertices = 0;
			subMesh.m_material = m_faces[0].m_material;
			for (const auto& face : m_faces)
			{
				if (face.m_material == subMesh.m_material)
				{
					subMesh.m_numVertices += 3;
				}
				else
				{
					subMeshes->push_back(subMesh);

					subMesh.m_material = face.m_material;
					subMesh.m_startIndex += subMesh.m_numVertices;
					subMesh.m_numVertices = 3;
				}
			}
			subMeshes->push_back(subMesh);
		}

		size_t GetComponentCount() const { return m_components.size(); }
		size_t GetPolygonCount() const { return m_polygonCount; }

	private:
		std::vector<ComponentPtr>	m_components;
		size_t						m_polygonCount;
		std::vector<Face>			m_faces;
	};
}

#endif // !GLVERTEXUTIL_H_

