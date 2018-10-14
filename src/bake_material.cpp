#include "glm_json.hpp"

#include <sigma/context.hpp>
#include <sigma/graphics/material.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <boost/filesystem.hpp>

#include <fstream>
#include <iostream>

namespace sigma {
namespace graphics {

void from_json(const nlohmann::json& j, buffer& b)
{
    const auto& schema = b.schema();
    for (const auto& item : j.items()) {
        const auto& value = item.value();
        const auto& key = item.key();
        auto it = schema.members.find(key);

        if (it == schema.members.end())
            continue;

        const auto& [name, member] = *it;

        switch (member.type) {
        case buffer_type::FLOAT: {
            if (member.is_array) {
                for (size_t i = 0; i < value.size(); ++i) {
                    float v = value[i];
                    b.set(key, i, v);
                }
            } else {
                float v = item.value();
                b.set(key, v);
            }
            break;
        }
        case buffer_type::VEC2: {
            if (member.is_array) {
                for (size_t i = 0; i < value.size(); ++i) {
                    glm::vec2 v = value[i];
                    b.set(key, i, v);
                }
            } else {
                glm::vec2 v = item.value();
                b.set(key, v);
            }
            break;
        }
        case buffer_type::VEC3: {
            if (member.is_array) {
                for (size_t i = 0; i < value.size(); ++i) {
                    glm::vec3 v = value[i];
                    b.set(key, i, v);
                }
            } else {
                glm::vec3 v = item.value();
                b.set(key, v);
            }
            break;
        }
        case buffer_type::VEC4: {
            if (member.is_array) {
                for (size_t i = 0; i < value.size(); ++i) {
                    glm::vec4 v = value[i];
                    b.set(key, i, v);
                }

            } else {
                glm::vec4 v = item.value();
                b.set(key, v);
            }
            break;
        }
        case buffer_type::MAT3x3: {
            if (member.is_array) {
                for (size_t i = 0; i < value.size(); ++i) {
                    glm::mat3 v = value[i];
                    b.set(key, i, v);
                }
            } else {
                glm::mat3 v = item.value();
                b.set(key, v);
            }
            break;
        }
        case buffer_type::MAT4x4: {
            if (member.is_array) {
                for (size_t i = 0; i < value.size(); ++i) {
                    glm::mat4 v = value[i];
                    b.set(key, i, v);
                }
            } else {
                glm::mat4 v = item.value();
                b.set(key, v);
            }
            break;
        }
        }
    }
}

void from_json(const nlohmann::json& j, material& mat)
{
    static const std::set<std::string> shader_keys = {
        "vertex", "tessellation_control", "tessellation_evaluation", "geometry", "fragment"
    };

    if (auto ctx = mat.context().lock()) {
        auto shader_cache = ctx->cache<shader>();
        auto buffer_cache = ctx->cache<buffer>();
        auto texture_cache = ctx->cache<texture>();

        for (const auto& item : j.items()) {
            auto key = item.key();
            if (shader_keys.count(key)) {
                auto shader_key = key / sigma::resource::key_type(item.value().get<std::string>());
                auto shader = shader_cache->get(shader_key);

                const auto& shader_schema = shader->schema();

                // Validate and merge buffer schema
                for (const auto& buff_schema : shader_schema.buffers) {
                    resource::handle_type<buffer> buffer = mat.buffer(buff_schema.binding_point);
                    if (!buffer) {
                        resource::key_type buffer_key = mat.key() / buff_schema.name;
                        buffer = buffer_cache->insert(buffer_key, std::make_shared<graphics::buffer>(mat.context(), buffer_key, buff_schema));
                        mat.set_buffer(buff_schema.binding_point, buffer);
                    } else if (!buffer->merge(buff_schema)) {
                        throw std::runtime_error("Buffer schema miss-match in material: " + mat.key().string());
                    }
                }

                mat.set_shader(shader->type(), shader);
            }
        }

        for (auto& [index, buffer] : mat.buffers()) {
            if (buffer)
                from_json(j, *buffer);
        }

        for (const auto& item : j.items()) {
            const auto& key = item.key();
            const auto& value = item.value();
            if (key == "textures") {
                for (const auto& texture_j : value.items()) {
                    size_t index;
                    if (mat.texture_binding_point(texture_j.key(), index))
                        mat.set_texture(index, texture_cache->get(texture_j.value().get<std::string>()));
                }
            } else if (key == "cubemaps") {
                // TODO: cubemaps
            }
        }
    } else {
        throw std::runtime_error("Could not lock context.");
    }
}

}
}

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
        buffer_cache->write_to_disk(buffer.second->key());
    }

    material_cache->insert(key, material, true);
}
