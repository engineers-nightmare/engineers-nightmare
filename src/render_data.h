#pragma once

#include <epoxy/gl.h>
#include <stdio.h>
#include <algorithm>


#define INSTANCE_BATCH_SIZE 256u        /* needs to be <= the value in the shader */

// 16M per frame
#define FRAME_DATA_SIZE     (16u * 1024 * 1024)
#define NUM_INFLIGHT_FRAMES 3

struct per_camera_params {
    glm::mat4 view_proj_matrix;
    glm::mat4 inv_centered_view_proj_matrix;
    float aspect;
};

struct frame_data {
    GLuint bo;
    void *base_ptr;
    size_t offset;
    GLsync fence;
    GLint hw_align;

    frame_data() : bo(0), base_ptr(0), offset(0), fence(0), hw_align(1) {
        glGenBuffers(1, &bo);
        glBindBuffer(GL_UNIFORM_BUFFER, bo);
        glBufferStorage(GL_UNIFORM_BUFFER, FRAME_DATA_SIZE, nullptr,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        base_ptr = glMapBufferRange(GL_UNIFORM_BUFFER, 0, FRAME_DATA_SIZE,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &hw_align);

        printf("frame_data base=%p hw_align=%d\n", base_ptr, hw_align);
    }

    /* Prepare for filling this frame_data. If the backing BO is still in flight,
    * this may stall waiting for the frame to retire.
    */
    void begin() {
        if (fence) {
            /* Wait on the fence if this frame_data might be in flight. */

            /* TODO: detect case where we are getting throttled by this wait -- it suggests
            * that either we have insufficient frame_data buffers, or that the driver is
            * trying to run the GPU an unacceptable number of frames behind.
            */
            glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
            glDeleteSync(fence);
            fence = nullptr;
        }

        offset = 0;
    }

    /* Signal that all uses of this frame_data have been submitted to the hardware. */
    void end() {
        /* All gpu commands using this frame_data have now been issued. Drop a fence
        * into the pipeline so we know when the buffer can be reused.
        */
        fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    /* A transient GPU memory allocation. Usable until end() is called.
    */
    template<typename T>
    struct alloc
    {
        T* ptr;
        size_t off;
        size_t size;

        void bind(GLuint index, frame_data *f) {
            glBindBufferRange(GL_UNIFORM_BUFFER, index, f->bo, off, size);
        }
    };

    /* Allocate a chunk of frame_data for an array of T*, aligned appropriately for
    * use as both a constant buffer, and for CPU access.
    */
    template<typename T>
    alloc<T> alloc_aligned(size_t count, size_t align = alignof(T)) {
        align = std::max(align, (size_t)hw_align);
        offset = (offset + align - 1) & ~(align - 1);
        alloc<T> a{ (T*)(offset + (size_t)base_ptr), offset, count * sizeof(T) };
        offset += a.size;
        return a;
    }
};
