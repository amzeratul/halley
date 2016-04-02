#pragma once

namespace Halley
{
	class Shader;
	class Texture;

	class Material
	{
	public:
		Material(std::shared_ptr<Shader> shader);

		void setTexture(std::shared_ptr<Texture> texture);
	};
}
