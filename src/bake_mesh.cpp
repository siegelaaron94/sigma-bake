#include <sigma/context.hpp>
#include <sigma/graphics/static_mesh.hpp>
#include <sigma/resource/cache.hpp>
#include <sigma/util/filesystem.hpp>
#include <sigma/util/string.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/gtc/quaternion.hpp>

#include <filesystem>
#include <fstream>

using namespace std::literals::string_literals;

glm::vec3 convert_color(aiColor3D c)
{
    return glm::vec3(c.r, c.g, c.b);
}

glm::vec3 convert_3d(aiVector3D v)
{
    return { v.x, v.y, v.z };
}

glm::quat convert(aiQuaternion q)
{
    return { q.w, q.x, q.y, q.z };
}

glm::vec2 convert_2d(aiVector3D v)
{
    return glm::vec2(v.x, v.y);
}

std::string get_name(const aiMaterial* mat, const std::filesystem::path& source_directory)
{
    aiString matName;
    mat->Get(AI_MATKEY_NAME, matName);
    std::string name = matName.C_Str();

    if (name == "DefaultMaterial")
        return "default";

    if (sigma::util::starts_with(name, "//"s))
        return (source_directory / name.substr(2)).string();
    if (name[0] == '/')
        return name.substr(1);
    return (source_directory / name).string();
}

std::string get_name(const aiMesh* mesh)
{
    std::string name = mesh->mName.C_Str();
    if (name == "")
        return "unnamed";
    return name;
}

std::string get_name(const aiNode* node)
{
    std::string name = node->mName.C_Str();
    if (name == "")
        return "unnamed";
    return name;
}

void convert_static_mesh(
    std::shared_ptr<sigma::context> context,
    const std::filesystem::path& source_directory,
    const aiScene* scene,
    const aiMesh* src_mesh,
    std::shared_ptr<sigma::graphics::static_mesh> dest_mesh)
{
    auto material_cache = context->cache<sigma::graphics::material>();

    float radius = dest_mesh->radius();
    std::vector<sigma::graphics::static_mesh::vertex> submesh_vertices(src_mesh->mNumVertices);
    std::vector<sigma::graphics::static_mesh::triangle> submesh_triangles(src_mesh->mNumFaces);

    for (unsigned int j = 0; j < src_mesh->mNumVertices; ++j) {
        auto pos = src_mesh->mVertices[j];
        submesh_vertices[j].position = convert_3d(pos);
        radius = std::max(radius, glm::length(submesh_vertices[j].position));

        if (src_mesh->HasNormals()) {
            submesh_vertices[j].normal = convert_3d(src_mesh->mNormals[j]);
        }

        if (src_mesh->HasTangentsAndBitangents()) {
            submesh_vertices[j].tangent = convert_3d(src_mesh->mTangents[j]);
            submesh_vertices[j].bitangent = convert_3d(src_mesh->mBitangents[j]);
        }

        if (src_mesh->HasTextureCoords(0)) {
            submesh_vertices[j].texcoord = convert_2d(src_mesh->mTextureCoords[0][j]);
        }
    }

    for (unsigned int j = 0; j < src_mesh->mNumFaces; ++j) {
        aiFace f = src_mesh->mFaces[j];
        for (unsigned int k = 0; k < 3; ++k)
            submesh_triangles[j][k] = f.mIndices[k] + static_cast<unsigned int>(dest_mesh->vertices().size());
    }

    std::string material_name = get_name(scene->mMaterials[src_mesh->mMaterialIndex], source_directory);

    dest_mesh->vertices().reserve(dest_mesh->vertices().size() + submesh_vertices.size());
    dest_mesh->vertices().insert(dest_mesh->vertices().end(), submesh_vertices.begin(), submesh_vertices.end());

    dest_mesh->triangles().reserve(dest_mesh->triangles().size() + submesh_triangles.size());
    dest_mesh->triangles().insert(dest_mesh->triangles().end(), submesh_triangles.begin(), submesh_triangles.end());

    dest_mesh->parts().push_back(sigma::graphics::mesh_part { dest_mesh->triangles().size(), submesh_triangles.size(), material_cache->get(material_name) });

    dest_mesh->set_radius(radius);
}

void bake_mesh(std::shared_ptr<sigma::context> context, const std::filesystem::path& source_directory, const std::filesystem::path& source_path)
{
    std::string source_str = source_path.string();
    auto key = sigma::filesystem::make_relative(source_directory, source_path).replace_extension("");
    auto mesh_cache = context->cache<sigma::graphics::static_mesh>();

    // TODO FEATURE add settings.

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(source_str.c_str(),
        aiProcess_CalcTangentSpace
            | aiProcess_JoinIdenticalVertices
            | aiProcess_Triangulate
            // | aiProcess_LimitBoneWeights
            | aiProcess_ValidateDataStructure
            | aiProcess_ImproveCacheLocality
            // | aiProcess_RemoveRedundantMaterials
            | aiProcess_SortByPType
            | aiProcess_FindDegenerates
            | aiProcess_FindInvalidData
            // | aiProcess_GenUVCoords
            // | aiProcess_FindInstances
            | aiProcess_FlipUVs
            | aiProcess_CalcTangentSpace
            // | aiProcess_MakeLeftHanded
            | aiProcess_RemoveComponent
            // | aiProcess_GenNormals
            // | aiProcess_GenSmoothNormals
            // | aiProcess_SplitLargeMeshes
            | aiProcess_PreTransformVertices
            // | aiProcess_FixInfacingNormals
            // | aiProcess_TransformUVCoords
            // | aiProcess_ConvertToLeftHanded
            | aiProcess_OptimizeMeshes
        // | aiProcess_OptimizeGraph
        // | aiProcess_FlipWindingOrder
        // | aiProcess_SplitByBoneCount
        // | aiProcess_Debone
    );

    if (scene == nullptr)
        throw std::runtime_error(importer.GetErrorString());

    auto dest_mesh = std::make_shared<sigma::graphics::static_mesh>(context, key);
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        if (sigma::util::ends_with(get_name(scene->mMeshes[i]), "_high"s))
            continue;
        convert_static_mesh(context, key.parent_path(), scene, scene->mMeshes[i], dest_mesh);
    }
    dest_mesh->vertices().shrink_to_fit();
    dest_mesh->triangles().shrink_to_fit();
    dest_mesh->parts().shrink_to_fit();

    mesh_cache->insert(key, dest_mesh, true);
}
