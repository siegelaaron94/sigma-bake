#include "hdr_io.hpp"

#include <sigma/context.hpp>
#include <sigma/graphics/texture.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <nlohmann/json.hpp>

#include <boost/filesystem.hpp>
#include <boost/gil/image.hpp>

#include <iostream>

using nlohmann::json;

struct texture_settings {
    sigma::graphics::texture_format format = sigma::graphics::texture_format::RGB8;
    sigma::graphics::texture_filter minification = sigma::graphics::texture_filter::LINEAR;
    sigma::graphics::texture_filter magnification = sigma::graphics::texture_filter::LINEAR;
    sigma::graphics::texture_filter mipmap = sigma::graphics::texture_filter::LINEAR;
};

void to_json(json& j, const texture_settings& settings)
{
    j = nlohmann::json {
        { "format", settings.format },
        { "filter", { { "minification", settings.minification }, { "magnification", settings.mipmap }, { "mipmap", settings.mipmap } } }
    };
}

void from_json(const json& j, texture_settings& settings)
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

template <class Image>
void load_texture(const boost::filesystem::path& source_path, Image& image)
{
    auto file_path_string = source_path.string();

    if (source_path.extension() == ".hdr") {
        hdr_read_and_convert_image(file_path_string, image);
    } else {
        int width, height, bbp;
        auto pixels = (boost::gil::rgba8_pixel_t*)stbi_load(file_path_string.c_str(), &width, &height, &bbp, 4);
        auto src_view = boost::gil::interleaved_view(width, height, pixels, width * 4 * sizeof(unsigned char));
        image = Image(width, height);
        boost::gil::copy_pixels(src_view, boost::gil::view(image));
        stbi_image_free((char*)pixels);
    }
}

void bake_texture(std::shared_ptr<sigma::context> context, const boost::filesystem::path& source_directory, const boost::filesystem::path& source_path)
{
    auto key = sigma::filesystem::make_relative(source_directory, source_path).replace_extension("");
    auto settings_path = source_path.parent_path() / (source_path.stem().string() + ".stex");

    auto cache = context->cache<sigma::graphics::texture>();

    texture_settings settings;
    if (boost::filesystem::exists(settings_path)) {
        nlohmann::json j_settings;
        std::ifstream file(settings_path.string());
        file >> j_settings;
        settings = j_settings;
    }

    std::shared_ptr<sigma::graphics::texture> texture;
    switch (settings.format) {
    case sigma::graphics::texture_format::RGB8: {
        boost::gil::rgb8_image_t image;
        load_texture(source_path, image);
        texture = std::make_shared<sigma::graphics::texture>(context, key, boost::gil::const_view(image), settings.minification, settings.magnification, settings.mipmap);
        break;
    }
    case sigma::graphics::texture_format::RGBA8: {
        boost::gil::rgba8_image_t image;
        load_texture(source_path, image);
        texture = std::make_shared<sigma::graphics::texture>(context, key, boost::gil::const_view(image), settings.minification, settings.magnification, settings.mipmap);
        break;
    }
    case sigma::graphics::texture_format::RGB32F: {
        boost::gil::rgb32f_image_t image;
        load_texture(source_path, image);
        texture = std::make_shared<sigma::graphics::texture>(context, key, boost::gil::const_view(image), settings.minification, settings.magnification, settings.mipmap);
        break;
    }
    }

    cache->insert(key, texture, true);
}
