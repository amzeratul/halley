#include "painter_opengl.h"
#include "halley/core/graphics/material/material_definition.h"
#include <gsl/gsl_assert>
#include "shader_opengl.h"
#include "constant_buffer_opengl.h"
#include "render_target_opengl.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "texture_opengl.h"

using namespace Halley;

PainterOpenGL::PainterOpenGL(VideoAPI& video, Resources& resources)
	: Painter(video, resources)
{}

PainterOpenGL::~PainterOpenGL()
{
#ifdef WITH_OPENGL
	if (vao != 0) {
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
#endif
}

void PainterOpenGL::doStartRender()
{
	glCheckError();

	if (!glUtils) {
		glUtils = std::make_unique<GLUtils>();
	}
	
	glUtils->resetTextureUnits();
	glUtils->setScissor(Rect4i(), false);

	vertexBuffer.init(GL_ARRAY_BUFFER);
	elementBuffer.init(GL_ELEMENT_ARRAY_BUFFER);
	stdQuadElementBuffer.init(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

#ifdef WITH_OPENGL
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
		glCheckError();
		glBindVertexArray(vao);
		glCheckError();
		vertexBuffer.bind();
		elementBuffer.bind();
	}

	glBindVertexArray(vao);
#endif
}

void PainterOpenGL::doEndRender()
{
#ifdef WITH_OPENGL
	glBindVertexArray(0);
#endif
	glCheckError();
}

void PainterOpenGL::doClear(std::optional<Colour> colour, std::optional<float> depth, std::optional<uint8_t> stencil)
{
	glUtils->setScissor(Rect4i(), false);

	glCheckError();

	auto renderTarget = dynamic_cast<const IRenderTargetOpenGL*>(tryGetActiveRenderTarget());

	const auto col = colour.value_or(Colour());

	if (!renderTarget || renderTarget->isScreenRenderTarget()) {
		glClearColor(col.r, col.g, col.b, col.a);

		glDepthMask(GL_TRUE);
		glClearDepth(depth.value_or(1.0f));
		glClearStencil(stencil.value_or(0));

		GLbitfield mask = 0;
		if (colour) {
			mask |= GL_COLOR_BUFFER_BIT;
		}
		if (depth) {
			mask |= GL_DEPTH_BUFFER_BIT;
		}
		if (stencil) {
			mask |= GL_STENCIL_BUFFER_BIT;
		}

		glClear(mask);
		glCheckError();
	} else {
		glClearBufferfv(GL_COLOR, 0, &col.r);
		glCheckError();

		if (depth) {
			glDepthMask(GL_TRUE);
			if (stencil) {
				glClearBufferfi(GL_DEPTH_STENCIL, 0, depth.value(), stencil.value());
			} else {
				const float dv = depth.value();
				glClearBufferfv(GL_DEPTH, 0, &dv);
			}
			glCheckError();
		}
	}
}

void PainterOpenGL::setMaterialPass(const Material& material, int passNumber)
{
	auto& pass = material.getDefinition().getPass(passNumber);

	glUtils->setScissor(clipping.value_or(Rect4i()), clipping.has_value());

	// Set blend and shader
	glUtils->setBlendType(pass.getBlend());
	glUtils->setDepthStencil(material.getDepthStencil(passNumber));

	ShaderOpenGL& shader = static_cast<ShaderOpenGL&>(pass.getShader());
	shader.bind();

	// Bind constant buffer
	// TODO: move this logic to Painter?
	for (size_t i = 0; i < material.getDataBlocks().size(); ++i) {
		const auto& dataBlock = material.getDataBlocks()[i];
		const auto& dataBlockDef = material.getDefinition().getUniformBlocks()[i];
		int address = dataBlockDef.getAddress(passNumber, ShaderType::Combined);
		if (address == -1) {
			address = dataBlock.getBindPoint();
		}
		shader.setUniformBlockBinding(address, dataBlock.getBindPoint());
	}

	// Bind textures
	// TODO: move this logic to Painter?
	int textureUnit = 0;
	for (auto& tex: material.getDefinition().getTextures()) {
		int location = tex.getAddress(passNumber, ShaderType::Combined);
		if (location == -1) {
			location = textureUnit;
		}
		auto texture = std::static_pointer_cast<const TextureOpenGL>(material.getTexture(textureUnit));
		if (!texture) {
			throw Exception("Error binding texture to texture unit #" + toString(textureUnit) + " with material \"" + material.getDefinition().getName() + "\": texture is null.", HalleyExceptions::VideoPlugin);					
		} else {
			glUniform1i(location, textureUnit);
			texture->bind(textureUnit);
		}
		++textureUnit;
	}
}

void PainterOpenGL::setMaterialData(const Material& material)
{
	for (auto& dataBlock: material.getDataBlocks()) {
		if (dataBlock.getType() != MaterialDataBlockType::SharedExternal) {
			static_cast<ConstantBufferOpenGL&>(getConstantBuffer(dataBlock)).bind(dataBlock.getBindPoint());
		}
	}
}

void PainterOpenGL::setClip(Rect4i clip, bool enable)
{
	clipping = enable ? clip : std::optional<Rect4i>();
}

void PainterOpenGL::setViewPort(Rect4i rect)
{
	glUtils->setViewPort(rect);
}

void PainterOpenGL::onUpdateProjection(Material& material, bool hashChanged)
{
	if (hashChanged) {
		setMaterialData(material);
	}
}

void PainterOpenGL::setVertices(const MaterialDefinition& material, size_t numVertices, const void* vertexData, size_t numIndices, const IndexType* indices, bool standardQuadsOnly)
{
	Expects(numVertices > 0);
	Expects(numIndices >= numVertices);
	Expects(vertexData);
	Expects(indices);

	// Load indices into VBO
	if (standardQuadsOnly) {
		if (stdQuadElementBuffer.getSize() < numIndices * sizeof(IndexType)) {
			size_t indicesToAllocate = nextPowerOf2(numIndices);
			Vector<IndexType> tmp(indicesToAllocate);
			generateQuadIndices(0, indicesToAllocate / 6, tmp.data());
			stdQuadElementBuffer.setData(gsl::as_bytes(gsl::span<IndexType>(tmp)));
		} else {
			stdQuadElementBuffer.bind();
		}
	} else {
		elementBuffer.setData(gsl::as_bytes(gsl::span<const IndexType>(indices, numIndices)));
	}

	// Load vertices into VBO
	size_t bytesSize = numVertices * material.getVertexStride();
	vertexBuffer.setData(gsl::as_bytes(gsl::span<const char>(static_cast<const char*>(vertexData), bytesSize)));

	// Set attributes
	setupVertexAttributes(material);
}

void PainterOpenGL::setupVertexAttributes(const MaterialDefinition& material)
{
    uint32_t unusedLocations = 0xffff;

	// Set vertex attribute pointers in VBO
	size_t vertexStride = material.getVertexStride();
	for (auto& attribute : material.getAttributes()) {
		int count = 0;
		int type = 0;
		switch (attribute.type) {
		case ShaderParameterType::Float:
			count = 1;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Float2:
			count = 2;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Float3:
			count = 3;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Float4:
			count = 4;
			type = GL_FLOAT;
			break;
		case ShaderParameterType::Int:
			count = 1;
			type = GL_INT;
			break;
		case ShaderParameterType::Int2:
			count = 2;
			type = GL_INT;
			break;
		case ShaderParameterType::Int3:
			count = 3;
			type = GL_INT;
			break;
		case ShaderParameterType::Int4:
			count = 4;
			type = GL_INT;
			break;
		default:
			break;
		}
		glEnableVertexAttribArray(attribute.location);
		size_t offset = attribute.offset;
		glVertexAttribPointer(attribute.location, count, type, GL_FALSE, GLsizei(vertexStride), reinterpret_cast<GLvoid*>(offset));
		glCheckError();

        Ensures(attribute.location < 16);
        uint32_t mask = 1u << attribute.location;
        Ensures((unusedLocations & mask) != 0);
        unusedLocations &= ~mask;
	}

    // Disable unused locations.
    int location = 0;
    while (unusedLocations != 0) {
        if ((unusedLocations & 1) != 0) {
            glDisableVertexAttribArray(location);
        }
        unusedLocations >>= 1;
        location++;
    }
    glCheckError();
}

void PainterOpenGL::drawTriangles(size_t numIndices)
{
	Expects(numIndices > 0);
	Expects(numIndices % 3 == 0);

	glDrawElements(GL_TRIANGLES, int(numIndices), GL_UNSIGNED_SHORT, nullptr);
	glCheckError();
}
