#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

namespace eve
{
	// from: https://stackoverflow.com/a/57595105
	template <typename T, typename... Rest>
	void hash_combine(std::size_t& seed, const T& v, const Rest&... rest)
	{
		seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		(hash_combine(seed, rest), ...);
	}

}

namespace std // Inject hash for B into std::
{
	using namespace eve;

    template<> struct hash<glm::ivec3>
    {
        std::size_t operator()(glm::ivec3 const& vec) const noexcept
        {
            std::size_t h = 0;
            eve::hash_combine(h, vec.x, vec.y, vec.z);
            return h;
        }
    };
}

#define MAKE_HASHABLE(type, ...) \
    namespace std {\
        template<> struct hash<glm::ivec3> {\
            std::size_t operator()(const glm::ivec3 &t) const {\
                std::size_t ret = 0;\
                hash_combine(ret, __VA_ARGS__);\
                return ret;\
            }\
        };\
    }