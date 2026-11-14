// Copyright
// Computação Gráfica
// URI Santiago
// Professor Laurence

#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <GL/glew.h>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "stb_image.h"

struct GlbTexture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
};

struct GlbModel {
    std::unique_ptr<Assimp::Importer> importer;
    const aiScene* scene = nullptr;

    // A chave é o índice do material no aiScene.
    std::unordered_map<unsigned int, GlbTexture> textures;
};

// -----------------------------------------------------------------------------
// Localiza a primeira textura incorporada (*0, *1, ...) do material.
// GLB normalmente armazena as imagens dentro do próprio arquivo.
// -----------------------------------------------------------------------------
static inline bool load_texture_index_from_material(
    unsigned int& texture_index,
    const aiMaterial* material) {

    if (!material) {
        return false;
    }

    aiString texture_path;

    // BASE_COLOR é o canal usado pelo fluxo PBR/glTF.
    // DIFFUSE é mantido como alternativa para outros modelos.
    const aiTextureType texture_types[] = {
        aiTextureType_BASE_COLOR,
        aiTextureType_DIFFUSE
    };

    for (const aiTextureType type : texture_types) {
        if (material->GetTextureCount(type) == 0) {
            continue;
        }

        if (material->GetTexture(type, 0, &texture_path) != AI_SUCCESS) {
            continue;
        }

        const std::string path = texture_path.C_Str();

        if (path.size() > 1 && path[0] == '*') {
            char* end = nullptr;
            const unsigned long index =
                std::strtoul(path.c_str() + 1, &end, 10);

            if (end != path.c_str() + 1 && *end == '\0') {
                texture_index = static_cast<unsigned int>(index);
                return true;
            }
        }
    }

    return false;
}

// -----------------------------------------------------------------------------
// Envia os pixels para a GPU e cria mipmaps.
// -----------------------------------------------------------------------------
static inline GlbTexture load_texture(
    const uint8_t* texture_data,
    GLenum internal_format,
    int texture_width,
    int texture_height,
    GLenum input_format) {

    GlbTexture texture;

    if (!texture_data || texture_width <= 0 || texture_height <= 0) {
        return texture;
    }

    GLint previous_texture_bind = 0;
    GLint previous_unpack_alignment = 4;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture_bind);
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);

    glGenTextures(1, &texture.id);

    if (texture.id == 0) {
        glBindTexture(GL_TEXTURE_2D, previous_texture_bind);
        return texture;
    }

    glBindTexture(GL_TEXTURE_2D, texture.id);

    texture.width = texture_width;
    texture.height = texture_height;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internal_format,
        texture_width,
        texture_height,
        0,
        input_format,
        GL_UNSIGNED_BYTE,
        texture_data
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D);

    glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previous_texture_bind));

    return texture;
}

// -----------------------------------------------------------------------------
// Carrega uma textura incorporada pelo Assimp.
//
// mHeight == 0: imagem comprimida (PNG/JPG), decodificada pelo stb_image.
// mHeight > 0: imagem já descomprimida em pixels aiTexel (BGRA).
// -----------------------------------------------------------------------------
static inline bool load_texture_embedded(
    GlbTexture& glb_texture,
    const aiTexture* assimp_texture) {

    if (!assimp_texture || !assimp_texture->pcData) {
        return false;
    }

    const uint8_t* data =
        reinterpret_cast<const uint8_t*>(assimp_texture->pcData);

    if (assimp_texture->mHeight == 0) {
        int texture_width = 0;
        int texture_height = 0;

        // mWidth representa o tamanho, em bytes, da imagem comprimida.
        uint8_t* decoded = stbi_load_from_memory(
            data,
            static_cast<int>(assimp_texture->mWidth),
            &texture_width,
            &texture_height,
            nullptr,
            4
        );

        if (!decoded) {
            std::cerr
                << "[MODEL] Falha ao decodificar textura incorporada: "
                << stbi_failure_reason()
                << '\n';
            return false;
        }

        glb_texture = load_texture(
            decoded,
            GL_RGBA,
            texture_width,
            texture_height,
            GL_RGBA
        );

        stbi_image_free(decoded);

        return glb_texture.id != 0;
    }

    // Textura não comprimida: aiTexel usa os canais na ordem BGRA.
    glb_texture = load_texture(
        data,
        GL_RGBA,
        static_cast<int>(assimp_texture->mWidth),
        static_cast<int>(assimp_texture->mHeight),
        GL_BGRA
    );

    return glb_texture.id != 0;
}

// -----------------------------------------------------------------------------
// Obtém a cor-base do material. Se o material não possuir fator de cor,
// utiliza branco para não alterar as cores da textura ou dos vértices.
// -----------------------------------------------------------------------------
static inline aiColor4D get_material_base_color(
    const aiMaterial* material) {

    aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);

    if (!material) {
        return color;
    }

    if (aiGetMaterialColor(
            material,
            AI_MATKEY_BASE_COLOR,
            &color) != AI_SUCCESS) {

        if (aiGetMaterialColor(
                material,
                AI_MATKEY_COLOR_DIFFUSE,
                &color) != AI_SUCCESS) {
            color = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    return color;
}

// -----------------------------------------------------------------------------
// Carrega a cena e as texturas incorporadas.
// Execute esta função depois de criar o contexto OpenGL.
// -----------------------------------------------------------------------------
static inline bool load_model(
    GlbModel& model,
    const std::string& filepath) {

    // Libera texturas de um carregamento anterior, se houver.
    // É necessário que exista um contexto OpenGL ativo.
    for (auto& entry : model.textures) {
        if (entry.second.id != 0) {
            glDeleteTextures(1, &entry.second.id);
        }
    }

    model.textures.clear();
    model.scene = nullptr;
    model.importer.reset();

    model.importer = std::make_unique<Assimp::Importer>();

    model.scene = model.importer->ReadFile(
        filepath.c_str(),
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_ImproveCacheLocality |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs
    );

    if (!model.scene || !model.scene->mRootNode) {
        std::cerr
            << "[MODEL] Erro ao carregar: " << filepath << '\n'
            << model.importer->GetErrorString()
            << std::endl;

        model.scene = nullptr;
        return false;
    }

    std::cout
        << "[MODEL] Modelo carregado: " << filepath << '\n'
        << "[MODEL] Materiais: " << model.scene->mNumMaterials << '\n'
        << "[MODEL] Texturas incorporadas: " << model.scene->mNumTextures
        << std::endl;

    for (unsigned int material_index = 0;
         material_index < model.scene->mNumMaterials;
         ++material_index) {

        const aiMaterial* material =
            model.scene->mMaterials[material_index];

        aiString material_name;
        material->Get(AI_MATKEY_NAME, material_name);

        unsigned int texture_index = 0;

        if (!load_texture_index_from_material(
                texture_index,
                material)) {

            std::cout
                << "[MODEL] Material " << material_index
                << " (" << material_name.C_Str()
                << ") sem textura incorporada; usando cor do material."
                << std::endl;
            continue;
        }

        if (texture_index >= model.scene->mNumTextures) {
            std::cerr
                << "[MODEL] Índice de textura inválido no material "
                << material_index << ": " << texture_index
                << std::endl;
            continue;
        }

        GlbTexture texture;

        if (!load_texture_embedded(
                texture,
                model.scene->mTextures[texture_index])) {

            std::cerr
                << "[MODEL] Falha ao carregar textura "
                << texture_index << " do material "
                << material_index << " (" << material_name.C_Str()
                << ")."
                << std::endl;
            continue;
        }

        model.textures[material_index] = texture;

        std::cout
            << "[MODEL] Textura " << texture_index
            << " carregada para material " << material_index
            << " (" << material_name.C_Str() << "): "
            << texture.width << "x" << texture.height
            << ", OpenGL ID=" << texture.id
            << std::endl;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Desenha um nó e seus filhos.
// Inclui:
// - cor-base do material;
// - cor individual por vértice (COLOR_0), quando existir;
// - textura UV incorporada, quando existir.
// -----------------------------------------------------------------------------
static inline void draw_model(
    const GlbModel& model,
    const aiNode* node) {

    if (!node || !model.scene) {
        return;
    }

    glPushMatrix();

    // Salva estados que este desenho modifica, evitando interferir
    // no restante da cena.
    glPushAttrib(
        GL_ENABLE_BIT |
        GL_TEXTURE_BIT |
        GL_CURRENT_BIT |
        GL_LIGHTING_BIT
    );

    aiMatrix4x4 matrix = node->mTransformation;
    matrix.Transpose();
    glMultMatrixf(&matrix.a1);

    // Faz glColor influenciar o material quando a iluminação estiver ativa.
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // A textura é multiplicada pela cor do material e por COLOR_0.
    glTexEnvi(
        GL_TEXTURE_ENV,
        GL_TEXTURE_ENV_MODE,
        GL_MODULATE
    );

    for (unsigned int mesh_index = 0;
         mesh_index < node->mNumMeshes;
         ++mesh_index) {

        const aiMesh* mesh =
            model.scene->mMeshes[node->mMeshes[mesh_index]];

        if (!mesh) {
            continue;
        }

        const aiMaterial* material = nullptr;

        if (mesh->mMaterialIndex < model.scene->mNumMaterials) {
            material = model.scene->mMaterials[mesh->mMaterialIndex];
        }

        const aiColor4D material_color =
            get_material_base_color(material);

        const auto texture_iterator =
            model.textures.find(mesh->mMaterialIndex);

        if (texture_iterator != model.textures.end() &&
            texture_iterator->second.id != 0) {

            glEnable(GL_TEXTURE_2D);
            glBindTexture(
                GL_TEXTURE_2D,
                texture_iterator->second.id
            );
        } else {
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glBegin(GL_TRIANGLES);

        for (unsigned int face_index = 0;
             face_index < mesh->mNumFaces;
             ++face_index) {

            const aiFace& face = mesh->mFaces[face_index];

            for (unsigned int vertex_index = 0;
                 vertex_index < face.mNumIndices;
                 ++vertex_index) {

                const unsigned int current_vertex_index =
                    face.mIndices[vertex_index];

                // COLOR_0: preserva as variações de cor exportadas no GLB.
                // No glTF, a cor do vértice é multiplicada pela cor-base
                // do material.
                if (mesh->HasVertexColors(0)) {
                    const aiColor4D& vertex_color =
                        mesh->mColors[0][current_vertex_index];

                    glColor4f(
                        vertex_color.r * material_color.r,
                        vertex_color.g * material_color.g,
                        vertex_color.b * material_color.b,
                        vertex_color.a * material_color.a
                    );
                } else {
                    glColor4f(
                        material_color.r,
                        material_color.g,
                        material_color.b,
                        material_color.a
                    );
                }

                if (mesh->HasNormals()) {
                    const aiVector3D& normal =
                        mesh->mNormals[current_vertex_index];

                    glNormal3f(normal.x, normal.y, normal.z);
                }

                if (mesh->HasTextureCoords(0)) {
                    const aiVector3D& uv =
                        mesh->mTextureCoords[0][current_vertex_index];

                    glTexCoord2f(uv.x, uv.y);
                }

                const aiVector3D& point =
                    mesh->mVertices[current_vertex_index];

                glVertex3f(point.x, point.y, point.z);
            }
        }

        glEnd();
    }

    // Desenha os nós filhos mantendo a hierarquia de transformações.
    for (unsigned int child_index = 0;
         child_index < node->mNumChildren;
         ++child_index) {

        draw_model(model, node->mChildren[child_index]);
    }

    glPopAttrib();
    glPopMatrix();
}

static inline void draw_model(const GlbModel& model) {
    if (!model.scene || !model.scene->mRootNode) {
        return;
    }

    draw_model(model, model.scene->mRootNode);
}

// -----------------------------------------------------------------------------
// Libera as texturas da GPU e os dados mantidos pelo Assimp.
// Execute esta função enquanto o contexto OpenGL ainda estiver ativo.
// -----------------------------------------------------------------------------
static inline void destroy_model(GlbModel& model) {
    for (auto& entry : model.textures) {
        if (entry.second.id != 0) {
            glDeleteTextures(1, &entry.second.id);
            entry.second.id = 0;
        }
    }

    model.textures.clear();
    model.scene = nullptr;
    model.importer.reset();
}

#endif // MODEL_HANDLER_H
