#include <sigma/context.hpp>
#include <sigma/graphics/material.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

void bake_material(std::shared_ptr<sigma::context> context, const boost::filesystem::path& source_directory, const boost::filesystem::path& source_path)
{
    auto key = sigma::filesystem::make_relative(source_directory, source_path).replace_extension("");
    auto material_cache = context->cache<sigma::graphics::material>();
    auto buffer_cache = context->cache<sigma::graphics::buffer>();

    nlohmann::json j_material;
    std::ifstream file(source_path);
    file >> j_material;

    auto material = std::make_shared<sigma::graphics::material>(context, key);
    from_json(j_material, *material);

    for (auto buffer : material->buffers()) {
        if (buffer)
            buffer_cache->insert(buffer->key(), buffer, true);
    }

    material_cache->insert(key, material, true);
}
