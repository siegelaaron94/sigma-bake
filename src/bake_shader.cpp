#include <sigma/context.hpp>
#include <sigma/graphics/shader.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>

#include <nlohmann/json.hpp>

#include <boost/filesystem.hpp>

#include <iostream>

void bake_shader(std::shared_ptr<sigma::context> context, const boost::filesystem::path& source_directory, const boost::filesystem::path& source_path)
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
