#pragma once

#include "eve_model.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace eve
{

	struct TransformComponent
	{
		glm::vec3 translation{};
		glm::vec3 scale{1.f, 1.f, 1.f};
		glm::vec3 rotation{};

		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	class EveGameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, EveGameObject>;

		static EveGameObject createGameObject()
		{
			static id_t currentId = 0;
			return EveGameObject{currentId++};
		}

		EveGameObject(const EveGameObject &) = delete;
		EveGameObject &operator=(const EveGameObject &) = delete;
		EveGameObject(EveGameObject &&) = default;
		EveGameObject &operator=(EveGameObject &&) = default;

		id_t getId() { return id; }

		std::shared_ptr<EveModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};

	private:
		EveGameObject(id_t objId) : id{objId} {}

		id_t id;
	};
}