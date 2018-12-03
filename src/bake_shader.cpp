#include <sigma/context.hpp>
#include <sigma/graphics/shader.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>

namespace sigma {
namespace graphics {
    void from_json(const nlohmann::json& j, buffer_member& member)
    {
        std::string type = j["type"].get<std::string>();
        auto array_it = j.find("array");

        member.name = j["name"].get<std::string>();
        member.offset = j["offset"].get<size_t>();
        member.is_array = array_it != j.end();

        if (member.is_array) {
            if (array_it->size() != 1)
                throw std::runtime_error("Multidimensional arrays are not supported!");
            member.count = (*array_it)[0].get<size_t>();
        }

        if (type == "float") {
            member.type = buffer_type::FLOAT;
            if (member.is_array)
                member.stride = sizeof(glm::vec4);
        } else if (type == "vec2") {
            member.type = buffer_type::VEC2;
            if (member.is_array)
                member.stride = sizeof(glm::vec4);
        } else if (type == "vec3") {
            member.type = buffer_type::VEC3;
            if (member.is_array)
                member.stride = sizeof(glm::vec4);
        } else if (type == "vec4") {
            member.type = buffer_type::VEC4;
            if (member.is_array)
                member.stride = sizeof(glm::vec4);
        } else if (type == "mat3") {
            member.type = buffer_type::MAT3x3;
            if (member.is_array)
                member.stride = 3 * sizeof(glm::vec4);
        } else if (type == "mat4") {
            member.type = buffer_type::MAT4x4;
            if (member.is_array)
                member.stride = sizeof(glm::mat4);
        } else {
            throw std::runtime_error("Buffer Type: " + type + " not supported!");
        }
    }

    void from_json(const nlohmann::json& j, buffer_schema& schema)
    {
        for (const auto& member_j : j["members"]) {
            buffer_member member = member_j;
            schema.members[member.name] = member;
        }
    }

    void from_json(const nlohmann::json& j, shader_schema& schema)
    {
        auto ubos_it = j.find("ubos");
        if (ubos_it != j.end()) {
            for (const auto& j_ubo : *ubos_it) {
                std::string type_name = j_ubo["type"].get<std::string>();
                buffer_schema buffer_schema;
                buffer_schema = j["types"][type_name];
                buffer_schema.size = j_ubo["block_size"].get<size_t>();
                buffer_schema.descriptor_set = j_ubo["set"].get<size_t>();
                buffer_schema.binding_point = j_ubo["binding"].get<size_t>();
                buffer_schema.name = j_ubo["name"].get<std::string>();
                schema.buffers.push_back(buffer_schema);
            }
        }

        auto textures_it = j.find("textures");
        if (textures_it != j.end()) {
            for (const auto& j_texture : *textures_it) {
                std::string type = j_texture["type"].get<std::string>();
                texture_schema tex_schema;
                tex_schema.name = j_texture["name"].get<std::string>();
                tex_schema.descriptor_set = j_texture["set"].get<size_t>();
                tex_schema.binding_point = j_texture["binding"].get<size_t>();
                if (type == "sampler2D")
                    tex_schema.type = texture_sampler_type::SAMPLER2D;
                else if (type == "sampler2DArrayShadow")
                    tex_schema.type = texture_sampler_type::SAMPLER2D_ARRAY_SHADOW;
                else
                    throw std::runtime_error("Sampler Type: " + type + " is not supported!");
                schema.textures.push_back(tex_schema);
            }
        }
    }
}
}

void bake_shader(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path)
{
    static const std::unordered_map<std::string, sigma::graphics::shader_type> source_types = {
        { ".vert_spv", { sigma::graphics::shader_type::vertex } },
        { ".tesc_spv", { sigma::graphics::shader_type::tessellation_control } },
        { ".tese_spv", { sigma::graphics::shader_type::tessellation_evaluation } },
        { ".geom_spv", { sigma::graphics::shader_type::geometry } },
        { ".frag_spv", { sigma::graphics::shader_type::fragment } }
    };

    static const std::unordered_map<sigma::graphics::shader_type, std::string> type_name_map {
        { sigma::graphics::shader_type::vertex, "vertex" },
        { sigma::graphics::shader_type::tessellation_control, "tessellation_control" },
        { sigma::graphics::shader_type::tessellation_evaluation, "tessellation_evaluation" },
        { sigma::graphics::shader_type::geometry, "geometry" },
        { sigma::graphics::shader_type::fragment, "fragment" },
        { sigma::graphics::shader_type::header, "header" }
    };

    auto source_type = source_types.at(source_path.extension().string());
    auto key = sigma::filesystem::make_relative(source_directory, source_path).replace_extension("");

    // Load the SPIRV source code
    std::ifstream source { source_path.string() };
    std::vector<unsigned char> spirv;
    spirv.insert(spirv.end(),
        std::istreambuf_iterator<char> { source.rdbuf() },
        std::istreambuf_iterator<char> {});

    // Read the SPIRV reflection data
    sigma::graphics::shader_schema schema;
    auto reflect_path = source_path.string() + ".json";
    nlohmann::json j_reflection;
    std::ifstream file(reflect_path);
    file >> j_reflection;
    schema = j_reflection;

    auto cache = context->cache<sigma::graphics::shader>();
    auto shader = std::make_shared<sigma::graphics::shader>(context, key);
    shader->add_source(source_type, std::move(spirv), std::move(schema));
    cache->insert(key, shader, true);
}
