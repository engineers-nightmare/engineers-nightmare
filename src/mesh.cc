#include <stdio.h>
#include <err.h>
#include <epoxy/gl.h>

#include "common.h"
#include "mesh.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// HISSSSS
#include <vector>


static void
compute_bounds(std::vector<vertex> *verts)
{
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;

    min_x = max_x = (*verts)[0].x;
    min_y = max_y = (*verts)[0].y;
    min_z = max_z = (*verts)[0].z;

    std::vector<vertex>::const_iterator it;
    for (it = ++verts->begin(); it != verts->end(); it++) {
        if (it->x < min_x) min_x = it->x;
        if (it->y < min_y) min_y = it->y;
        if (it->z < min_z) min_z = it->z;
        if (it->x > max_x) max_x = it->x;
        if (it->y > max_y) max_y = it->y;
        if (it->z > max_z) max_z = it->z;
    }

    printf("\tBounding box: (%2.2f %2.2f %2.2f) (%2.2f %2.2f %2.2f)\n",
            min_x, min_y, min_z, max_x, max_y, max_z);
}


sw_mesh *
load_mesh(char const *filename) {
    aiScene const *scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality |
            aiProcess_MakeLeftHanded);
    if (!scene)
        errx(1, "Failed to load mesh %s", filename);

    printf("Mesh %s:\n", filename);
    printf("\tNumMeshes: %d\n", scene->mNumMeshes);

    std::vector<vertex> verts;
    std::vector<unsigned> indices;

    for (int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh const *m = scene->mMeshes[i];
        if (!m->mFaces)
            errx(1, "Submesh %d is not indexed.\n");

        // when we have many submeshes we need to rebase the indices.
        // NOTE: we assume that all mesh origins coincide, so we're not applying transforms here
        int submesh_base = verts.size();

        for (int j = 0; j < m->mNumVertices; j++)
            verts.push_back(vertex(m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z, 0 /* mat */));

        for (int j = 0; j < m->mNumFaces; j++) {
            if (m->mFaces[j].mNumIndices != 3)
                errx(1, "Submesh %d face %d isnt a tri (%d indices)\n", i, j, m->mFaces[j].mNumIndices);

            indices.push_back(m->mFaces[j].mIndices[0] + submesh_base);
            indices.push_back(m->mFaces[j].mIndices[1] + submesh_base);
            indices.push_back(m->mFaces[j].mIndices[2] + submesh_base);
        }
    }

    printf("\tAfter processing: %d verts, %d indices\n", verts.size(), indices.size());
    compute_bounds(&verts);

    aiReleaseImport(scene);

    sw_mesh *ret = new sw_mesh;
    ret->num_vertices = verts.size();
    ret->num_indices = indices.size();
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

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(vertex), (GLvoid const *)offsetof(vertex, mat));

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
    glDrawElements(GL_TRIANGLES, m->num_indices, GL_UNSIGNED_INT, NULL);
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
