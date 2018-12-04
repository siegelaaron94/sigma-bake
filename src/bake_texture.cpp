#include <sigma/context.hpp>
#include <sigma/graphics/texture.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>
#include <sigma/util/string.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <nlohmann/json.hpp>

#include <filesystem>

namespace sigma {
namespace graphics {
    struct texture_settings {
        texture_format format = texture_format::RGB8;
        texture_filter minification = texture_filter::LINEAR;
        texture_filter magnification = texture_filter::LINEAR;
        texture_filter mipmap = texture_filter::LINEAR;
    };

    void from_json(const nlohmann::json& j, texture_filter& flt)
    {
        static std::map<std::string, texture_filter> filter_map = {
            { "NEAREST", texture_filter::NEAREST },
            { "LINEAR", texture_filter::LINEAR },
            { "NONE", texture_filter::NONE }
        };

        auto str_val = sigma::util::to_upper_copy(j.get<std::string>());
        auto it = filter_map.find(str_val);

        if (it != filter_map.end())
            flt = it->second;
        else
            flt = texture_filter::LINEAR;
    }

    void from_json(const nlohmann::json& j, texture_format& fmt)
    {
        static std::map<std::string, texture_format> format_map = {
            { "RGB8", texture_format::RGB8 },
            { "RGBA8", texture_format::RGBA8 },
            { "RGB16F", texture_format::RGB16F },
            { "RGBA16F", texture_format::RGBA16F },
            { "RGB32F", texture_format::RGB32F },
            { "DEPTH32F_STENCIL8", texture_format::DEPTH32F_STENCIL8 }
        };

        auto str_val = sigma::util::to_upper_copy(j.get<std::string>());
        auto it = format_map.find(str_val);

        if (it != format_map.end())
            fmt = it->second;
        else
            fmt = texture_format::RGB8;
    }

    void from_json(const nlohmann::json& j, texture_settings& settings)
    {
        auto format_j = j.find("format");
        if (format_j != j.end())
            settings.format = *format_j;

        auto filter_j = j.find("filter");
        if (filter_j != j.end()) {
            auto min_j = filter_j->find("minification");
            if (min_j != filter_j->end())
                settings.minification = *min_j;

            auto mag_j = filter_j->find("magnification");
            if (mag_j != filter_j->end())
                settings.magnification = *mag_j;

            auto mip_j = filter_j->find("mipmap");
            if (mip_j != filter_j->end())
                settings.mipmap = *mip_j;
        }
    }
}
}

template <class Image>
void load_texture(const std::filesystem::path& source_path, Image& image)
{
    auto file_path_string = source_path.string();
    int width, height, bbp;
    auto pixels = (typename Image::pixel_type*)stbi_load(file_path_string.c_str(), &width, &height, &bbp, sigma::graphics::channel_count_v<Image>);
    image.size = { width, height };
    image.pixels.resize(width * height);
    std::copy(pixels, pixels + (width * height), image.pixels.begin());
    stbi_image_free((void*)pixels);
}

void load_texture(const std::filesystem::path& source_path, sigma::graphics::image_t<sigma::graphics::rgb32f_pixel_t>& image)
{
    auto file_path_string = source_path.string();
    int width, height, bbp;
    auto pixels = (sigma::graphics::rgb32f_pixel_t*)stbi_loadf(file_path_string.c_str(), &width, &height, &bbp, 3);
    image.size = { width, height };
    image.pixels.resize(width * height);
    std::copy(pixels, pixels + (width * height), image.pixels.begin());
    stbi_image_free((void*)pixels);
}

void bake_texture(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path)
{
    auto key = sigma::filesystem::make_relative(source_directory, source_path).replace_extension("");
    auto settings_path = source_path.parent_path() / (source_path.stem().string() + ".stex");

    auto cache = context->cache<sigma::graphics::texture>();

    sigma::graphics::texture_settings settings;
    if (std::filesystem::exists(settings_path)) {
        nlohmann::json j_settings;
        std::ifstream file(settings_path.string());
        file >> j_settings;
        settings = j_settings;
    }

    std::shared_ptr<sigma::graphics::texture> texture;
    switch (settings.format) {
    case sigma::graphics::texture_format::RGB8: {
        sigma::graphics::image_t<sigma::graphics::rgb8_pixel_t> image;
        load_texture(source_path, image);
        texture = std::make_shared<sigma::graphics::texture>(context, key, image, settings.minification, settings.magnification, settings.mipmap);
        break;
    }
    case sigma::graphics::texture_format::RGBA8: {
        sigma::graphics::image_t<sigma::graphics::rgba8_pixel_t> image;
        load_texture(source_path, image);
        texture = std::make_shared<sigma::graphics::texture>(context, key, image, settings.minification, settings.magnification, settings.mipmap);
        break;
    }
    case sigma::graphics::texture_format::RGB32F: {
        sigma::graphics::image_t<sigma::graphics::rgb32f_pixel_t> image;
        load_texture(source_path, image);
        texture = std::make_shared<sigma::graphics::texture>(context, key, image, settings.minification, settings.magnification, settings.mipmap);
        break;
    }
    }

    cache->insert(key, texture, true);
}
