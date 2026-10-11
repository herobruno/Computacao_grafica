 // Copyright
 // Computação Gráfica
 // URI Santiago
 // Professor Laurence

#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <GL/glew.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>

#include <assimp/Importer.hpp>
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
  std::unordered_map<unsigned int, GlbTexture> textures;
};

static inline bool load_texture_index_from_material(
    unsigned int& texture_index,
    const aiMaterial* material) {
  if (!material) {
    return false;
  }

  aiString texture_path;

  const aiTextureType texture_types[] = {
      aiTextureType_DIFFUSE,
      aiTextureType_BASE_COLOR
  };

  for (const aiTextureType type : texture_types) {
    if (material->GetTextureCount(type) == 0) {
      continue;
    }

    if (material->GetTexture(type, 0, &texture_path) != AI_SUCCESS) {
      continue;
    }

    const std::string path = texture_path.C_Str();

    if (!path.empty() && path[0] == '*') {
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

static inline GlbTexture load_texture(
    const uint8_t* texture_data,
    GLenum internal_format,
    int texture_width,
    int texture_height,
    GLenum input_format) {
  GLint previous_texture_bind = 0;
  GLint previous_unpack_alignment = 4;

  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture_bind);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);

  GlbTexture texture;

  glGenTextures(1, &texture.id);
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
      texture_data);

  glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(
      GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glGenerateMipmap(GL_TEXTURE_2D);

  glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
  glBindTexture(GL_TEXTURE_2D, previous_texture_bind);

  return texture;
}

static inline bool load_texture_embedded(
    GlbTexture& glb_texture,
    const aiTexture* assimp_texture) {
  if (!assimp_texture) {
    return false;
  }

  const uint8_t* data =
      reinterpret_cast<const uint8_t*>(assimp_texture->pcData);

  if (assimp_texture->mHeight == 0) {
    int texture_width = 0;
    int texture_height = 0;

    uint8_t* decoded = stbi_load_from_memory(
        data,
        static_cast<int>(assimp_texture->mWidth),
        &texture_width,
        &texture_height,
        nullptr,
        4);

    if (!decoded) {
      std::cerr << "Falha ao decodificar textura incorporada.\n";
      return false;
    }

    glb_texture = load_texture(
        decoded,
        GL_RGBA,
        texture_width,
        texture_height,
        GL_RGBA);

    stbi_image_free(decoded);

    return glb_texture.id != 0;
  }

  const int texture_width =
      static_cast<int>(assimp_texture->mWidth);
  const int texture_height =
      static_cast<int>(assimp_texture->mHeight);

  glb_texture = load_texture(
      data,
      GL_RGBA,
      texture_width,
      texture_height,
      GL_BGRA);

  return glb_texture.id != 0;
}

static inline bool load_model(
    GlbModel& model,
    const std::string& filepath) {
  model.importer = std::make_unique<Assimp::Importer>();

  model.scene = model.importer->ReadFile(
      filepath.c_str(),
      aiProcess_Triangulate |
          aiProcess_GenNormals |
          aiProcess_ImproveCacheLocality |
          aiProcess_JoinIdenticalVertices |
          aiProcess_CalcTangentSpace |
          aiProcess_FlipUVs);

  if (!model.scene || !model.scene->mRootNode) {
    std::cerr << "Erro ao carregar modelo: "
              << filepath << "\n"
              << model.importer->GetErrorString()
              << std::endl;
    return false;
  }

  model.textures.clear();

  for (unsigned int material_index = 0;
       material_index < model.scene->mNumMaterials;
       ++material_index) {
    const aiMaterial* material =
        model.scene->mMaterials[material_index];

    unsigned int texture_index = 0;

    if (!load_texture_index_from_material(
            texture_index, material)) {
      continue;
    }

    if (texture_index >= model.scene->mNumTextures) {
      continue;
    }

    GlbTexture texture;

    if (!load_texture_embedded(
            texture,
            model.scene->mTextures[texture_index])) {
      continue;
    }

    model.textures[material_index] = texture;
  }

  return true;
}

static inline void draw_model(
    const GlbModel& model,
    const aiNode* node) {
  if (!node || !model.scene) {
    return;
  }

  glPushMatrix();

  {
    aiMatrix4x4 matrix = node->mTransformation;
    matrix.Transpose();
    glMultMatrixf(&matrix.a1);

    for (unsigned int mesh_index = 0;
         mesh_index < node->mNumMeshes;
         ++mesh_index) {
      const aiMesh* mesh =
          model.scene->mMeshes[node->mMeshes[mesh_index]];

      if (!mesh) {
        continue;
      }

      // Lê a cor original do material associado ao mesh.
      aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);

      if (mesh->mMaterialIndex < model.scene->mNumMaterials) {
        const aiMaterial* material =
            model.scene->mMaterials[mesh->mMaterialIndex];

        if (aiGetMaterialColor(
                material, AI_MATKEY_BASE_COLOR, &color) != AI_SUCCESS) {
          if (aiGetMaterialColor(
                  material, AI_MATKEY_COLOR_DIFFUSE, &color) != AI_SUCCESS) {
            color = aiColor4D(1.0f, 1.0f, 1.0f, 1.0f);
          }
        }
      }

      glColor4f(color.r, color.g, color.b, color.a);

      const auto iterator =
          model.textures.find(mesh->mMaterialIndex);

      if (iterator != model.textures.end() &&
          iterator->second.id != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, iterator->second.id);
      } else {
        glDisable(GL_TEXTURE_2D);
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

    for (unsigned int child_index = 0;
         child_index < node->mNumChildren;
         ++child_index) {
      draw_model(model, node->mChildren[child_index]);
    }
  }

  glPopMatrix();
}

static inline void draw_model(const GlbModel& model) {
  if (!model.scene || !model.scene->mRootNode) {
    return;
  }

  draw_model(model, model.scene->mRootNode);
}

static inline void destroy_model(GlbModel& model) {
  for (auto& model_texture : model.textures) {
    if (model_texture.second.id != 0) {
      glDeleteTextures(1, &model_texture.second.id);
      model_texture.second.id = 0;
    }
  }

  model.textures.clear();
  model.scene = nullptr;
  model.importer.reset();
}

#endif