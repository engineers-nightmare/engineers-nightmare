#include <stdio.h>
#include <SDL.h>
#include <err.h>
#include <epoxy/gl.h>

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "src/common.h"


#define APP_NAME    "Engineer's Nightmare"
#define DEFAULT_WIDTH   1024
#define DEFAULT_HEIGHT  768


struct wnd {
    SDL_Window *ptr;
    SDL_GLContext gl_ctx;
    int width;
    int height;
} wnd;


struct mesh {
    GLuint vbo;
    GLuint ibo;
    GLuint vao;
    GLuint num_indices;
};


mesh *load_mesh(char const *filename) {
    aiScene const *scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
    if (!scene)
        errx(1, "Failed to load mesh %s", filename);

    printf("Mesh %s:\n", filename);
    printf("\tNumMeshes: %d\n", scene->mNumMeshes);

    aiReleaseImport(scene);

    return 0;
}


void
gl_debug_callback(GLenum source __unused,
                  GLenum type __unused,
                  GLenum id __unused,
                  GLenum severity __unused,
                  GLsizei length __unused,
                  GLchar const *message,
                  void const *userParam __unused)
{
    printf("GL: %s\n", message);
}

mesh *scaffold;

void
init()
{
    printf("%s starting up.\n", APP_NAME);
    printf("OpenGL version: %.1f\n", epoxy_gl_version() / 10.0f);

    /* Enable GL debug extension */
    if (!epoxy_has_gl_extension("GL_KHR_debug"))
        errx(1, "No support for GL debugging, life isn't worth it.\n");

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_debug_callback, NULL);

    scaffold = load_mesh("mesh/initial_scaffold.obj");
}


void
resize(int width, int height)
{
    /* TODO: projection, resize offscreen (but screen-sized)
     * surfaces, etc.
     */
    glViewport(0, 0, width, height);
    wnd.width = width;
    wnd.height = height;
    printf("Resized to %dx%d\n", width, height);
}

void
update()
{
    /* TODO: incorporate user input */
    /* TODO: step simulation */

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* TODO: draw something */
}


void
run()
{
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                printf("Quit event caught, shutting down.\n");
                return;

            case SDL_WINDOWEVENT:
                /* We MUST support resize events even if we
                 * don't really care about resizing, because a tiling
                 * WM isn't going to give us what we asked for anyway!
                 */
                if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                    resize(e.window.data1, e.window.data2);
                break;

            /* TODO: mouse input */
            }
        }

        update();

        SDL_GL_SwapWindow(wnd.ptr);
    }
}

int
main(int argc, char **argv)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        errx(1, "Error initializing SDL: %s\n", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    wnd.ptr = SDL_CreateWindow(APP_NAME,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               DEFAULT_WIDTH,
                               DEFAULT_HEIGHT,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    if (!wnd.ptr)
        errx(1, "Failed to create window.\n");

    wnd.gl_ctx = SDL_GL_CreateContext(wnd.ptr);

    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    init();

    run();

    return 0;
}
