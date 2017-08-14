//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_GL_GLOBJECT_H_
#define SABA_GL_GLOBJECT_H_

#include <atomic>
#include <GL/gl3w.h>

namespace saba
{

	template <GLenum ShaderType>
	struct GLShaderTraits
	{
		static GLuint Create()
		{
			return glCreateShader(ShaderType);
		}

		static void Destroy(GLuint shader)
		{
			glDeleteShader(shader);
		}
	};

	using GLVertexShaderTraits = GLShaderTraits<GL_VERTEX_SHADER>;
	using GLFragmentShaderTraits = GLShaderTraits<GL_FRAGMENT_SHADER>;
	using GLTessControlShaderTraits = GLShaderTraits<GL_TESS_CONTROL_SHADER>;
	using GLTessEvaluationShaderTraits = GLShaderTraits<GL_TESS_EVALUATION_SHADER>;
	using GLGeometryShaderTraits = GLShaderTraits<GL_GEOMETRY_SHADER>;
	using GLComputeShaderTraits = GLShaderTraits<GL_COMPUTE_SHADER>;

	struct GLProgramTraits
	{
		static GLuint Create()
		{
			return glCreateProgram();
		}

		static void Destroy(GLuint program)
		{
			glDeleteProgram(program);
		}
	};

	struct GLBufferTraits
	{
		static GLuint Create()
		{
			GLuint buf;
			glGenBuffers(1, &buf);
			return buf;
		}

		static void Destroy(GLuint buf)
		{
			glDeleteBuffers(1, &buf);
		}
	};

	struct GLVertexArrayTraits
	{
		static GLuint Create()
		{
			GLuint vertexArray;
			glGenVertexArrays(1, &vertexArray);
			return vertexArray;
		}

		static void Destroy(GLuint vertexArray)
		{
			glDeleteVertexArrays(1, &vertexArray);
		}
	};

	struct GLTextureTraits
	{
		static GLuint Create()
		{
			GLuint tex;
			glGenTextures(1, &tex);
			return tex;
		}

		static void Destroy(GLuint tex)
		{
			glDeleteTextures(1, &tex);
		}
	};

	struct GLFramebufferTraits
	{
		static GLuint Create()
		{
			GLuint fb;
			glGenFramebuffers(1, &fb);
			return fb;
		}

		static void Destroy(GLuint fb)
		{
			glDeleteFramebuffers(1, &fb);
		}
	};

	struct GLRenderbufferTraits
	{
		static GLuint Create()
		{
			GLuint rb;
			glGenRenderbuffers(1, &rb);
			return rb;
		}
		
		static void Destroy(GLuint rb)
		{
			glDeleteRenderbuffers(1, &rb);
		}
	};

	template <typename GLTraits>
	class GLObject
	{
	public:
		using GLObjectType = GLObject<GLTraits>;

		GLObject()
			: m_obj(0)
		{
		}

		explicit GLObject(GLuint obj)
			: m_obj(obj)
		{
		}

		GLObject(GLObjectType&& rhs)
		{
			m_obj = rhs.m_obj;
			rhs.m_obj = 0;
		}

		GLObjectType& operator =(GLObjectType&& rhs)
		{
			m_obj = rhs.m_obj;
			rhs.m_obj = 0;
			return *this;
		}

		GLObject(const GLObjectType& rhs) = delete;
		GLObjectType& operator =(const GLObjectType& rhs) = delete;

		~GLObject()
		{
			Destroy();
		}

		bool Create()
		{
			Destroy();
			m_obj = GLTraits::Create();
			return m_obj != 0;
		}

		void Destroy()
		{
			if (m_obj != 0)
			{
				GLTraits::Destroy(m_obj);
				m_obj = 0;
			}
		}

		void Reset(GLuint obj)
		{
			Destroy();
			m_obj = obj;
		}

		GLuint Release()
		{
			GLuint ret = m_obj;
			m_obj = 0;
			return ret;
		}

		GLuint Get() const
		{
			return m_obj;
		}

		operator GLuint() const { return m_obj; }

	private:
		GLuint	m_obj;
	};


	template <typename GLTraits>
	class GLRef
	{
	private:
		class GLRefCount
		{
		public:
			GLRefCount(GLuint obj)
				: m_counter(1)
				, m_obj(obj)
			{
			}

			GLRefCount(const GLRefCount&) = delete;
			GLRefCount& operator = (const GLRefCount&) = delete;

			void IncRef()
			{
				m_counter.fetch_add(1);
			}

			void DecRef()
			{
				if (--m_counter == 0)
				{
					if (m_obj != 0)
					{
						GLTraits::Destroy(m_obj);
						m_obj = 0;
						delete this;
					}
				}
			}

			GLuint Get() const
			{
				return m_obj;
			}

			operator GLuint() const { return m_obj; }

			int GetCount() const { return m_counter; }

		private:
			std::atomic<int>	m_counter;
			GLuint				m_obj;
		};

	public:
		using Type = GLTraits;
		using RefType = GLRef<GLTraits>;

		GLRef()
			: m_ref(nullptr)
		{
		}

		~GLRef()
		{
			if (m_ref)
			{
				m_ref->DecRef();
			}
		}

		GLRef(GLuint obj)
			: m_ref(nullptr)
		{
			Reset(obj);
		}

		GLRef(RefType&& rhs)
			: m_ref(nullptr)
		{
			Reset(rhs.m_ref);
			rhs.Reset();
		}

		GLRef(GLObject<GLTraits>&& rhs)
			: m_ref(nullptr)
		{
			Reset(rhs.Release());
		}

		RefType& operator =(RefType&& rhs)
		{
			Reset(rhs.m_ref);
			rhs.Reset();
			return *this;
		}

		RefType& operator =(GLObject<GLTraits>&& rhs)
		{
			Reset(rhs.Release());
			return *this;
		}

		GLRef(const RefType& rhs)
			: m_ref(nullptr)
		{
			Reset(const_cast<GLRefCount*>(rhs.m_ref));
		}

		RefType& operator =(const RefType& rhs)
		{
			Reset(const_cast<GLRefCount*>(rhs.m_ref));
			return *this;
		}

		GLuint Get() const
		{
			if (m_ref != nullptr)
			{
				return m_ref->Get();
			}
			return 0;
		}

		operator GLuint() const
		{
			if (m_ref != nullptr)
			{
				return m_ref->Get();
			}
			else
			{
				return 0;
			}
		}

		int GetRefCount() const
		{
			if (m_ref != nullptr)
			{
				return m_ref->GetCount();
			}
			else
			{
				return 0;
			}
		}

		void Release()
		{
			Reset();
		}

	private:
		void Reset()
		{
			if (m_ref != nullptr)
			{
				m_ref->DecRef();
				m_ref = nullptr;
			}
		}

		void Reset(GLuint obj)
		{
			Reset();
			if (obj != 0)
			{
				m_ref = new GLRefCount(obj);
			}
		}

		void Reset(GLRefCount* refobj)
		{
			Reset();
			if (refobj != nullptr)
			{
				m_ref = refobj;
				m_ref->IncRef();
			}
		}
	private:
		GLRefCount*	m_ref;
	};

	template <GLenum ShaderType>
	using GLShaderObject = GLObject< GLShaderTraits<ShaderType> >;

	using GLVertexShaderObject = GLShaderObject<GL_VERTEX_SHADER>;
	using GLFragmentShaderObject = GLShaderObject<GL_FRAGMENT_SHADER>;
	using GLProgramObject = GLObject<GLProgramTraits>;
	using GLBufferObject = GLObject<GLBufferTraits>;
	using GLVertexArrayObject = GLObject<GLVertexArrayTraits>;
	using GLTextureObject = GLObject<GLTextureTraits>;
	using GLFramebufferObject = GLObject<GLFramebufferTraits>;
	using GLRenderbufferObject = GLObject<GLRenderbufferTraits>;

	template <GLenum ShaderType>
	using GLShaderRef = GLRef< GLShaderTraits<ShaderType> >;

	using GLVertexShaderRef = GLShaderRef<GL_VERTEX_SHADER>;
	using GLFragmentShaderRef = GLShaderRef<GL_FRAGMENT_SHADER>;
	using GLProgramRef = GLRef<GLProgramTraits>;
	using GLBufferRef = GLRef<GLBufferTraits>;
	using GLVertexArrayRef = GLRef<GLVertexArrayTraits>;
	using GLTextureRef = GLRef<GLTextureTraits>;
	using GLFramebufferRef = GLRef<GLFramebufferTraits>;
	using GLRenderbufferRef = GLRef<GLRenderbufferTraits>;
}

#endif // !SABA_GL_GLOBJECT_H_
