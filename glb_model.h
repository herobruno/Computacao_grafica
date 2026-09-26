// Copyright
// Computação Gráfica
// URI Santiago
// Professor Laurence

#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <GL/glew.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <cstdint>
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

static inline bool load_texture_index_from_material(unsigned int& texture_index,
                                                    const aiMaterial* material) {
  if (!material) {
    return false;
  }

  aiString texture_path;
  if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0 &&
      material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) != AI_SUCCESS) {
    return false;
  }

  const std::string texture_path_string = texture_path.C_Str();
  if (!texture_path_string.empty() && texture_path_string[0] == '*') {
    texture_index = std::strtoul(texture_path_string.c_str() + 1, nullptr, 10);
    return true;
  }

  return false;
}

static inline GlbTexture load_texture(const uint8_t* texture_data, GLenum internal_format,
                                      int texture_width, int texture_height, GLenum input_format) {
  GLint previous_texture_bind, previous_unpack_alignment;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture_bind);
  glGetIntegerv(GL_UNPACK_ALIGNMENT, &previous_unpack_alignment);

  GlbTexture texture;
  glGenTextures(1, &texture.id);
  glBindTexture(GL_TEXTURE_2D, texture.id);
  texture.width = texture_width;
  texture.height = texture_height;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture_width, texture_height, 0, input_format,
               GL_UNSIGNED_BYTE, texture_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glGenerateMipmap(GL_TEXTURE_2D);

  glPixelStorei(GL_UNPACK_ALIGNMENT, previous_unpack_alignment);
  glBindTexture(GL_TEXTURE_2D, previous_texture_bind);
  return texture;
}

static inline bool load_texture_embedded(GlbTexture& glb_texture, const aiTexture* assimp_texture) {
  if (!assimp_texture) {
    return false;
  }

  const uint8_t* assimp_texture_data = reinterpret_cast<const uint8_t*>(assimp_texture->pcData);

  if (assimp_texture->mHeight == 0) {
    int texture_length = assimp_texture->mWidth;
    int texture_width;
    int texture_height;
    uint8_t* glb_texture_data = stbi_load_from_memory(assimp_texture_data, texture_length,
                                                      &texture_width, &texture_height, nullptr, 4);
    if (!glb_texture_data) {
      return false;
    }

    const GLenum internal_format = GL_RGBA;
    const GLenum input_format = GL_RGBA;
    glb_texture = load_texture(glb_texture_data, internal_format, texture_width, texture_height,
                               input_format);
    stbi_image_free(glb_texture_data);
    return true;
  }

  const GLenum internal_format = GL_RGBA;
  const int texture_width = assimp_texture->mWidth;
  const int texture_height = assimp_texture->mHeight;
  const GLenum input_format = GL_BGRA;
  glb_texture = load_texture(assimp_texture_data, internal_format, texture_width, texture_height,
                             input_format);
  return true;
}

static inline bool load_model(GlbModel& model, const std::string& filepath) {
  model.importer = std::make_unique<Assimp::Importer>();
  model.scene = model.importer->ReadFile(
      filepath.c_str(), aiProcess_Triangulate | aiProcess_GenNormals |
                            aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices |
                            aiProcess_CalcTangentSpace | aiProcess_FlipUVs);
  if (!model.scene) {
    std::cerr << "Erro ao carregar modelo: " << filepath << std::endl;
    return false;
  }

  model.textures.clear();

  for (unsigned int material_index = 0; material_index < model.scene->mNumMaterials;
       material_index++) {
    const aiMaterial* material = model.scene->mMaterials[material_index];

    unsigned int texture_index;
    if (!load_texture_index_from_material(texture_index, material)) {
      continue;
    }
    if (texture_index >= model.scene->mNumTextures) {
      continue;
    }

    GlbTexture texture;
    if (!load_texture_embedded(texture, model.scene->mTextures[texture_index])) {
      continue;
    }
    model.textures[material_index] = texture;
  }

  return true;
}

static inline void draw_model(const GlbModel& model, const aiNode* node) {
  if (!node) {
    return;
  }

  glPushMatrix();
  {
    aiMatrix4x4 matrix = node->mTransformation;
    matrix.Transpose();
    glMultMatrixf(&matrix.a1);

    for (unsigned int mesh_index = 0; mesh_index < node->mNumMeshes; mesh_index++) {
      const aiMesh* mesh = model.scene->mMeshes[node->mMeshes[mesh_index]];

      glColor3ub(255, 255, 255);
      const auto iterator = model.textures.find(mesh->mMaterialIndex);
      if (iterator != model.textures.end() && iterator->second.id) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, iterator->second.id);

      } else {
        glDisable(GL_TEXTURE_2D);
      }

      glBegin(GL_TRIANGLES);
      for (unsigned int face_index = 0; face_index < mesh->mNumFaces; face_index++) {
        const aiFace& face = mesh->mFaces[face_index];
        for (unsigned int vertex_index = 0; vertex_index < face.mNumIndices; vertex_index++) {
          const unsigned current_vertex_index = face.mIndices[vertex_index];

          if (mesh->HasNormals()) {
            const aiVector3D& normal = mesh->mNormals[current_vertex_index];
            glNormal3f(normal.x, normal.y, normal.z);
          }

          if (mesh->HasTextureCoords(0)) {
            const aiVector3D& texture_uv = mesh->mTextureCoords[0][current_vertex_index];
            glTexCoord2f(texture_uv.x, texture_uv.y);
          }

          const aiVector3D& point = mesh->mVertices[current_vertex_index];
          glVertex3f(point.x, point.y, point.z);
        }
      }
      glEnd();
    }

    for (unsigned int child_index = 0; child_index < node->mNumChildren; child_index++) {
      draw_model(model, node->mChildren[child_index]);
    }
  }
  glPopMatrix();
}

static inline void draw_model(const GlbModel& model) {
  if (!model.scene) {
    return;
  }
  if (!model.scene->mRootNode) {
    return;
  }

  draw_model(model, model.scene->mRootNode);
}

static inline void destroy_model(GlbModel& model) {
  for (auto& model_texture : model.textures) {
    if (model_texture.second.id) {
      glDeleteTextures(1, &model_texture.second.id);
      model_texture.second.id = 0;
    }
  }
  model.textures.clear();
  model.scene = nullptr;
  model.importer.reset();
}

#endif
