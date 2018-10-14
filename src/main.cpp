#include <sigma/context.hpp>
#include <sigma/util/filesystem.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace po = boost::program_options;

void bake_texture(std::shared_ptr<sigma::context> context, const boost::filesystem::path& source_directory, const boost::filesystem::path& source_path);
void bake_shader(std::shared_ptr<sigma::context> context, const boost::filesystem::path& source_directory, const boost::filesystem::path& source_path);
void bake_material(std::shared_ptr<sigma::context> context, const boost::filesystem::path& source_directory, const boost::filesystem::path& source_path);

int main(int argc, char* argv[])
{
    po::options_description global_options("Options");
    // clang-format off
    global_options.add_options()("help,h", "Show this help message")
    ("output,o", po::value<std::string>()->default_value((boost::filesystem::current_path()).string()), "output directory")
    ("input-files", po::value<std::vector<std::string>>(), "input resource files");
    // clang-format on

    po::positional_options_description positional_options;
    positional_options.add("input-files", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(global_options).positional(positional_options).run(), vm);

    if (vm.count("help")) {
        std::cout << global_options << '\n';
        return 0;
    }

    if (vm.count("input-files") <= 0) {
        std::cerr << "sigma-bake: fatal error: no input files.\n";
        return -1;
    }

    std::unordered_map<std::string, void (*)(std::shared_ptr<sigma::context>, const boost::filesystem::path&, const boost::filesystem::path&)> bakers = {
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
    };

    boost::filesystem::path cache_dir { vm["output"].as<std::string>() };
    boost::filesystem::create_directories(cache_dir);

    auto context = std::make_shared<sigma::context>(cache_dir);

    auto source_directory = boost::filesystem::current_path();
    for (const auto& src : vm["input-files"].as<std::vector<std::string>>()) {
        auto src_path = boost::filesystem::absolute(src);
        if (sigma::filesystem::contains_file(source_directory, src_path) && boost::filesystem::exists(src_path)) {
            auto ext = src_path.extension().string();
            if (bakers.count(ext)) {
                bakers[ext](context, source_directory, src_path);
            } else {
                std::cerr << "sigma-bake: error: File '" << src << "' is not supported'!\n";
                return -1;
            }
        } else {
            std::cerr << "sigma-bake: error: File '" << src << "' is not contained in '" << boost::filesystem::current_path() << "'!\n";
            return -1;
        }
    }

    return 0;
}