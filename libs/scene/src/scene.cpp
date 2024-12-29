#include "scene/scene.hpp"


namespace rn::scene
{
    RN_DEFINE_MEMORY_CATEGORY(Scene);

    Scene::~Scene()
    {
        for (const auto& p : _archetypes)
        {
            TrackedDelete(p.second.builder);
        }
    }
}