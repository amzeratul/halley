#pragma once

namespace Halley
{
	class Shader;
	class Texture;

	class Material
	{
	public:
		Material(shared_ptr<Shader> shader);

		void setTexture(shared_ptr<Texture> texture);
	};
}
