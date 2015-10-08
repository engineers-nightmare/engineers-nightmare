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

    static aiPropertyStore* meshImportProps = nullptr;
    if (!meshImportProps) {
        /* FIXME: threading hazard if we want to do concurrent loading */
        meshImportProps = aiCreatePropertyStore();
        aiSetImportPropertyInteger(meshImportProps, AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
    }

    aiScene const *scene = aiImportFileExWithProperties(filename, aiProcess_Triangulate |
            aiProcess_GenNormals | aiProcess_OptimizeMeshes | aiProcess_RemoveComponent,
            nullptr /* IO */,
            meshImportProps);

    if (!scene)
        errx(1, "Failed to load mesh %s", filename);

    printf("Mesh %s:\n", filename);
    printf("\tNumMeshes: %d\n", scene->mNumMeshes);

    std::vector<vertex> verts;
    std::vector<unsigned> indices;
    std::vector<glm::mat4> attach_points[num_wire_types];

    auto attach_name = "attach_";

    for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++) {
        aiNode const *n = scene->mRootNode->mChildren[i];

        if (!n) {
            continue;
        }

        const char *str = n->mName.C_Str();

        auto type = wire_type_power;

        /* match on "attach_{wire_type}_nnn"
         * for instance, "attach_power_001"
         * name assumed to be lower case
         */
        if (strstr(str, attach_name)) {
            /* is attach */

            auto found = false;
            for (auto wire_index = 0; wire_index < num_wire_types; ++wire_index) {
                if (strstr(str, wire_type_names[wire_index])) {
                    type = (wire_type)wire_index;
                    found = true;
                    break;
                }
            }

            if (found == false) {
                assert(false || "attach of unknown type found.");
                continue;
            }

            auto attach_mat = n->mTransformation;
            glm::mat4 mat;

            mat = glm::transpose(glm::make_mat4(&attach_mat.a1));

            attach_points[type].push_back(mat);
        }
        else {
            /* isn't attach */

            for (unsigned int k = 0; k < n->mNumMeshes; ++k) {
                aiMesh const *m = scene->mMeshes[n->mMeshes[k]];

                // when we have many submeshes we need to rebase the indices.
                // NOTE: we assume that all mesh origins coincide, so we're not applying transforms here
                auto submesh_base = (unsigned)verts.size();

                for (unsigned int j = 0; j < m->mNumVertices; j++)
                    verts.push_back(vertex(m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z,
                        -m->mNormals[j].x, -m->mNormals[j].y, -m->mNormals[j].z, 0 /* mat */));

                for (unsigned int j = 0; j < m->mNumFaces; j++) {
                    if (m->mFaces[j].mNumIndices != 3)
                        errx(1, "Submesh %u face %u isnt a tri (%u indices)\n", i, j, m->mFaces[j].mNumIndices);

                    indices.push_back(m->mFaces[j].mIndices[0] + submesh_base);
                    indices.push_back(m->mFaces[j].mIndices[1] + submesh_base);
                    indices.push_back(m->mFaces[j].mIndices[2] + submesh_base);
                }
            }
        }
    }

    printf("\tAfter processing: %zu verts, %zu indices",
        verts.size(), indices.size());

    for (auto wire_index = 0; wire_index < num_wire_types; ++wire_index) {
        auto type = (wire_type)wire_index;
        printf(", %zu %s attach points",
            attach_points[type].size(), wire_type_names[type]);
    }
    printf("\n");



    aiReleaseImport(scene);

    sw_mesh *ret = new sw_mesh;
    ret->num_vertices = (unsigned)verts.size();
    ret->num_indices = (unsigned)indices.size();
    ret->verts = new vertex[verts.size()];
    memcpy(ret->verts, &verts[0], sizeof(vertex) * verts.size());
    ret->indices = new unsigned int[indices.size()];
    memcpy(ret->indices, &indices[0], sizeof(unsigned) * indices.size());

    for (auto wire_index = 0; wire_index < num_wire_types; ++wire_index) {
        auto type = (wire_type)wire_index;
        ret->num_attach_points[type] = (unsigned)attach_points[type].size();
        ret->attach_points[type] = new glm::mat4[attach_points[type].size()];
        memcpy(ret->attach_points[type], &attach_points[type][0],
            sizeof(glm::mat4) * attach_points[type].size());
    }

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

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(vertex), (GLvoid const *)offsetof(vertex, mat));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid const *)offsetof(vertex, nx));

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


void
set_mesh_material(sw_mesh *m, int material)
{
    for (unsigned int i = 0; i < m->num_vertices; i++) {
        m->verts[i].mat = material;
    }
}
