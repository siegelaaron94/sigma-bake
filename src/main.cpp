#include <sigma/context.hpp>
#include <sigma/util/filesystem.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

void bake_texture(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path);
void bake_shader(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path);
void bake_material(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path);
void bake_mesh(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path);

int main(int argc, char* argv[])
{
    std::unordered_map<std::string, void (*)(std::shared_ptr<sigma::context>, const std::filesystem::path&, const std::filesystem::path&)> bakers = {
        // Textures
        { ".tiff", bake_texture },
        { ".tif", bake_texture },
        { ".jpg", bake_texture },
        { ".jpeg", bake_texture },
        { ".jpe", bake_texture },
        { ".jif", bake_texture },
        { ".jfif", bake_texture },
        { ".jfi", bake_texture },
        { ".png", bake_texture },
        { ".hdr", bake_texture },

        // Shaders
        { ".vert_spv", bake_shader },
        { ".tesc_spv", bake_shader },
        { ".tese_spv", bake_shader },
        { ".geom_spv", bake_shader },
        { ".frag_spv", bake_shader },

        // Materials
        { ".smat", bake_material },

        // Static Meshes
        { ".3ds", bake_mesh },
        { ".dae", bake_mesh },
        { ".fbx", bake_mesh },
        { ".ifc-step", bake_mesh },
        { ".ase", bake_mesh },
        { ".dxf", bake_mesh },
        { ".hmp", bake_mesh },
        { ".md2", bake_mesh },
        { ".md3", bake_mesh },
        { ".md5", bake_mesh },
        { ".mdc", bake_mesh },
        { ".mdl", bake_mesh },
        { ".nff", bake_mesh },
        { ".ply", bake_mesh },
        { ".stl", bake_mesh },
        { ".x", bake_mesh },
        { ".obj", bake_mesh },
        { ".opengex", bake_mesh },
        { ".smd", bake_mesh },
        { ".lwo", bake_mesh },
        { ".lxo", bake_mesh },
        { ".lws", bake_mesh },
        { ".ter", bake_mesh },
        { ".ac3d", bake_mesh },
        { ".ms3d", bake_mesh },
        { ".cob", bake_mesh },
        { ".q3bsp", bake_mesh },
        { ".xgl", bake_mesh },
        { ".csm", bake_mesh },
        { ".bvh", bake_mesh },
        { ".b3d", bake_mesh },
        { ".ndo", bake_mesh },
        { ".q3d", bake_mesh },
        { ".gltf", bake_mesh },
        { ".3mf", bake_mesh },
        { ".blend", bake_mesh }
    };

    auto cache_dir = std::filesystem::current_path();
    auto source_directory = cache_dir;
    std::vector<std::filesystem::path> source_files;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" || arg == "--output") {
            if (i < argc) {
                cache_dir = argv[++i];
            } else {
                std::cerr << "missing --output value!";
                return -1;
            }
        } else {
            source_files.push_back(argv[i]);
        }
    }

    std::filesystem::create_directories(cache_dir);
    auto context = std::make_shared<sigma::context>(cache_dir);

    for (const auto& src : source_files) {
        auto src_path = std::filesystem::absolute(src);
        if (sigma::filesystem::contains_file(source_directory, src_path) && std::filesystem::exists(src_path)) {
            auto ext = src_path.extension().string();
            if (bakers.count(ext)) {
                bakers[ext](context, source_directory, src_path);
            } else {
                std::cerr << "sigma-bake: error: File '" << src << "' is not supported'!\n";
                return -1;
            }
        } else {
            std::cerr << "sigma-bake: error: File '" << src << "' is not contained in '" << std::filesystem::current_path() << "'!\n";
            return -1;
        }
    }

    return 0;
}
