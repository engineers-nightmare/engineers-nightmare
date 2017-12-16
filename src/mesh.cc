#ifndef _WIN32
#include <err.h> /* errx */
#else
#include "winerr.h"
#endif

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <epoxy/gl.h>
#include <stdio.h>
#include <vector>

#include "mesh.h"


sw_mesh *
load_mesh(char const *filename) {

    aiScene const *scene = aiImportFileEx(filename, aiProcess_Triangulate | aiProcess_OptimizeMeshes,
        nullptr /* IO */);

    if (!scene)
        errx(1, "Failed to load mesh %s", filename);

    printf("Mesh %s:\n", filename);
    printf("\tNumMeshes: %d\n", scene->mNumMeshes);

    std::vector<vertex> verts;
    std::vector<unsigned> indices;

    // Temp hack for braindamaged exporters that insist on jumbling axes.
    // This can go away once we read .vox ourselves.
    bool fixup = !!strstr(filename, ".obj");

    for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++) {
        aiNode const *n = scene->mRootNode->mChildren[i];

        for (unsigned int k = 0; k < n->mNumMeshes; ++k) {
            aiMesh const *m = scene->mMeshes[n->mMeshes[k]];

            // when we have many submeshes we need to rebase the indices.
            // NOTE: we assume that all mesh origins coincide, so we're not applying transforms here
            auto submesh_base = (unsigned)verts.size();

            if (fixup) {
                // rotate to undo what MV did on export:
                // Y' = -Z    Z' = Y
                for (unsigned int j = 0; j < m->mNumVertices; j++) {
                    verts.push_back(vertex(m->mVertices[j].x, -m->mVertices[j].z, m->mVertices[j].y,
                        m->mNormals[j].x, -m->mNormals[j].z, m->mNormals[j].y,
                        m->mTextureCoords[0] ? m->mTextureCoords[0][j].x : 0.0f,
                        m->mTextureCoords[0] ? m->mTextureCoords[0][j].y : 0.0f));
                }
            }
            else {
                for (unsigned int j = 0; j < m->mNumVertices; j++) {
                    verts.push_back(vertex(m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z,
                        m->mNormals[j].x, m->mNormals[j].y, m->mNormals[j].z,
                        m->mTextureCoords[0] ? m->mTextureCoords[0][j].x : 0.0f,
                        m->mTextureCoords[0] ? m->mTextureCoords[0][j].y : 0.0f));
                }
            }

            for (unsigned int j = 0; j < m->mNumFaces; j++) {
                indices.push_back(m->mFaces[j].mIndices[0] + submesh_base);
                indices.push_back(m->mFaces[j].mIndices[1] + submesh_base);
                indices.push_back(m->mFaces[j].mIndices[2] + submesh_base);
            }
        }
    }

    printf("\tAfter processing: %zu verts, %zu indices",
        verts.size(), indices.size());

    aiReleaseImport(scene);

    sw_mesh *ret = new sw_mesh;
    ret->num_vertices = (unsigned)verts.size();
    ret->num_indices = (unsigned)indices.size();
    ret->verts = new vertex[verts.size()];
    memcpy(ret->verts, &verts[0], sizeof(vertex) * verts.size());
    ret->indices = new unsigned int[indices.size()];
    memcpy(ret->indices, &indices[0], sizeof(unsigned) * indices.size());

    return ret;
}

hw_mesh *
upload_mesh(sw_mesh *mesh)
{
    hw_mesh *ret = new hw_mesh;

    glGenVertexArrays(1, &ret->vao);
    glBindVertexArray(ret->vao);

    glGenBuffers(1, &ret->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ret->vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh->num_vertices * sizeof(vertex), mesh->verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid const *)offsetof(vertex, x));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_INT_2_10_10_10_REV, GL_TRUE, sizeof(vertex), (GLvoid const *)offsetof(vertex, normal_packed));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(vertex), (GLvoid const *)offsetof(vertex, uv_packed));

    glGenBuffers(1, &ret->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ret->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_indices * sizeof(unsigned), mesh->indices, GL_STATIC_DRAW);

    ret->num_indices = mesh->num_indices;

    printf("upload_mesh: %p num_indices=%d vram_size=%.1fKB\n", ret,
            ret->num_indices, (mesh->num_vertices * sizeof(vertex) + mesh->num_indices * sizeof(unsigned)) / 1024.0f);

    return ret;
}


void
draw_mesh(hw_mesh *m)
{
    glBindVertexArray(m->vao);
    glDrawElements(GL_TRIANGLES, m->num_indices, GL_UNSIGNED_INT, nullptr);
}


void
draw_mesh_instanced(hw_mesh *m, unsigned num_instances)
{
    glBindVertexArray(m->vao);
    glDrawElementsInstanced(GL_TRIANGLES, m->num_indices,
                            GL_UNSIGNED_INT, nullptr, num_instances);
}


void
free_mesh(hw_mesh *m)
{
    /* TODO: try to reuse BO */
    glDeleteBuffers(1, &m->vbo);
    glDeleteBuffers(1, &m->ibo);
    glDeleteVertexArrays(1, &m->vao);

    printf("free_mesh: %p num_indices=%d\n", m, m->num_indices);
}

