#include "DebugTexDisplayer.h"
#include "Model.h"
#include "Shader.h"

namespace e186
{

	DebugTexDisplayer::DebugTexDisplayer() 
	{
		m_texture_configs.reserve(kVecSize);
		auto& twBarManager = Engine::current->tweak_bar_manager();
		m_tweak_bar = twBarManager.create_new_tweak_bar("Textures");

		m_quad = Model::LoadFromFile("assets/models/quad01.obj", glm::mat4(1.0f), MOLF_default);
		assert(m_quad);
		m_quad->GenerateVAOsWithVertexAttribConfig(VertexAttribData_Position | VertexAttribData_Tex2D);

		m_shader.AddVertexShaderSourceFromFile("assets/shaders/texture.vert")
				.AddFragmentShaderSourceFromFile("assets/shaders/texture.frag", { std::make_tuple(0, "fragColor") })
				.Build();

		m_sampler_loc = m_shader.GetUniformLocation("sampler");
		m_color_loc = m_shader.GetUniformLocation("color");
		m_pvmt_matrix_loc = m_shader.GetUniformLocation("pvmtMatrix");
	}

	DebugTexDisplayer::~DebugTexDisplayer()
	{
		auto& twBarManager = Engine::current->tweak_bar_manager();
		twBarManager.destroy_tweak_bar(m_tweak_bar);
	}

	DebugTexDisplayer::DbgTexConfig* DebugTexDisplayer::Add(GLsizei width, GLsizei height, GLenum texTarget, GLuint glHandle, std::string name)
	{
		if (m_texture_configs.size() >= kVecSize)
		{
			log_error("Max. number of debug textures reached");
			return nullptr;
		}

		m_texture_configs.emplace_back(DebugTexDisplayer::DbgTexConfig{
			true,	   // bool m_enabled;
			false,	   // bool m_border;
			width,	   // GLsizei m_width;
			height,	   // GLsizei m_height;
			.5f,	   // float m_scale;
			true,	   // bool m_scale_relative_to_window_height;
			0,		   // GLint m_offset_x;
			0,		   // GLint m_offset_y;
			texTarget, // GLenum m_texture_target;
			glHandle   // GLuint m_gl_handle;
		});
		auto& element = m_texture_configs.back();
		auto n = m_texture_configs.size();
		auto num = "Tex #" + std::to_string(n);
		auto groupAssignment = " group='" + name + "' ";
		TwAddVarRW(m_tweak_bar, (num + " enbld").c_str(), TW_TYPE_BOOL32, &element.m_enabled, groupAssignment.c_str());
		TwAddVarRW(m_tweak_bar, (num + " off-x").c_str(), TW_TYPE_INT32, &element.m_offset_x, groupAssignment.c_str());
		TwAddVarRW(m_tweak_bar, (num + " off-y").c_str(), TW_TYPE_INT32, &element.m_offset_y, groupAssignment.c_str());
		TwAddVarRW(m_tweak_bar, (num + " scale").c_str(), TW_TYPE_FLOAT, &element.m_scale, (" min=0.01 max=2.0 step=0.01 " + groupAssignment).c_str());
		TwAddVarRW(m_tweak_bar, (num + " relsc").c_str(), TW_TYPE_BOOL32, &element.m_scale_relative_to_window_height, groupAssignment.c_str());
		return &element;
	}

	DebugTexDisplayer::DbgTexConfig* DebugTexDisplayer::Add(const TexInfo& data, std::string name)
	{	
		return Add(256, 256, data.target(), data.handle(), std::move(name));
	}

	DebugTexDisplayer::DbgTexConfig* DebugTexDisplayer::Add(const Tex2D& data, std::string name)
	{
		return Add(data.width(), data.height(), data.target(), data.handle(), std::move(name));
	}

	DebugTexDisplayer::DbgTexConfig* DebugTexDisplayer::Add(const FrameBufferObject& data, GLenum attachment, std::string name)
	{
		auto& texInfo = data.GetAttachedTexture(attachment);
		return Add(data.width(), data.height(), texInfo.target(), texInfo.handle(), std::move(name));
	}

	void DebugTexDisplayer::Render()
	{
		const auto wnd_width = Engine::current->window_width();
		const auto wnd_height = Engine::current->window_height();
		
		Camera orthoCam; orthoCam.SetOrthogonalProjection(0.0f, wnd_width, 0.0f, wnd_height, 0.0f, 100.0f);
		auto pM = orthoCam.projection_matrix();

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		m_shader.Use();
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(m_sampler_loc, 0); // 0 => GL_TEXTURE0
		glUniform4f(m_color_loc, 1.0f, 1.0f, 1.0f, 1.0f);

		for (auto& tc : m_texture_configs)
		{
			if (!tc.m_enabled)
				continue;

			glBindTexture(tc.m_texture_target, tc.m_gl_handle);

			float sc_w, sc_h;
			if (tc.m_scale_relative_to_window_height)
			{
				sc_h = tc.m_scale * wnd_height;
				sc_w = sc_h * tc.m_width / tc.m_height;
			}
			else
			{
				sc_h = tc.m_height * tc.m_scale;
				sc_w = tc.m_width * tc.m_scale;
			}

			if (tc.m_border)
			{
				// make it black
				glUniform4f(m_color_loc, 0.0f, 0.0f, 0.0f, 1.0f);

				glm::mat4 screenMb = pM
					* glm::translate(glm::vec3(tc.m_offset_x - 1.0f, tc.m_offset_y - 1.0f, 0.0f))
					* glm::scale(glm::vec3(sc_w + 2.0f, sc_h + 2.0f, 0.0f));
				glUniformMatrix4fv(m_pvmt_matrix_loc, 1, GL_FALSE, glm::value_ptr(screenMb));
				m_quad->RenderForVertexAttribConfig(VertexAttribData_Position | VertexAttribData_Tex2D, GL_TRIANGLES);

				// change back to actual colors
				glUniform4f(m_shader.GetUniformLocation("color"), 1.0f, 1.0f, 1.0f, 1.0f);
			}

			glm::mat4 screenM = pM
				* glm::translate(glm::vec3(tc.m_offset_x, tc.m_offset_y, 0.0f))
				* glm::scale(glm::vec3(sc_w, sc_h, 0.0f));
			glUniformMatrix4fv(m_pvmt_matrix_loc, 1, GL_FALSE, glm::value_ptr(screenM));
			m_quad->RenderForVertexAttribConfig(VertexAttribData_Position | VertexAttribData_Tex2D, GL_TRIANGLES);
		}

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	}
}