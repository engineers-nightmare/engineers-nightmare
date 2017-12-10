
// ================================================================================================
// -*- C++ -*-
// File:   debug_draw.hpp
// Author: Guilherme R. Lampert
// Brief:  Debug Draw - an immediate-mode, renderer agnostic, lightweight debug drawing API.
// ================================================================================================

#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

// ========================================================
// Library Overview:
// ========================================================
//
// ---------
//  LICENSE
// ---------
// This software is in the public domain. Where that dedication is not recognized,
// you are granted a perpetual, irrevocable license to copy, distribute, and modify
// this file as you see fit.
//
// The source code is provided "as is", without warranty of any kind, express or implied.
// No attribution is required, but a mention about the author(s) is appreciated.
//
// -------------
//  QUICK SETUP
// -------------
// In *one* C++ source file, *before* including this file, do this:
//
//   #define DEBUG_DRAW_IMPLEMENTATION
//
// To enable the implementation. Further includes of this
// file *should not* redefine DEBUG_DRAW_IMPLEMENTATION.
// Example:
//
// In my_program.cpp:
//
//   #define DEBUG_DRAW_IMPLEMENTATION
//   #include "debug_draw.hpp"
//
// In my_program.hpp:
//
//   #include "debug_draw.hpp"
//
// ----------------------
//  COMPILATION SWITCHES
// ----------------------
//
// DEBUG_DRAW_CXX11_SUPPORTED
//  Enables the use of some C++11 features. If your compiler supports C++11
//  or better, you should define this switch globally or before every inclusion
//  of this file. If it is not defined, we try to guess it from the value of the
//  '__cplusplus' built-in macro constant.
//
// DEBUG_DRAW_MAX_*
//  Sizes of internal intermediate buffers, which are allocated on initialization
//  by the implementation. If you need to draw more primitives than the sizes of
//  these buffers, you need to redefine the macros and recompile.
//
// DEBUG_DRAW_VERTEX_BUFFER_SIZE
//  Size in dd::DrawVertex elements of the intermediate vertex buffer used
//  to batch primitives before sending them to dd::RenderInterface. A bigger
//  buffer will reduce the number of calls to dd::RenderInterface when drawing
//  large sets of debug primitives.
//
// DEBUG_DRAW_OVERFLOWED(message)
//  An error handler called if any of the DEBUG_DRAW_MAX_* sizes overflow.
//  By default it just prints a message to stderr.
//
// DEBUG_DRAW_*_TYPE_DEFINED
//  The compound types used by Debug Draw can also be customized.
//  By default, ddVec3 and ddMat4x4 are plain C-arrays, but you can
//  redefine them to use your own classes or structures (see below).
//  ddStr is by default a std::string, but you can redefine it to
//  a custom string type if necessary. The only requirements are that
//  it provides a 'c_str()' method returning a null terminated
//  const char* string and an assignment operator (=).
//
// DEBUG_DRAW_NO_DEFAULT_COLORS
//  If defined, doesn't add the set of predefined color constants inside
//  dd::colors:: namespace. Each color is a ddVec3, so you can define this
//  to prevent adding more global data to the binary if you don't need them.
//
// DEBUG_DRAW_PER_THREAD_CONTEXT
//  If defined, a per-thread global context will be created for Debug Draw.
//  This allows having an instance of the library for each thread in
//  your application. You must then call initialize/shutdown/flush/etc
//  for each thread that wishes to use the library. If this is not
//  defined it defaults to a single threaded global context.
//
// DEBUG_DRAW_EXPLICIT_CONTEXT
//  If defined, each Debug Draw function will expect and additional argument
//  (the first one) which is the library context instance. This is an alternative
//  to DEBUG_DRAW_PER_THREAD_CONTEXT to allow having multiple instances of the
//  library in the same application. This flag is mutually exclusive with
//  DEBUG_DRAW_PER_THREAD_CONTEXT.
//
// -------------------
//  MEMORY ALLOCATION
// -------------------
// Debug Draw will only perform a couple of memory allocations during startup to decompress
// the built-in glyph bitmap used for debug text rendering and to allocate the vertex buffers
// and intermediate draw/batch buffers and context data used internally.
//
// Memory allocation and deallocation for Debug Draw will be done via:
//
//   DD_MALLOC(size)
//   DD_MFREE(ptr)
//
// These two macros can be redefined if you'd like to supply you own memory allocator.
// By default, they are defined to use std::malloc and std::free, respectively.
// Note: If you redefine one, you must also provide the other.
//
// --------------------------------
//  INTERFACING WITH YOUR RENDERER
// --------------------------------
// Debug Draw doesn't touch on any renderer-specific aspects or APIs, instead you provide
// the library with all of it's rendering needs via the dd::RenderInterface abstract class.
//
// See the declaration of dd::RenderInterface for details. Not all methods are
// required. In fact, you could also implement a full no-op RenderInterface that
// disables debug drawing by simply inheriting from dd::RenderInterface and not overriding
// any of the methods (or even easier, call dd::initialize(nullptr) to make everything a no-op).
//
// For examples on how to implement your own dd::RenderInterface, see the accompanying samples.
// You can also find them in the source code repository for this project:
// https://github.com/glampert/debug-draw
//
// ------------------
//  CONVENTIONS USED
// ------------------
// Points and lines are always specified in world-space positions. This also
// applies to shapes drawn from lines, like boxes, spheres, cones, etc.
//
// 2D screen-text is in screen-space pixels (from 0,0 in the upper-left
// corner of the screen to screen_width-1 and screen_height-1).
// RenderInterface::drawGlyphList() also receives vertexes in screen-space.
//
// We make some usage of matrices for things like the projected text labels.
// Matrix layout used is column-major and vectors multiply as columns.
// This is the convention normally used by standard OpenGL.
//
// C++ Exceptions are not used. Little error checking is provided or
// done inside the library. We favor simpler, faster and easier to maintain
// code over more sophisticated error handling. The rationale is that a
// debug drawing API doesn't have to be very robust, since it won't make
// into the final release executable in most cases.
//

// ========================================================
// Configurable compilation switches:
// ========================================================

//
// If the user didn't specify if C++11 or above are supported, try to guess
// from the value of '__cplusplus'. It should be 199711L for pre-C++11 compilers
// and 201103L in those supporting C++11, but this is not a guarantee that all
// C++11 features will be available and stable, so again, we are making a guess.
// It is recommended to instead supply the DEBUG_DRAW_CXX11_SUPPORTED switch
// yourself before including this file.
//
#ifndef DEBUG_DRAW_CXX11_SUPPORTED
#if (__cplusplus > 199711L)
#define DEBUG_DRAW_CXX11_SUPPORTED 1
#endif // __cplusplus
#endif // DEBUG_DRAW_CXX11_SUPPORTED

//
// Max elements of each type at any given time.
// We supply these reasonable defaults, but you can provide your
// own tunned values to save memory or fit all of your debug data.
// These are hard constraints. If not enough, change and recompile.
//
#ifndef DEBUG_DRAW_MAX_STRINGS
#define DEBUG_DRAW_MAX_STRINGS 512
#endif // DEBUG_DRAW_MAX_STRINGS

#ifndef DEBUG_DRAW_MAX_POINTS
#define DEBUG_DRAW_MAX_POINTS 8192
#endif // DEBUG_DRAW_MAX_POINTS

#ifndef DEBUG_DRAW_MAX_LINES
#define DEBUG_DRAW_MAX_LINES 32768
#endif // DEBUG_DRAW_MAX_LINES

//
// Size in vertexes of a local buffer we use to sort elements
// drawn with and without depth testing before submitting them to
// the dd::RenderInterface. A larger buffer will require less flushes
// (e.g. dd::RenderInterface calls) when drawing large amounts of
// primitives. Less will obviously save more memory. Each DrawVertex
// is about 32 bytes in size, we keep a context-specific array
// with this many entries.
//
#ifndef DEBUG_DRAW_VERTEX_BUFFER_SIZE
#define DEBUG_DRAW_VERTEX_BUFFER_SIZE 4096
#endif // DEBUG_DRAW_VERTEX_BUFFER_SIZE

//
// This macro is called with an error message if any of the above
// sizes is overflowed during runtime. In a debug build, you might
// keep this enabled to be able to log and find out if more space
// is needed for the debug data arrays. Default output is stderr.
//
#ifndef DEBUG_DRAW_OVERFLOWED
#include <cstdio>
#define DEBUG_DRAW_OVERFLOWED(message) std::fprintf(stderr, "%s\n", message)
#endif // DEBUG_DRAW_OVERFLOWED

//
// Use <math.h> and <float.h> for trigonometry functions by default.
// If you wish to avoid those dependencies, DD provides local approximations
// of the required functions as a portable replacement. Just define
// DEBUG_DRAW_USE_STD_MATH to zero before including this file.
//
#ifndef DEBUG_DRAW_USE_STD_MATH
#define DEBUG_DRAW_USE_STD_MATH 1
#endif // DEBUG_DRAW_USE_STD_MATH

// ========================================================
// Overridable Debug Draw types:
// ========================================================

#include <cstddef>
#include <cstdint>

//
// Following typedefs are not members of the dd:: namespace to allow easy redefinition by the user.
// If you provide a custom implementation for them before including this file, be sure to
// also define the proper DEBUG_DRAW_*_TYPE_DEFINED switch to disable the default typedefs.
//
// The only requirement placed on the vector/matrix types is that they provide
// an array subscript operator [] and have the expected number of elements. Apart
// from that, they could be structs, classes, what-have-you. POD types are recommended
// but not mandatory.
//

#ifndef DEBUG_DRAW_VEC3_TYPE_DEFINED
// ddVec3:
//  A small array of floats with at least three elements, but
//  it could have more for alignment purposes, extra slots are ignored.
//  A custom ddVec3 type must provide the array subscript operator.
typedef float ddVec3[3];

// ddVec3_In/ddVec3_Out:
//  Since our default ddVec3 is a plain C-array, it decays to a pointer
//  when passed as an input parameter to a function, so we can use it directly.
//  If you change it to some structured type, it might be more efficient
//  passing by const reference instead, however, some platforms have optimized
//  hardware registers for vec3s/vec4s, so passing by value might also be efficient.
typedef const ddVec3 ddVec3_In;
typedef       ddVec3 ddVec3_Out;

#define DEBUG_DRAW_VEC3_TYPE_DEFINED 1
#endif // DEBUG_DRAW_VEC3_TYPE_DEFINED

#ifndef DEBUG_DRAW_MAT4X4_TYPE_DEFINED
// ddMat4x4:
//  Homogeneous matrix of 16 floats, representing rotations as well as
//  translation/scaling and projections. The internal matrix layout used by this
//  library is COLUMN-MAJOR, vectors multiplying as columns (usual OpenGL convention).
//  Column-major matrix layout:
//          c.0   c.1   c.2    c.3
//    r.0 | 0.x   4.x   8.x    12.x |
//    r.1 | 1.y   5.y   9.y    13.y |
//    r.2 | 2.z   6.z   10.z   14.z |
//    r.3 | 3.w   7.w   11.w   15.w |
//  If your custom matrix type uses row-major format internally, you'll
//  have to transpose them before passing your matrices to the DD functions.
//  We use the array subscript operator internally, so it must also be provided.
typedef float ddMat4x4[4 * 4];

// ddMat4x4_In/ddMat4x4_Out:
//  Since our default ddMat4x4 is a plain C-array, it decays to a pointer
//  when passed as an input parameter to a function, so we can use it directly.
//  If you change it to some structured type, it might be more efficient
//  passing by const reference instead.
typedef const ddMat4x4 ddMat4x4_In;
typedef       ddMat4x4 ddMat4x4_Out;

#define DEBUG_DRAW_MAT4X4_TYPE_DEFINED 1
#endif // DEBUG_DRAW_MAT4X4_TYPE_DEFINED

#ifndef DEBUG_DRAW_STRING_TYPE_DEFINED
// ddStr:
//  String type used internally to store the debug text strings.
//  A custom string type must provide at least an assignment
//  operator (=) and a 'c_str()' method that returns a
//  null-terminated const char* string pointer. That's it.
//  An array subscript operator [] is not required for ddStr.
#include <string>
typedef std::string   ddStr;
typedef const ddStr & ddStr_In;
typedef       ddStr & ddStr_Out;

#define DEBUG_DRAW_STRING_TYPE_DEFINED 1
#endif // DEBUG_DRAW_STRING_TYPE_DEFINED

namespace dd
{

// ========================================================
// Optional built-in colors in RGB float format:
// ========================================================

#ifndef DEBUG_DRAW_NO_DEFAULT_COLORS
    namespace colors
    {
        extern const ddVec3 AliceBlue;
        extern const ddVec3 AntiqueWhite;
        extern const ddVec3 Aquamarine;
        extern const ddVec3 Azure;
        extern const ddVec3 Beige;
        extern const ddVec3 Bisque;
        extern const ddVec3 Black;
        extern const ddVec3 BlanchedAlmond;
        extern const ddVec3 Blue;
        extern const ddVec3 BlueViolet;
        extern const ddVec3 Brown;
        extern const ddVec3 BurlyWood;
        extern const ddVec3 CadetBlue;
        extern const ddVec3 Chartreuse;
        extern const ddVec3 Chocolate;
        extern const ddVec3 Coral;
        extern const ddVec3 CornflowerBlue;
        extern const ddVec3 Cornsilk;
        extern const ddVec3 Crimson;
        extern const ddVec3 Cyan;
        extern const ddVec3 DarkBlue;
        extern const ddVec3 DarkCyan;
        extern const ddVec3 DarkGoldenRod;
        extern const ddVec3 DarkGray;
        extern const ddVec3 DarkGreen;
        extern const ddVec3 DarkKhaki;
        extern const ddVec3 DarkMagenta;
        extern const ddVec3 DarkOliveGreen;
        extern const ddVec3 DarkOrange;
        extern const ddVec3 DarkOrchid;
        extern const ddVec3 DarkRed;
        extern const ddVec3 DarkSalmon;
        extern const ddVec3 DarkSeaGreen;
        extern const ddVec3 DarkSlateBlue;
        extern const ddVec3 DarkSlateGray;
        extern const ddVec3 DarkTurquoise;
        extern const ddVec3 DarkViolet;
        extern const ddVec3 DeepPink;
        extern const ddVec3 DeepSkyBlue;
        extern const ddVec3 DimGray;
        extern const ddVec3 DodgerBlue;
        extern const ddVec3 FireBrick;
        extern const ddVec3 FloralWhite;
        extern const ddVec3 ForestGreen;
        extern const ddVec3 Gainsboro;
        extern const ddVec3 GhostWhite;
        extern const ddVec3 Gold;
        extern const ddVec3 GoldenRod;
        extern const ddVec3 Gray;
        extern const ddVec3 Green;
        extern const ddVec3 GreenYellow;
        extern const ddVec3 HoneyDew;
        extern const ddVec3 HotPink;
        extern const ddVec3 IndianRed;
        extern const ddVec3 Indigo;
        extern const ddVec3 Ivory;
        extern const ddVec3 Khaki;
        extern const ddVec3 Lavender;
        extern const ddVec3 LavenderBlush;
        extern const ddVec3 LawnGreen;
        extern const ddVec3 LemonChiffon;
        extern const ddVec3 LightBlue;
        extern const ddVec3 LightCoral;
        extern const ddVec3 LightCyan;
        extern const ddVec3 LightGoldenYellow;
        extern const ddVec3 LightGray;
        extern const ddVec3 LightGreen;
        extern const ddVec3 LightPink;
        extern const ddVec3 LightSalmon;
        extern const ddVec3 LightSeaGreen;
        extern const ddVec3 LightSkyBlue;
        extern const ddVec3 LightSlateGray;
        extern const ddVec3 LightSteelBlue;
        extern const ddVec3 LightYellow;
        extern const ddVec3 Lime;
        extern const ddVec3 LimeGreen;
        extern const ddVec3 Linen;
        extern const ddVec3 Magenta;
        extern const ddVec3 Maroon;
        extern const ddVec3 MediumAquaMarine;
        extern const ddVec3 MediumBlue;
        extern const ddVec3 MediumOrchid;
        extern const ddVec3 MediumPurple;
        extern const ddVec3 MediumSeaGreen;
        extern const ddVec3 MediumSlateBlue;
        extern const ddVec3 MediumSpringGreen;
        extern const ddVec3 MediumTurquoise;
        extern const ddVec3 MediumVioletRed;
        extern const ddVec3 MidnightBlue;
        extern const ddVec3 MintCream;
        extern const ddVec3 MistyRose;
        extern const ddVec3 Moccasin;
        extern const ddVec3 NavajoWhite;
        extern const ddVec3 Navy;
        extern const ddVec3 OldLace;
        extern const ddVec3 Olive;
        extern const ddVec3 OliveDrab;
        extern const ddVec3 Orange;
        extern const ddVec3 OrangeRed;
        extern const ddVec3 Orchid;
        extern const ddVec3 PaleGoldenRod;
        extern const ddVec3 PaleGreen;
        extern const ddVec3 PaleTurquoise;
        extern const ddVec3 PaleVioletRed;
        extern const ddVec3 PapayaWhip;
        extern const ddVec3 PeachPuff;
        extern const ddVec3 Peru;
        extern const ddVec3 Pink;
        extern const ddVec3 Plum;
        extern const ddVec3 PowderBlue;
        extern const ddVec3 Purple;
        extern const ddVec3 RebeccaPurple;
        extern const ddVec3 Red;
        extern const ddVec3 RosyBrown;
        extern const ddVec3 RoyalBlue;
        extern const ddVec3 SaddleBrown;
        extern const ddVec3 Salmon;
        extern const ddVec3 SandyBrown;
        extern const ddVec3 SeaGreen;
        extern const ddVec3 SeaShell;
        extern const ddVec3 Sienna;
        extern const ddVec3 Silver;
        extern const ddVec3 SkyBlue;
        extern const ddVec3 SlateBlue;
        extern const ddVec3 SlateGray;
        extern const ddVec3 Snow;
        extern const ddVec3 SpringGreen;
        extern const ddVec3 SteelBlue;
        extern const ddVec3 Tan;
        extern const ddVec3 Teal;
        extern const ddVec3 Thistle;
        extern const ddVec3 Tomato;
        extern const ddVec3 Turquoise;
        extern const ddVec3 Violet;
        extern const ddVec3 Wheat;
        extern const ddVec3 White;
        extern const ddVec3 WhiteSmoke;
        extern const ddVec3 Yellow;
        extern const ddVec3 YellowGreen;
    } // namespace colors
#endif // DEBUG_DRAW_NO_DEFAULT_COLORS

// ========================================================
// Optional explicit context mode:
// ========================================================

#ifdef DEBUG_DRAW_EXPLICIT_CONTEXT
    struct OpaqueContextType { };
    typedef OpaqueContextType * ContextHandle;
    #define DD_EXPLICIT_CONTEXT_ONLY(...) __VA_ARGS__
#else // !DEBUG_DRAW_EXPLICIT_CONTEXT
#define DD_EXPLICIT_CONTEXT_ONLY(...) /* nothing */
#endif // DEBUG_DRAW_EXPLICIT_CONTEXT

// ========================================================
// Debug Draw functions:
// - Durations are always in milliseconds.
// - Colors are RGB floats in the [0,1] range.
// - Positions are in world-space, unless stated otherwise.
// ========================================================

// Add a point in 3D space to the debug draw queue.
// Point is expressed in world-space coordinates.
// Note that not all renderer support configurable point
// size, so take the 'size' parameter as a hint only
    void point(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
               ddVec3_In pos,
               ddVec3_In color,
               float size = 1.0f,
               int durationMillis = 0,
               bool depthEnabled = true);

// Add a 3D line to the debug draw queue. Note that
// lines are expressed in world coordinates, and so are
// all wireframe primitives which are built from lines.
    void line(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
              ddVec3_In from,
              ddVec3_In to,
              ddVec3_In color,
              int durationMillis = 0,
              bool depthEnabled = true);

// Add a set of three coordinate axis depicting the position and orientation of the given transform.
// 'size' defines the size of the arrow heads. 'length' defines the length of the arrow's base line.
    void axisTriad(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                   ddMat4x4_In transform,
                   float size,
                   float length,
                   int durationMillis = 0,
                   bool depthEnabled = true);

// Add a 3D line with an arrow-like end to the debug draw queue.
// 'size' defines the arrow head size. 'from' and 'to' the length of the arrow's base line.
    void arrow(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
               ddVec3_In from,
               ddVec3_In to,
               ddVec3_In color,
               float size,
               int durationMillis = 0,
               bool depthEnabled = true);

// Add an axis-aligned cross (3 lines converging at a point) to the debug draw queue.
// 'length' defines the length of the crossing lines.
// 'center' is the world-space point where the lines meet.
    void cross(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
               ddVec3_In center,
               float length,
               int durationMillis = 0,
               bool depthEnabled = true);

// Add a wireframe circle to the debug draw queue.
    void circle(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                ddVec3_In center,
                ddVec3_In planeNormal,
                ddVec3_In color,
                float radius,
                float numSteps,
                int durationMillis = 0,
                bool depthEnabled = true);

// Add a wireframe plane in 3D space to the debug draw queue.
// If 'normalVecScale' is not zero, a line depicting the plane normal is also draw.
    void plane(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
               ddVec3_In center,
               ddVec3_In planeNormal,
               ddVec3_In planeColor,
               ddVec3_In normalVecColor,
               float planeScale,
               float normalVecScale,
               int durationMillis = 0,
               bool depthEnabled = true);

// Add a wireframe sphere to the debug draw queue.
    void sphere(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                ddVec3_In center,
                ddVec3_In color,
                float radius,
                int durationMillis = 0,
                bool depthEnabled = true);

// Add a wireframe cone to the debug draw queue.
// The cone 'apex' is the point where all lines meet.
// The length of the 'dir' vector determines the thickness.
// 'baseRadius' & 'apexRadius' are in degrees.
    void cone(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
              ddVec3_In apex,
              ddVec3_In dir,
              ddVec3_In color,
              float baseRadius,
              float apexRadius,
              int durationMillis = 0,
              bool depthEnabled = true);

// Wireframe box from the eight points that define it.
    void box(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
             const ddVec3 points[8],
             ddVec3_In color,
             int durationMillis = 0,
             bool depthEnabled = true);

// Add a wireframe box to the debug draw queue.
    void box(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
             ddVec3_In center,
             ddVec3_In color,
             float width,
             float height,
             float depth,
             int durationMillis = 0,
             bool depthEnabled = true);

// Add a wireframe Axis Aligned Bounding Box (AABB) to the debug draw queue.
    void aabb(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
              ddVec3_In mins,
              ddVec3_In maxs,
              ddVec3_In color,
              int durationMillis = 0,
              bool depthEnabled = true);

// Add a wireframe frustum pyramid to the debug draw queue.
// 'invClipMatrix' is the inverse of the matrix defining the frustum
// (AKA clip) volume, which normally consists of the projection * view matrix.
// E.g.: inverse(projMatrix * viewMatrix)
    void frustum(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                 ddMat4x4_In invClipMatrix,
                 ddVec3_In color,
                 int durationMillis = 0,
                 bool depthEnabled = true);

// Add a vertex normal for debug visualization.
// The normal vector 'normal' is assumed to be already normalized.
    void vertexNormal(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                      ddVec3_In origin,
                      ddVec3_In normal,
                      float length,
                      int durationMillis = 0,
                      bool depthEnabled = true);

// Add a "tangent basis" at a given point in world space.
// Color scheme used is: normal=WHITE, tangent=YELLOW, bi-tangent=MAGENTA.
// The normal vector, tangent and bi-tangent vectors are assumed to be already normalized.
    void tangentBasis(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                      ddVec3_In origin,
                      ddVec3_In normal,
                      ddVec3_In tangent,
                      ddVec3_In bitangent,
                      float lengths,
                      int durationMillis = 0,
                      bool depthEnabled = true);

// Makes a 3D square grid of lines along the X and Z planes.
// 'y' defines the height in the Y axis where the grid is placed.
// The grid will go from 'mins' to 'maxs' units in both the X and Z.
// 'step' defines the gap between each line of the grid.
    void xzSquareGrid(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
                      float mins,
                      float maxs,
                      float y,
                      float step,
                      ddVec3_In color,
                      int durationMillis = 0,
                      bool depthEnabled = true);

// ========================================================
// Debug Draw vertex type:
// The only drawing type the user has to interface with.
// ========================================================

    union DrawVertex
    {
        struct
        {
            float x, y, z;
            float r, g, b;
            float size;
        } point;

        struct
        {
            float x, y, z;
            float r, g, b;
        } line;
    };

// ========================================================
// Debug Draw rendering callbacks:
// Implementation is provided by the user so we don't
// tie this code directly to a specific rendering API.
// ========================================================

    class RenderInterface
    {
    public:

        //
        // These are called by dd::flush() before any drawing and after drawing is finished.
        // User can override these to perform any common setup for subsequent draws and to
        // cleanup afterwards. By default, no-ops stubs are provided.
        //
        virtual void beginDraw();
        virtual void endDraw();

        //
        // Batch drawing methods for the primitives used by the debug renderer.
        // If you don't wish to support a given primitive type, don't override the method.
        //
        virtual void drawPointList(const DrawVertex * points, int count, bool depthEnabled);
        virtual void drawLineList(const DrawVertex * lines, int count, bool depthEnabled);

        // User defined cleanup. Nothing by default.
        virtual ~RenderInterface() = 0;
    };

// ========================================================
// Housekeeping functions:
// ========================================================

// Flags for dd::flush()
    enum FlushFlags
    {
        FlushPoints = 1 << 1,
        FlushLines  = 1 << 2,
        FlushAll    = (FlushPoints | FlushLines)
    };

// Initialize with the user-supplied renderer interface.
// Given object must remain valid until after dd::shutdown() is called!
// If 'renderer' is null, the Debug Draw functions become no-ops, but
// can still be safely called.
    bool initialize(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle * outCtx,) RenderInterface * renderer);

// After this is called, it is safe to dispose the dd::RenderInterface instance
// you passed to dd::initialize(). Shutdown will also attempt to free the glyph texture.
    void shutdown(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx));

// Test if the Debug Draw library is currently initialized and has a render interface.
    bool isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx));

// Test if there's data in the debug draw queue and dd::flush() should be called.
    bool hasPendingDraws(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx));

// Manually removes all queued debug render data without drawing.
// This is not normally called. To draw stuff, call dd::flush() instead.
    void clear(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx));

// Actually calls the dd::RenderInterface to consume the debug draw queues.
// Objects that have expired their lifetimes get removed. Pass the current
// application time in milliseconds to remove timed objects that have expired.
// Passing zero removes all objects after they get drawn, regardless of lifetime.
    void flush(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,)
               std::int64_t currTimeMillis = 0,
               std::uint32_t flags = FlushAll);

} // namespace dd

// ================== End of header file ==================
#endif // DEBUG_DRAW_HPP
// ================== End of header file ==================

// ================================================================================================
//
//                                  Debug Draw Implementation
//
// ================================================================================================

#ifdef DEBUG_DRAW_IMPLEMENTATION

#ifndef DD_MALLOC
    #include <cstdlib>
    #define DD_MALLOC std::malloc
    #define DD_MFREE  std::free
#endif // DD_MALLOC

#if DEBUG_DRAW_USE_STD_MATH
    #include <math.h>
    #include <float.h>
#endif // DEBUG_DRAW_USE_STD_MATH

namespace dd
{

#if defined(FLT_EPSILON) && DEBUG_DRAW_USE_STD_MATH
    static const float FloatEpsilon = FLT_EPSILON;
#else // !FLT_EPSILON || !DEBUG_DRAW_USE_STD_MATH
    static const float FloatEpsilon = 1e-14;
#endif // FLT_EPSILON && DEBUG_DRAW_USE_STD_MATH

#if defined(M_PI) && DEBUG_DRAW_USE_STD_MATH
    static const float PI = M_PI;
#else // !M_PI || !DEBUG_DRAW_USE_STD_MATH
    static const float PI = 3.1415926535897931f;
#endif // M_PI && DEBUG_DRAW_USE_STD_MATH

static const float HalfPI = PI * 0.5f;
static const float TAU    = PI * 2.0f;

template<typename T>
static inline float degreesToRadians(const T degrees)
{
    return (static_cast<float>(degrees) * PI / 180.0f);
}

template<typename T, int Size>
static inline int arrayLength(const T (&)[Size])
{
    return Size;
}

// ========================================================
// Built-in color constants:
// ========================================================

#ifndef DEBUG_DRAW_NO_DEFAULT_COLORS
namespace colors
{
const ddVec3 AliceBlue         = {0.941176f, 0.972549f, 1.000000f};
const ddVec3 AntiqueWhite      = {0.980392f, 0.921569f, 0.843137f};
const ddVec3 Aquamarine        = {0.498039f, 1.000000f, 0.831373f};
const ddVec3 Azure             = {0.941176f, 1.000000f, 1.000000f};
const ddVec3 Beige             = {0.960784f, 0.960784f, 0.862745f};
const ddVec3 Bisque            = {1.000000f, 0.894118f, 0.768627f};
const ddVec3 Black             = {0.000000f, 0.000000f, 0.000000f};
const ddVec3 BlanchedAlmond    = {1.000000f, 0.921569f, 0.803922f};
const ddVec3 Blue              = {0.000000f, 0.000000f, 1.000000f};
const ddVec3 BlueViolet        = {0.541176f, 0.168627f, 0.886275f};
const ddVec3 Brown             = {0.647059f, 0.164706f, 0.164706f};
const ddVec3 BurlyWood         = {0.870588f, 0.721569f, 0.529412f};
const ddVec3 CadetBlue         = {0.372549f, 0.619608f, 0.627451f};
const ddVec3 Chartreuse        = {0.498039f, 1.000000f, 0.000000f};
const ddVec3 Chocolate         = {0.823529f, 0.411765f, 0.117647f};
const ddVec3 Coral             = {1.000000f, 0.498039f, 0.313726f};
const ddVec3 CornflowerBlue    = {0.392157f, 0.584314f, 0.929412f};
const ddVec3 Cornsilk          = {1.000000f, 0.972549f, 0.862745f};
const ddVec3 Crimson           = {0.862745f, 0.078431f, 0.235294f};
const ddVec3 Cyan              = {0.000000f, 1.000000f, 1.000000f};
const ddVec3 DarkBlue          = {0.000000f, 0.000000f, 0.545098f};
const ddVec3 DarkCyan          = {0.000000f, 0.545098f, 0.545098f};
const ddVec3 DarkGoldenRod     = {0.721569f, 0.525490f, 0.043137f};
const ddVec3 DarkGray          = {0.662745f, 0.662745f, 0.662745f};
const ddVec3 DarkGreen         = {0.000000f, 0.392157f, 0.000000f};
const ddVec3 DarkKhaki         = {0.741176f, 0.717647f, 0.419608f};
const ddVec3 DarkMagenta       = {0.545098f, 0.000000f, 0.545098f};
const ddVec3 DarkOliveGreen    = {0.333333f, 0.419608f, 0.184314f};
const ddVec3 DarkOrange        = {1.000000f, 0.549020f, 0.000000f};
const ddVec3 DarkOrchid        = {0.600000f, 0.196078f, 0.800000f};
const ddVec3 DarkRed           = {0.545098f, 0.000000f, 0.000000f};
const ddVec3 DarkSalmon        = {0.913725f, 0.588235f, 0.478431f};
const ddVec3 DarkSeaGreen      = {0.560784f, 0.737255f, 0.560784f};
const ddVec3 DarkSlateBlue     = {0.282353f, 0.239216f, 0.545098f};
const ddVec3 DarkSlateGray     = {0.184314f, 0.309804f, 0.309804f};
const ddVec3 DarkTurquoise     = {0.000000f, 0.807843f, 0.819608f};
const ddVec3 DarkViolet        = {0.580392f, 0.000000f, 0.827451f};
const ddVec3 DeepPink          = {1.000000f, 0.078431f, 0.576471f};
const ddVec3 DeepSkyBlue       = {0.000000f, 0.749020f, 1.000000f};
const ddVec3 DimGray           = {0.411765f, 0.411765f, 0.411765f};
const ddVec3 DodgerBlue        = {0.117647f, 0.564706f, 1.000000f};
const ddVec3 FireBrick         = {0.698039f, 0.133333f, 0.133333f};
const ddVec3 FloralWhite       = {1.000000f, 0.980392f, 0.941176f};
const ddVec3 ForestGreen       = {0.133333f, 0.545098f, 0.133333f};
const ddVec3 Gainsboro         = {0.862745f, 0.862745f, 0.862745f};
const ddVec3 GhostWhite        = {0.972549f, 0.972549f, 1.000000f};
const ddVec3 Gold              = {1.000000f, 0.843137f, 0.000000f};
const ddVec3 GoldenRod         = {0.854902f, 0.647059f, 0.125490f};
const ddVec3 Gray              = {0.501961f, 0.501961f, 0.501961f};
const ddVec3 Green             = {0.000000f, 0.501961f, 0.000000f};
const ddVec3 GreenYellow       = {0.678431f, 1.000000f, 0.184314f};
const ddVec3 HoneyDew          = {0.941176f, 1.000000f, 0.941176f};
const ddVec3 HotPink           = {1.000000f, 0.411765f, 0.705882f};
const ddVec3 IndianRed         = {0.803922f, 0.360784f, 0.360784f};
const ddVec3 Indigo            = {0.294118f, 0.000000f, 0.509804f};
const ddVec3 Ivory             = {1.000000f, 1.000000f, 0.941176f};
const ddVec3 Khaki             = {0.941176f, 0.901961f, 0.549020f};
const ddVec3 Lavender          = {0.901961f, 0.901961f, 0.980392f};
const ddVec3 LavenderBlush     = {1.000000f, 0.941176f, 0.960784f};
const ddVec3 LawnGreen         = {0.486275f, 0.988235f, 0.000000f};
const ddVec3 LemonChiffon      = {1.000000f, 0.980392f, 0.803922f};
const ddVec3 LightBlue         = {0.678431f, 0.847059f, 0.901961f};
const ddVec3 LightCoral        = {0.941176f, 0.501961f, 0.501961f};
const ddVec3 LightCyan         = {0.878431f, 1.000000f, 1.000000f};
const ddVec3 LightGoldenYellow = {0.980392f, 0.980392f, 0.823529f};
const ddVec3 LightGray         = {0.827451f, 0.827451f, 0.827451f};
const ddVec3 LightGreen        = {0.564706f, 0.933333f, 0.564706f};
const ddVec3 LightPink         = {1.000000f, 0.713726f, 0.756863f};
const ddVec3 LightSalmon       = {1.000000f, 0.627451f, 0.478431f};
const ddVec3 LightSeaGreen     = {0.125490f, 0.698039f, 0.666667f};
const ddVec3 LightSkyBlue      = {0.529412f, 0.807843f, 0.980392f};
const ddVec3 LightSlateGray    = {0.466667f, 0.533333f, 0.600000f};
const ddVec3 LightSteelBlue    = {0.690196f, 0.768627f, 0.870588f};
const ddVec3 LightYellow       = {1.000000f, 1.000000f, 0.878431f};
const ddVec3 Lime              = {0.000000f, 1.000000f, 0.000000f};
const ddVec3 LimeGreen         = {0.196078f, 0.803922f, 0.196078f};
const ddVec3 Linen             = {0.980392f, 0.941176f, 0.901961f};
const ddVec3 Magenta           = {1.000000f, 0.000000f, 1.000000f};
const ddVec3 Maroon            = {0.501961f, 0.000000f, 0.000000f};
const ddVec3 MediumAquaMarine  = {0.400000f, 0.803922f, 0.666667f};
const ddVec3 MediumBlue        = {0.000000f, 0.000000f, 0.803922f};
const ddVec3 MediumOrchid      = {0.729412f, 0.333333f, 0.827451f};
const ddVec3 MediumPurple      = {0.576471f, 0.439216f, 0.858824f};
const ddVec3 MediumSeaGreen    = {0.235294f, 0.701961f, 0.443137f};
const ddVec3 MediumSlateBlue   = {0.482353f, 0.407843f, 0.933333f};
const ddVec3 MediumSpringGreen = {0.000000f, 0.980392f, 0.603922f};
const ddVec3 MediumTurquoise   = {0.282353f, 0.819608f, 0.800000f};
const ddVec3 MediumVioletRed   = {0.780392f, 0.082353f, 0.521569f};
const ddVec3 MidnightBlue      = {0.098039f, 0.098039f, 0.439216f};
const ddVec3 MintCream         = {0.960784f, 1.000000f, 0.980392f};
const ddVec3 MistyRose         = {1.000000f, 0.894118f, 0.882353f};
const ddVec3 Moccasin          = {1.000000f, 0.894118f, 0.709804f};
const ddVec3 NavajoWhite       = {1.000000f, 0.870588f, 0.678431f};
const ddVec3 Navy              = {0.000000f, 0.000000f, 0.501961f};
const ddVec3 OldLace           = {0.992157f, 0.960784f, 0.901961f};
const ddVec3 Olive             = {0.501961f, 0.501961f, 0.000000f};
const ddVec3 OliveDrab         = {0.419608f, 0.556863f, 0.137255f};
const ddVec3 Orange            = {1.000000f, 0.647059f, 0.000000f};
const ddVec3 OrangeRed         = {1.000000f, 0.270588f, 0.000000f};
const ddVec3 Orchid            = {0.854902f, 0.439216f, 0.839216f};
const ddVec3 PaleGoldenRod     = {0.933333f, 0.909804f, 0.666667f};
const ddVec3 PaleGreen         = {0.596078f, 0.984314f, 0.596078f};
const ddVec3 PaleTurquoise     = {0.686275f, 0.933333f, 0.933333f};
const ddVec3 PaleVioletRed     = {0.858824f, 0.439216f, 0.576471f};
const ddVec3 PapayaWhip        = {1.000000f, 0.937255f, 0.835294f};
const ddVec3 PeachPuff         = {1.000000f, 0.854902f, 0.725490f};
const ddVec3 Peru              = {0.803922f, 0.521569f, 0.247059f};
const ddVec3 Pink              = {1.000000f, 0.752941f, 0.796078f};
const ddVec3 Plum              = {0.866667f, 0.627451f, 0.866667f};
const ddVec3 PowderBlue        = {0.690196f, 0.878431f, 0.901961f};
const ddVec3 Purple            = {0.501961f, 0.000000f, 0.501961f};
const ddVec3 RebeccaPurple     = {0.400000f, 0.200000f, 0.600000f};
const ddVec3 Red               = {1.000000f, 0.000000f, 0.000000f};
const ddVec3 RosyBrown         = {0.737255f, 0.560784f, 0.560784f};
const ddVec3 RoyalBlue         = {0.254902f, 0.411765f, 0.882353f};
const ddVec3 SaddleBrown       = {0.545098f, 0.270588f, 0.074510f};
const ddVec3 Salmon            = {0.980392f, 0.501961f, 0.447059f};
const ddVec3 SandyBrown        = {0.956863f, 0.643137f, 0.376471f};
const ddVec3 SeaGreen          = {0.180392f, 0.545098f, 0.341176f};
const ddVec3 SeaShell          = {1.000000f, 0.960784f, 0.933333f};
const ddVec3 Sienna            = {0.627451f, 0.321569f, 0.176471f};
const ddVec3 Silver            = {0.752941f, 0.752941f, 0.752941f};
const ddVec3 SkyBlue           = {0.529412f, 0.807843f, 0.921569f};
const ddVec3 SlateBlue         = {0.415686f, 0.352941f, 0.803922f};
const ddVec3 SlateGray         = {0.439216f, 0.501961f, 0.564706f};
const ddVec3 Snow              = {1.000000f, 0.980392f, 0.980392f};
const ddVec3 SpringGreen       = {0.000000f, 1.000000f, 0.498039f};
const ddVec3 SteelBlue         = {0.274510f, 0.509804f, 0.705882f};
const ddVec3 Tan               = {0.823529f, 0.705882f, 0.549020f};
const ddVec3 Teal              = {0.000000f, 0.501961f, 0.501961f};
const ddVec3 Thistle           = {0.847059f, 0.749020f, 0.847059f};
const ddVec3 Tomato            = {1.000000f, 0.388235f, 0.278431f};
const ddVec3 Turquoise         = {0.250980f, 0.878431f, 0.815686f};
const ddVec3 Violet            = {0.933333f, 0.509804f, 0.933333f};
const ddVec3 Wheat             = {0.960784f, 0.870588f, 0.701961f};
const ddVec3 White             = {1.000000f, 1.000000f, 1.000000f};
const ddVec3 WhiteSmoke        = {0.960784f, 0.960784f, 0.960784f};
const ddVec3 Yellow            = {1.000000f, 1.000000f, 0.000000f};
const ddVec3 YellowGreen       = {0.603922f, 0.803922f, 0.196078f};
} // namespace colors
#endif // DEBUG_DRAW_NO_DEFAULT_COLORS

#if DEBUG_DRAW_CXX11_SUPPORTED
    #define DD_ALIGNED_BUFFER(name) alignas(16) static const std::uint8_t name[]
#else // !C++11
    #if defined(__GNUC__) // Clang & GCC
        #define DD_ALIGNED_BUFFER(name) static const std::uint8_t name[] __attribute__((aligned(16)))
    #elif defined(_MSC_VER) // Visual Studio
        #define DD_ALIGNED_BUFFER(name) __declspec(align(16)) static const std::uint8_t name[]
    #else // Unknown compiler
        #define DD_ALIGNED_BUFFER(name) static const std::uint8_t name[] /* hope for the best! */
    #endif // Compiler id
#endif // DEBUG_DRAW_CXX11_SUPPORTED


// ========================================================
// Internal Debug Draw queues and helper types/functions:
// ========================================================

struct DebugPoint
{
    std::int64_t expiryDateMillis;
    ddVec3       position;
    ddVec3       color;
    float        size;
    bool         depthEnabled;
};

struct DebugLine
{
    std::int64_t expiryDateMillis;
    ddVec3       posFrom;
    ddVec3       posTo;
    ddVec3       color;
    bool         depthEnabled;
};

struct InternalContext DD_EXPLICIT_CONTEXT_ONLY(: public OpaqueContextType)
{
    int                vertexBufferUsed;
    int                debugPointsCount;
    int                debugLinesCount;
    std::int64_t       currentTimeMillis;                           // Latest time value (in milliseconds) from dd::flush().
    RenderInterface *  renderInterface;                             // Ref to the external renderer. Can be null for a no-op debug draw.
    DrawVertex         vertexBuffer[DEBUG_DRAW_VERTEX_BUFFER_SIZE]; // Vertex buffer we use to expand the lines/points before calling on RenderInterface.
    DebugPoint         debugPoints[DEBUG_DRAW_MAX_POINTS];          // 3D debug points queue.
    DebugLine          debugLines[DEBUG_DRAW_MAX_LINES];            // 3D debug lines queue.

    InternalContext(RenderInterface * renderer)
        : vertexBufferUsed(0)
        , debugPointsCount(0)
        , debugLinesCount(0)
        , currentTimeMillis(0)
        , renderInterface(renderer)
    { }
};

// ========================================================
// Library context mode selection:
// ========================================================

#if (defined(DEBUG_DRAW_PER_THREAD_CONTEXT) && defined(DEBUG_DRAW_EXPLICIT_CONTEXT))
    #error "DEBUG_DRAW_PER_THREAD_CONTEXT and DEBUG_DRAW_EXPLICIT_CONTEXT are mutually exclusive!"
#endif // DEBUG_DRAW_PER_THREAD_CONTEXT && DEBUG_DRAW_EXPLICIT_CONTEXT

#if defined(DEBUG_DRAW_EXPLICIT_CONTEXT)
    //
    // Explicit context passed as argument
    //
    #define DD_CONTEXT static_cast<InternalContext *>(ctx)
#elif defined(DEBUG_DRAW_PER_THREAD_CONTEXT)
    //
    // Per-thread global context (MT safe)
    //
    #if defined(__GNUC__) || defined(__clang__) // GCC/Clang
        #define DD_THREAD_LOCAL static __thread
    #elif defined(_MSC_VER) // Visual Studio
        #define DD_THREAD_LOCAL static __declspec(thread)
    #else // Try C++11 thread_local
        #if DEBUG_DRAW_CXX11_SUPPORTED
            #define DD_THREAD_LOCAL static thread_local
        #else // !DEBUG_DRAW_CXX11_SUPPORTED
            #error "Unsupported compiler - unknown TLS model"
        #endif // DEBUG_DRAW_CXX11_SUPPORTED
    #endif // TLS model
    DD_THREAD_LOCAL InternalContext * s_threadContext = nullptr;
    #define DD_CONTEXT s_threadContext
    #undef DD_THREAD_LOCAL
#else // Debug Draw context selection
    //
    // Global static context (single threaded operation)
    //
    static InternalContext * s_globalContext = nullptr;
    #define DD_CONTEXT s_globalContext
#endif // Debug Draw context selection

// ========================================================

static inline float floatAbs(float x)       { return fabsf(x); }
static inline float floatSin(float radians) { return sinf(radians); }
static inline float floatCos(float radians) { return cosf(radians); }
static inline float floatInvSqrt(float x)   { return (1.0f / sqrtf(x)); }

// ========================================================
// ddVec3 helpers:
// ========================================================

enum VecElements { X, Y, Z, W };

static inline void vecSet(ddVec3_Out dest, const float x, const float y, const float z)
{
    dest[X] = x;
    dest[Y] = y;
    dest[Z] = z;
}

static inline void vecCopy(ddVec3_Out dest, ddVec3_In src)
{
    dest[X] = src[X];
    dest[Y] = src[Y];
    dest[Z] = src[Z];
}

static inline void vecAdd(ddVec3_Out result, ddVec3_In a, ddVec3_In b)
{
    result[X] = a[X] + b[X];
    result[Y] = a[Y] + b[Y];
    result[Z] = a[Z] + b[Z];
}

static inline void vecSub(ddVec3_Out result, ddVec3_In a, ddVec3_In b)
{
    result[X] = a[X] - b[X];
    result[Y] = a[Y] - b[Y];
    result[Z] = a[Z] - b[Z];
}

static inline void vecScale(ddVec3_Out result, ddVec3_In v, const float s)
{
    result[X] = v[X] * s;
    result[Y] = v[Y] * s;
    result[Z] = v[Z] * s;
}

static inline void vecNormalize(ddVec3_Out result, ddVec3_In v)
{
    const float lenSqr = v[X] * v[X] + v[Y] * v[Y] + v[Z] * v[Z];
    const float invLen = floatInvSqrt(lenSqr);
    result[X] = v[X] * invLen;
    result[Y] = v[Y] * invLen;
    result[Z] = v[Z] * invLen;
}

static inline void vecOrthogonalBasis(ddVec3_Out left, ddVec3_Out up, ddVec3_In v)
{
    // Produces two orthogonal vectors for normalized vector v.
    float lenSqr, invLen;
    if (floatAbs(v[Z]) > 0.7f)
    {
        lenSqr  = v[Y] * v[Y] + v[Z] * v[Z];
        invLen  = floatInvSqrt(lenSqr);
        up[X]   = 0.0f;
        up[Y]   =  v[Z] * invLen;
        up[Z]   = -v[Y] * invLen;
        left[X] = lenSqr * invLen;
        left[Y] = -v[X] * up[Z];
        left[Z] =  v[X] * up[Y];
    }
    else
    {
        lenSqr  = v[X] * v[X] + v[Y] * v[Y];
        invLen  = floatInvSqrt(lenSqr);
        left[X] = -v[Y] * invLen;
        left[Y] =  v[X] * invLen;
        left[Z] = 0.0f;
        up[X]   = -v[Z] * left[Y];
        up[Y]   =  v[Z] * left[X];
        up[Z]   = lenSqr * invLen;
    }
}

// ========================================================
// ddMat4x4 helpers:
// ========================================================

static inline void matTransformPointXYZ(ddVec3_Out result, ddVec3_In p, ddMat4x4_In m)
{
    result[X] = (m[0] * p[X]) + (m[4] * p[Y]) + (m[8]  * p[Z]) + m[12]; // p[W] assumed to be 1
    result[Y] = (m[1] * p[X]) + (m[5] * p[Y]) + (m[9]  * p[Z]) + m[13];
    result[Z] = (m[2] * p[X]) + (m[6] * p[Y]) + (m[10] * p[Z]) + m[14];
}

static inline void matTransformPointXYZW(float result[4], ddVec3_In p, ddMat4x4_In m)
{
    result[X] = (m[0] * p[X]) + (m[4] * p[Y]) + (m[8]  * p[Z]) + m[12]; // p[W] assumed to be 1
    result[Y] = (m[1] * p[X]) + (m[5] * p[Y]) + (m[9]  * p[Z]) + m[13];
    result[Z] = (m[2] * p[X]) + (m[6] * p[Y]) + (m[10] * p[Z]) + m[14];
    result[W] = (m[3] * p[X]) + (m[7] * p[Y]) + (m[11] * p[Z]) + m[15];
}

static inline float matTransformPointXYZW2(ddVec3_Out result, const float p[3], ddMat4x4_In m)
{
    result[X] = (m[0] * p[X]) + (m[4] * p[Y]) + (m[8]  * p[Z]) + m[12]; // p[W] assumed to be 1
    result[Y] = (m[1] * p[X]) + (m[5] * p[Y]) + (m[9]  * p[Z]) + m[13];
    result[Z] = (m[2] * p[X]) + (m[6] * p[Y]) + (m[10] * p[Z]) + m[14];
    float rw  = (m[3] * p[X]) + (m[7] * p[Y]) + (m[11] * p[Z]) + m[15];
    return rw;
}

// ========================================================
// Misc local functions for draw queue management:
// ========================================================

enum DrawMode
{
    DrawModePoints,
    DrawModeLines
};

static void flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) const DrawMode mode, const bool depthEnabled)
{
    if (DD_CONTEXT->vertexBufferUsed == 0)
    {
        return;
    }

    switch (mode)
    {
    case DrawModePoints :
        DD_CONTEXT->renderInterface->drawPointList(DD_CONTEXT->vertexBuffer,
                                                   DD_CONTEXT->vertexBufferUsed,
                                                   depthEnabled);
        break;
    case DrawModeLines :
        DD_CONTEXT->renderInterface->drawLineList(DD_CONTEXT->vertexBuffer,
                                                  DD_CONTEXT->vertexBufferUsed,
                                                  depthEnabled);
        break;
    } // switch (mode)

    DD_CONTEXT->vertexBufferUsed = 0;
}

static void pushPointVert(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) const DebugPoint & point)
{
    // Make room for one more vert:
    if ((DD_CONTEXT->vertexBufferUsed + 1) >= DEBUG_DRAW_VERTEX_BUFFER_SIZE)
    {
        flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DrawModePoints, point.depthEnabled);
    }

    DrawVertex & v = DD_CONTEXT->vertexBuffer[DD_CONTEXT->vertexBufferUsed++];
    v.point.x      = point.position[X];
    v.point.y      = point.position[Y];
    v.point.z      = point.position[Z];
    v.point.r      = point.color[X];
    v.point.g      = point.color[Y];
    v.point.b      = point.color[Z];
    v.point.size   = point.size;
}

static void pushLineVert(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) const DebugLine & line)
{
    // Make room for two more verts:
    if ((DD_CONTEXT->vertexBufferUsed + 2) >= DEBUG_DRAW_VERTEX_BUFFER_SIZE)
    {
        flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DrawModeLines, line.depthEnabled);
    }

    DrawVertex & v0 = DD_CONTEXT->vertexBuffer[DD_CONTEXT->vertexBufferUsed++];
    DrawVertex & v1 = DD_CONTEXT->vertexBuffer[DD_CONTEXT->vertexBufferUsed++];

    v0.line.x = line.posFrom[X];
    v0.line.y = line.posFrom[Y];
    v0.line.z = line.posFrom[Z];
    v0.line.r = line.color[X];
    v0.line.g = line.color[Y];
    v0.line.b = line.color[Z];

    v1.line.x = line.posTo[X];
    v1.line.y = line.posTo[Y];
    v1.line.z = line.posTo[Z];
    v1.line.r = line.color[X];
    v1.line.g = line.color[Y];
    v1.line.b = line.color[Z];
}

static void drawDebugPoints(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx))
{
    const int count = DD_CONTEXT->debugPointsCount;
    if (count == 0)
    {
        return;
    }

    const DebugPoint * const debugPoints = DD_CONTEXT->debugPoints;

    //
    // First pass, points with depth test ENABLED:
    //
    int numDepthlessPoints = 0;
    for (int i = 0; i < count; ++i)
    {
        const DebugPoint & point = debugPoints[i];
        if (point.depthEnabled)
        {
            pushPointVert(DD_EXPLICIT_CONTEXT_ONLY(ctx,) point);
        }
        numDepthlessPoints += !point.depthEnabled;
    }
    flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DrawModePoints, true);

    //
    // Second pass draws points with depth DISABLED:
    //
    if (numDepthlessPoints > 0)
    {
        for (int i = 0; i < count; ++i)
        {
            const DebugPoint & point = debugPoints[i];
            if (!point.depthEnabled)
            {
                pushPointVert(DD_EXPLICIT_CONTEXT_ONLY(ctx,) point);
            }
        }
        flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DrawModePoints, false);
    }
}

static void drawDebugLines(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx))
{
    const int count = DD_CONTEXT->debugLinesCount;
    if (count == 0)
    {
        return;
    }

    const DebugLine * const debugLines = DD_CONTEXT->debugLines;

    //
    // First pass, lines with depth test ENABLED:
    //
    int numDepthlessLines = 0;
    for (int i = 0; i < count; ++i)
    {
        const DebugLine & line = debugLines[i];
        if (line.depthEnabled)
        {
            pushLineVert(DD_EXPLICIT_CONTEXT_ONLY(ctx,) line);
        }
        numDepthlessLines += !line.depthEnabled;
    }
    flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DrawModeLines, true);

    //
    // Second pass draws lines with depth DISABLED:
    //
    if (numDepthlessLines > 0)
    {
        for (int i = 0; i < count; ++i)
        {
            const DebugLine & line = debugLines[i];
            if (!line.depthEnabled)
            {
                pushLineVert(DD_EXPLICIT_CONTEXT_ONLY(ctx,) line);
            }
        }
        flushDebugVerts(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DrawModeLines, false);
    }
}

template<typename T>
static void clearDebugQueue(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) T * queue, int & queueCount)
{
    const std::int64_t time = DD_CONTEXT->currentTimeMillis;
    if (time == 0)
    {
        queueCount = 0;
        return;
    }

    int index = 0;
    T * pElem = queue;

    // Concatenate elements that still need to be draw on future frames:
    for (int i = 0; i < queueCount; ++i, ++pElem)
    {
        if (pElem->expiryDateMillis > time)
        {
            if (index != i)
            {
                queue[index] = *pElem;
            }
            ++index;
        }
    }

    queueCount = index;
}


// ========================================================
// Public Debug Draw interface:
// ========================================================

bool initialize(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle * outCtx,) RenderInterface * renderer)
{
    if (renderer == nullptr)
    {
        return false;
    }

    void * buffer = DD_MALLOC(sizeof(InternalContext));
    if (buffer == nullptr)
    {
        return false;
    }

    InternalContext * newCtx = ::new(buffer) InternalContext(renderer);

    #ifdef DEBUG_DRAW_EXPLICIT_CONTEXT
    if ((*outCtx) != nullptr) { shutdown(*outCtx); }
    (*outCtx) = newCtx;
    #else // !DEBUG_DRAW_EXPLICIT_CONTEXT
    if (DD_CONTEXT != nullptr) { shutdown(); }
    DD_CONTEXT = newCtx;
    #endif // DEBUG_DRAW_EXPLICIT_CONTEXT
    return true;
}

void shutdown(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx))
{
    if (DD_CONTEXT != nullptr)
    {
        DD_CONTEXT->~InternalContext(); // Destroy first
        DD_MFREE(DD_CONTEXT);

        #ifndef DEBUG_DRAW_EXPLICIT_CONTEXT
        DD_CONTEXT = nullptr;
        #endif // DEBUG_DRAW_EXPLICIT_CONTEXT
    }
}

bool isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx))
{
    return (DD_CONTEXT != nullptr && DD_CONTEXT->renderInterface != nullptr);
}

bool hasPendingDraws(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx))
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return false;
    }
    return (DD_CONTEXT->debugPointsCount + DD_CONTEXT->debugLinesCount) > 0;
}

void flush(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) const std::int64_t currTimeMillis, const std::uint32_t flags)
{
    if (!hasPendingDraws(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    // Save the last know time value for next dd::line/dd::point calls.
    DD_CONTEXT->currentTimeMillis = currTimeMillis;

    // Let the user set common render states.
    DD_CONTEXT->renderInterface->beginDraw();

    // Issue the render calls:
    if (flags & FlushLines)  { drawDebugLines(DD_EXPLICIT_CONTEXT_ONLY(ctx));   }
    if (flags & FlushPoints) { drawDebugPoints(DD_EXPLICIT_CONTEXT_ONLY(ctx));  }
    
    // And cleanup if needed.
    DD_CONTEXT->renderInterface->endDraw();

    // Remove all expired objects, regardless of draw flags:
    clearDebugQueue(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DD_CONTEXT->debugPoints,  DD_CONTEXT->debugPointsCount);
    clearDebugQueue(DD_EXPLICIT_CONTEXT_ONLY(ctx,) DD_CONTEXT->debugLines,   DD_CONTEXT->debugLinesCount);
}

void clear(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx))
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    DD_CONTEXT->vertexBufferUsed  = 0;
    DD_CONTEXT->debugPointsCount  = 0;
    DD_CONTEXT->debugLinesCount   = 0;
}

void point(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In pos, ddVec3_In color,
           const float size, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    if (DD_CONTEXT->debugPointsCount == DEBUG_DRAW_MAX_POINTS)
    {
        DEBUG_DRAW_OVERFLOWED("DEBUG_DRAW_MAX_POINTS limit reached! Dropping further debug point draws.");
        return;
    }

    DebugPoint & point     = DD_CONTEXT->debugPoints[DD_CONTEXT->debugPointsCount++];
    point.expiryDateMillis = DD_CONTEXT->currentTimeMillis + durationMillis;
    point.depthEnabled     = depthEnabled;
    point.size             = size;

    vecCopy(point.position, pos);
    vecCopy(point.color, color);
}

void line(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In from, ddVec3_In to,
          ddVec3_In color, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    if (DD_CONTEXT->debugLinesCount == DEBUG_DRAW_MAX_LINES)
    {
        DEBUG_DRAW_OVERFLOWED("DEBUG_DRAW_MAX_LINES limit reached! Dropping further debug line draws.");
        return;
    }

    DebugLine & line      = DD_CONTEXT->debugLines[DD_CONTEXT->debugLinesCount++];
    line.expiryDateMillis = DD_CONTEXT->currentTimeMillis + durationMillis;
    line.depthEnabled     = depthEnabled;

    vecCopy(line.posFrom, from);
    vecCopy(line.posTo, to);
    vecCopy(line.color, color);
}

void axisTriad(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddMat4x4_In transform, const float size,
               const float length, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 p0, p1, p2, p3;
    ddVec3 xEnd, yEnd, zEnd;
    ddVec3 origin, cR, cG, cB;

    vecSet(cR, 1.0f, 0.0f, 0.0f);
    vecSet(cG, 0.0f, 1.0f, 0.0f);
    vecSet(cB, 0.0f, 0.0f, 1.0f);

    vecSet(origin, 0.0f, 0.0f, 0.0f);
    vecSet(xEnd, length, 0.0f, 0.0f);
    vecSet(yEnd, 0.0f, length, 0.0f);
    vecSet(zEnd, 0.0f, 0.0f, length);

    matTransformPointXYZ(p0, origin, transform);
    matTransformPointXYZ(p1, xEnd, transform);
    matTransformPointXYZ(p2, yEnd, transform);
    matTransformPointXYZ(p3, zEnd, transform);

    arrow(DD_EXPLICIT_CONTEXT_ONLY(ctx,) p0, p1, cR, size, durationMillis, depthEnabled); // X: red axis
    arrow(DD_EXPLICIT_CONTEXT_ONLY(ctx,) p0, p2, cG, size, durationMillis, depthEnabled); // Y: green axis
    arrow(DD_EXPLICIT_CONTEXT_ONLY(ctx,) p0, p3, cB, size, durationMillis, depthEnabled); // Z: blue axis
}

void arrow(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In from, ddVec3_In to, ddVec3_In color,
           const float size, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    static const float arrowStep = 30.0f; // In degrees
    static const float arrowSin[45] = {
        0.0f, 0.5f, 0.866025f, 1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866025f,
        -1.0f, -0.866025f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    static const float arrowCos[45] = {
        1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866026f, -1.0f, -0.866025f, -0.5f, 0.0f,
        0.5f, 0.866026f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    // Body line:
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) from, to, color, durationMillis, depthEnabled);

    // Aux vectors to compute the arrowhead:
    ddVec3 up, right, forward;
    vecSub(forward, to, from);
    vecNormalize(forward, forward);
    vecOrthogonalBasis(right, up, forward);
    vecScale(forward, forward, size);

    // Arrowhead is a cone (sin/cos tables used here):
    float degrees = 0.0f;
    for (int i = 0; degrees < 360.0f; degrees += arrowStep, ++i)
    {
        float scale;
        ddVec3 v1, v2, temp;

        scale = 0.5f * size * arrowCos[i];
        vecScale(temp, right, scale);
        vecSub(v1, to, forward);
        vecAdd(v1, v1, temp);

        scale = 0.5f * size * arrowSin[i];
        vecScale(temp, up, scale);
        vecAdd(v1, v1, temp);

        scale = 0.5f * size * arrowCos[i + 1];
        vecScale(temp, right, scale);
        vecSub(v2, to, forward);
        vecAdd(v2, v2, temp);

        scale = 0.5f * size * arrowSin[i + 1];
        vecScale(temp, up, scale);
        vecAdd(v2, v2, temp);

        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) v1, to, color, durationMillis, depthEnabled);
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) v1, v2, color, durationMillis, depthEnabled);
    }
}

void cross(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In center, const float length,
           const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 from, to;
    ddVec3 cR, cG, cB;

    vecSet(cR, 1.0f, 0.0f, 0.0f);
    vecSet(cG, 0.0f, 1.0f, 0.0f);
    vecSet(cB, 0.0f, 0.0f, 1.0f);

    const float cx = center[X];
    const float cy = center[Y];
    const float cz = center[Z];
    const float hl = length * 0.5f; // Half on each side.

    // Red line: X - length/2 to X + length/2
    vecSet(from, cx - hl, cy, cz);
    vecSet(to,   cx + hl, cy, cz);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) from, to, cR, durationMillis, depthEnabled);

    // Green line: Y - length/2 to Y + length/2
    vecSet(from, cx, cy - hl, cz);
    vecSet(to,   cx, cy + hl, cz);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) from, to, cG, durationMillis, depthEnabled);

    // Blue line: Z - length/2 to Z + length/2
    vecSet(from, cx, cy, cz - hl);
    vecSet(to,   cx, cy, cz + hl);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) from, to, cB, durationMillis, depthEnabled);
}

void circle(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In center, ddVec3_In planeNormal, ddVec3_In color,
            const float radius, const float numSteps, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 left, up;
    ddVec3 point, lastPoint;

    vecOrthogonalBasis(left, up, planeNormal);

    vecScale(up, up, radius);
    vecScale(left, left, radius);
    vecAdd(lastPoint, center, up);

    for (int i = 1; i <= numSteps; ++i)
    {
        const float radians = TAU * i / numSteps;

        ddVec3 vs, vc;
        vecScale(vs, left, floatSin(radians));
        vecScale(vc, up,   floatCos(radians));

        vecAdd(point, center, vs);
        vecAdd(point, point,  vc);

        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) lastPoint, point, color, durationMillis, depthEnabled);
        vecCopy(lastPoint, point);
    }
}

void plane(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In center, ddVec3_In planeNormal, ddVec3_In planeColor,
           ddVec3_In normalVecColor, const float planeScale, const float normalVecScale, const int durationMillis,
           const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 v1, v2, v3, v4;
    ddVec3 tangent, bitangent;
    vecOrthogonalBasis(tangent, bitangent, planeNormal);

    // A little bit of preprocessor voodoo to make things more interesting :P
    #define DD_PLANE_V(v, op1, op2) \
    v[X] = (center[X] op1 (tangent[X] * planeScale) op2 (bitangent[X] * planeScale)); \
    v[Y] = (center[Y] op1 (tangent[Y] * planeScale) op2 (bitangent[Y] * planeScale)); \
    v[Z] = (center[Z] op1 (tangent[Z] * planeScale) op2 (bitangent[Z] * planeScale))
    DD_PLANE_V(v1, -, -);
    DD_PLANE_V(v2, +, -);
    DD_PLANE_V(v3, +, +);
    DD_PLANE_V(v4, -, +);
    #undef DD_PLANE_V

    // Draw the wireframe plane quadrilateral:
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) v1, v2, planeColor, durationMillis, depthEnabled);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) v2, v3, planeColor, durationMillis, depthEnabled);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) v3, v4, planeColor, durationMillis, depthEnabled);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) v4, v1, planeColor, durationMillis, depthEnabled);

    // Optionally add a line depicting the plane normal:
    if (normalVecScale != 0.0f)
    {
        ddVec3 normalVec;
        normalVec[X] = (planeNormal[X] * normalVecScale) + center[X];
        normalVec[Y] = (planeNormal[Y] * normalVecScale) + center[Y];
        normalVec[Z] = (planeNormal[Z] * normalVecScale) + center[Z];
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) center, normalVec, normalVecColor, durationMillis, depthEnabled);
    }
}

void sphere(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In center, ddVec3_In color,
            const float radius, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    static const int stepSize = 15;
    ddVec3 cache[360 / stepSize];
    ddVec3 radiusVec;

    vecSet(radiusVec, 0.0f, 0.0f, radius);
    vecAdd(cache[0], center, radiusVec);

    for (int n = 1; n < arrayLength(cache); ++n)
    {
        vecCopy(cache[n], cache[0]);
    }

    ddVec3 lastPoint, temp;
    for (int i = stepSize; i <= 360; i += stepSize)
    {
        const float s = floatSin(degreesToRadians(i));
        const float c = floatCos(degreesToRadians(i));

        lastPoint[X] = center[X];
        lastPoint[Y] = center[Y] + radius * s;
        lastPoint[Z] = center[Z] + radius * c;

        for (int n = 0, j = stepSize; j <= 360; j += stepSize, ++n)
        {
            temp[X] = center[X] + floatSin(degreesToRadians(j)) * radius * s;
            temp[Y] = center[Y] + floatCos(degreesToRadians(j)) * radius * s;
            temp[Z] = lastPoint[Z];

            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) lastPoint, temp, color, durationMillis, depthEnabled);
            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) lastPoint, cache[n], color, durationMillis, depthEnabled);

            vecCopy(cache[n], lastPoint);
            vecCopy(lastPoint, temp);
        }
    }
}

void cone(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In apex, ddVec3_In dir, ddVec3_In color,
          const float baseRadius, const float apexRadius, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    static const int stepSize = 20;
    ddVec3 axis[3];
    ddVec3 top, temp0, temp1, temp2;
    ddVec3 p1, p2, lastP1, lastP2;

    vecCopy(axis[2], dir);
    vecNormalize(axis[2], axis[2]);
    vecOrthogonalBasis(axis[0], axis[1], axis[2]);

    axis[1][X] = -axis[1][X];
    axis[1][Y] = -axis[1][Y];
    axis[1][Z] = -axis[1][Z];

    vecAdd(top, apex, dir);
    vecScale(temp1, axis[1], baseRadius);
    vecAdd(lastP2, top, temp1);

    if (apexRadius == 0.0f)
    {
        for (int i = stepSize; i <= 360; i += stepSize)
        {
            vecScale(temp1, axis[0], floatSin(degreesToRadians(i)));
            vecScale(temp2, axis[1], floatCos(degreesToRadians(i)));
            vecAdd(temp0, temp1, temp2);

            vecScale(temp0, temp0, baseRadius);
            vecAdd(p2, top, temp0);

            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) lastP2, p2, color, durationMillis, depthEnabled);
            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) p2, apex, color, durationMillis, depthEnabled);

            vecCopy(lastP2, p2);
        }
    }
    else // A degenerate cone with open apex:
    {
        vecScale(temp1, axis[1], apexRadius);
        vecAdd(lastP1, apex, temp1);

        for (int i = stepSize; i <= 360; i += stepSize)
        {
            vecScale(temp1, axis[0], floatSin(degreesToRadians(i)));
            vecScale(temp2, axis[1], floatCos(degreesToRadians(i)));
            vecAdd(temp0, temp1, temp2);

            vecScale(temp1, temp0, apexRadius);
            vecScale(temp2, temp0, baseRadius);

            vecAdd(p1, apex, temp1);
            vecAdd(p2, top,  temp2);

            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) lastP1, p1, color, durationMillis, depthEnabled);
            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) lastP2, p2, color, durationMillis, depthEnabled);
            line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) p1, p2, color, durationMillis, depthEnabled);

            vecCopy(lastP1, p1);
            vecCopy(lastP2, p2);
        }
    }
}

void box(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) const ddVec3 points[8], ddVec3_In color,
         const int durationMillis, const bool depthEnabled)
{
    // Build the lines from points using clever indexing tricks:
    // (& 3 is a fancy way of doing % 4, but avoids the expensive modulo operation)
    for (int i = 0; i < 4; ++i)
    {
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) points[i], points[(i + 1) & 3], color, durationMillis, depthEnabled);
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) points[4 + i], points[4 + ((i + 1) & 3)], color, durationMillis, depthEnabled);
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) points[i], points[4 + i], color, durationMillis, depthEnabled);
    }
}

void box(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In center, ddVec3_In color, const float width,
         const float height, const float depth, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    const float cx = center[X];
    const float cy = center[Y];
    const float cz = center[Z];
    const float w  = width  * 0.5f;
    const float h  = height * 0.5f;
    const float d  = depth  * 0.5f;

    // Create all the 8 points:
    ddVec3 points[8];
    #define DD_BOX_V(v, op1, op2, op3) \
    v[X] = cx op1 w; \
    v[Y] = cy op2 h; \
    v[Z] = cz op3 d
    DD_BOX_V(points[0], -, +, +);
    DD_BOX_V(points[1], -, +, -);
    DD_BOX_V(points[2], +, +, -);
    DD_BOX_V(points[3], +, +, +);
    DD_BOX_V(points[4], -, -, +);
    DD_BOX_V(points[5], -, -, -);
    DD_BOX_V(points[6], +, -, -);
    DD_BOX_V(points[7], +, -, +);
    #undef DD_BOX_V

    box(DD_EXPLICIT_CONTEXT_ONLY(ctx,) points, color, durationMillis, depthEnabled);
}

void aabb(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In mins, ddVec3_In maxs,
          ddVec3_In color, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 bb[2];
    ddVec3 points[8];

    vecCopy(bb[0], mins);
    vecCopy(bb[1], maxs);

    // Expand min/max bounds:
    for (int i = 0; i < arrayLength(points); ++i)
    {
        points[i][X] = bb[(i ^ (i >> 1)) & 1][X];
        points[i][Y] = bb[(i >> 1) & 1][Y];
        points[i][Z] = bb[(i >> 2) & 1][Z];
    }

    // Build the lines:
    box(DD_EXPLICIT_CONTEXT_ONLY(ctx,) points, color, durationMillis, depthEnabled);
}

void frustum(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddMat4x4_In invClipMatrix,
             ddVec3_In color, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    // Start with the standard clip volume, then bring it back to world space.
    static const float planes[8][3] = {
        // near plane
        { -1.0f, -1.0f, -1.0f }, {  1.0f, -1.0f, -1.0f },
        {  1.0f,  1.0f, -1.0f }, { -1.0f,  1.0f, -1.0f },
        // far plane
        { -1.0f, -1.0f,  1.0f }, {  1.0f, -1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f }, { -1.0f,  1.0f,  1.0f }
    };

    ddVec3 points[8];
    float wCoords[8];

    // Transform the planes by the inverse clip matrix:
    for (int i = 0; i < arrayLength(planes); ++i)
    {
        wCoords[i] = matTransformPointXYZW2(points[i], planes[i], invClipMatrix);
    }

    // Divide by the W component of each:
    for (int i = 0; i < arrayLength(planes); ++i)
    {
        // But bail if any W ended up as zero.
        if (floatAbs(wCoords[W]) < FloatEpsilon)
        {
            return;
        }

        points[i][X] /= wCoords[i];
        points[i][Y] /= wCoords[i];
        points[i][Z] /= wCoords[i];
    }

    // Connect the dots:
    box(DD_EXPLICIT_CONTEXT_ONLY(ctx,) points, color, durationMillis, depthEnabled);
}

void vertexNormal(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In origin, ddVec3_In normal,
                  const float length, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 normalVec;
    ddVec3 normalColor;

    vecSet(normalColor, 1.0f, 1.0f, 1.0f);

    normalVec[X] = (normal[X] * length) + origin[X];
    normalVec[Y] = (normal[Y] * length) + origin[Y];
    normalVec[Z] = (normal[Z] * length) + origin[Z];

    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) origin, normalVec, normalColor, durationMillis, depthEnabled);
}

void tangentBasis(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) ddVec3_In origin, ddVec3_In normal, ddVec3_In tangent,
                  ddVec3_In bitangent, const float lengths, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 cN, cT, cB;
    ddVec3 vN, vT, vB;

    vecSet(cN, 1.0f, 1.0f, 1.0f); // Vertex normals are WHITE
    vecSet(cT, 1.0f, 1.0f, 0.0f); // Tangents are YELLOW
    vecSet(cB, 1.0f, 0.0f, 1.0f); // Bi-tangents are MAGENTA

    vN[X] = (normal[X] * lengths) + origin[X];
    vN[Y] = (normal[Y] * lengths) + origin[Y];
    vN[Z] = (normal[Z] * lengths) + origin[Z];

    vT[X] = (tangent[X] * lengths) + origin[X];
    vT[Y] = (tangent[Y] * lengths) + origin[Y];
    vT[Z] = (tangent[Z] * lengths) + origin[Z];

    vB[X] = (bitangent[X] * lengths) + origin[X];
    vB[Y] = (bitangent[Y] * lengths) + origin[Y];
    vB[Z] = (bitangent[Z] * lengths) + origin[Z];

    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) origin, vN, cN, durationMillis, depthEnabled);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) origin, vT, cT, durationMillis, depthEnabled);
    line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) origin, vB, cB, durationMillis, depthEnabled);
}

void xzSquareGrid(DD_EXPLICIT_CONTEXT_ONLY(ContextHandle ctx,) const float mins, const float maxs, const float y,
                  const float step, ddVec3_In color, const int durationMillis, const bool depthEnabled)
{
    if (!isInitialized(DD_EXPLICIT_CONTEXT_ONLY(ctx)))
    {
        return;
    }

    ddVec3 from, to;
    for (float i = mins; i <= maxs; i += step)
    {
        // Horizontal line (along the X)
        vecSet(from, mins, y, i);
        vecSet(to,   maxs, y, i);
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) from, to, color, durationMillis, depthEnabled);

        // Vertical line (along the Z)
        vecSet(from, i, y, mins);
        vecSet(to,   i, y, maxs);
        line(DD_EXPLICIT_CONTEXT_ONLY(ctx,) from, to, color, durationMillis, depthEnabled);
    }
}

// ========================================================
// RenderInterface stubs:
// ========================================================

RenderInterface::~RenderInterface()                                              { }
void RenderInterface::beginDraw()                                                { }
void RenderInterface::endDraw()                                                  { }
void RenderInterface::drawPointList(const DrawVertex *, int, bool)               { }
void RenderInterface::drawLineList(const DrawVertex *, int, bool)                { }

} // namespace dd

#undef DD_CONTEXT
#undef DD_MALLOC
#undef DD_MFREE

// ================ End of implementation =================
#endif // DEBUG_DRAW_IMPLEMENTATION
// ================ End of implementation =================

class DDRenderInterfaceCoreGL final
    : public dd::RenderInterface {
public:
    DDRenderInterfaceCoreGL();
    ~DDRenderInterfaceCoreGL() override;
private:
    void drawPointList(const dd::DrawVertex *, int, bool) override;

    void drawLineList(const dd::DrawVertex *, int, bool) override;

private:
    GLuint linePointProgram{};

    GLuint linePointVAO{};
    GLuint linePointVBO{};

    void setupShaderPrograms();
    void setupVertexBuffers();
};
