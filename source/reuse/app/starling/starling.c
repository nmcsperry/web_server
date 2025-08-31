#include "../../base/base_include.h"
#include "../../opengl/opengl_include.h"
#include "../../io/io_include.h"

#include "bitmap.c"

opengl_gl * GL;

typedef struct vertex_format {
    f32_v3 Position;
    f32 Shade;
    f32_v2 UV;
    i32 TextureIndex;
} vertex_format;

u32 MatrixPosition;

void EntryHook()
{
    memory_arena * Arena = ArenaCreate(0);

    opengl_context Context = OpenGLInit(0);
    GL = Context.Gl;

    OpenGLWindowCreate(Arena, Context, 400, 400);

    bitmap TerrainBMP = LoadBitmapFromFile(Arena, "mc_terrain.bmp");
    str8 VertexShaderSource = FileInputFilename("vertex_shader.glsl", Arena);
    str8 FragmentShaderSource = FileInputFilename("fragment_shader.glsl", Arena);

    u32 Texture;
    GL->GenTextures(1, &Texture);
    GL->ActiveTexture(GL_TEXTURE0);
    GL->BindTexture(GL_TEXTURE_2D, Texture);
    GL->TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TerrainBMP.Width, TerrainBMP.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, TerrainBMP.Data);
    GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    u32 VertexShader = GL->CreateShader(GL_VERTEX_SHADER);
    GL->ShaderSource(VertexShader, 1, &VertexShaderSource.Data, &VertexShaderSource.Count);
    GL->CompileShader(VertexShader);

    u32 FragmentShader = GL->CreateShader(GL_FRAGMENT_SHADER);
    GL->ShaderSource(FragmentShader, 1, &FragmentShaderSource.Data, &FragmentShaderSource.Count);
    GL->CompileShader(FragmentShader);

    u32 Program = GL->CreateProgram();
    GL->AttachShader(Program, VertexShader);
    GL->AttachShader(Program, FragmentShader);
    GL->LinkProgram(Program);

    GL->UseProgram(Program);

    f32 XS = 0.7, YS = 1.0, ZS = 0.8;
    i32 XF0 = 3, XF1 = 3, YF0 = 2, YF1 = 0, ZF0 = 3, ZF1 = 3;

    vertex_format BlockData[] = {
        { F32V3(-0.5, -0.5, +0.5), XS, F32V2(1.0, 1.0), XF0 },
        { F32V3(-0.5, +0.5, +0.5), XS, F32V2(1.0, 0.0), XF0 },
        { F32V3(-0.5, +0.5, -0.5), XS, F32V2(0.0, 0.0), XF0 },

        { F32V3(-0.5, -0.5, +0.5), XS, F32V2(1.0, 1.0), XF0 },
        { F32V3(-0.5, -0.5, -0.5), XS, F32V2(0.0, 1.0), XF0 },
        { F32V3(-0.5, +0.5, -0.5), XS, F32V2(0.0, 0.0), XF0 },

        { F32V3(+0.5, -0.5, +0.5), XS, F32V2(0.0, 1.0), XF1 },
        { F32V3(+0.5, +0.5, +0.5), XS, F32V2(0.0, 0.0), XF1 },
        { F32V3(+0.5, +0.5, -0.5), XS, F32V2(1.0, 0.0), XF1 },

        { F32V3(+0.5, -0.5, +0.5), XS, F32V2(0.0, 1.0), XF1 },
        { F32V3(+0.5, -0.5, -0.5), XS, F32V2(1.0, 1.0), XF1 },
        { F32V3(+0.5, +0.5, -0.5), XS, F32V2(1.0, 0.0), XF1 },

        { F32V3(-0.5, -0.5, -0.5), YS, F32V2(0.0, 1.0), YF0 },
        { F32V3(-0.5, -0.5, +0.5), YS, F32V2(0.0, 0.0), YF0 },
        { F32V3(+0.5, -0.5, +0.5), YS, F32V2(1.0, 0.0), YF0 },

        { F32V3(-0.5, -0.5, -0.5), YS, F32V2(0.0, 1.0), YF0 },
        { F32V3(+0.5, -0.5, -0.5), YS, F32V2(1.0, 1.0), YF0 },
        { F32V3(+0.5, -0.5, +0.5), YS, F32V2(1.0, 0.0), YF0 },

        { F32V3(-0.5, +0.5, -0.5), YS, F32V2(0.0, 0.0), YF1 },
        { F32V3(-0.5, +0.5, +0.5), YS, F32V2(0.0, 1.0), YF1 },
        { F32V3(+0.5, +0.5, +0.5), YS, F32V2(1.0, 1.0), YF1 },

        { F32V3(-0.5, +0.5, -0.5), YS, F32V2(0.0, 0.0), YF1 },
        { F32V3(+0.5, +0.5, -0.5), YS, F32V2(1.0, 0.0), YF1 },
        { F32V3(+0.5, +0.5, +0.5), YS, F32V2(1.0, 1.0), YF1 },

        { F32V3(-0.5, -0.5, -0.5), ZS, F32V2(1.0, 1.0), ZF0 },
        { F32V3(-0.5, +0.5, -0.5), ZS, F32V2(1.0, 0.0), ZF0 },
        { F32V3(+0.5, +0.5, -0.5), ZS, F32V2(0.0, 0.0), ZF0 },

        { F32V3(-0.5, -0.5, -0.5), ZS, F32V2(1.0, 1.0), ZF0 },
        { F32V3(+0.5, -0.5, -0.5), ZS, F32V2(0.0, 1.0), ZF0 },
        { F32V3(+0.5, +0.5, -0.5), ZS, F32V2(0.0, 0.0), ZF0 },

        { F32V3(-0.5, -0.5, +0.5), ZS, F32V2(0.0, 1.0), ZF1 },
        { F32V3(-0.5, +0.5, +0.5), ZS, F32V2(0.0, 0.0), ZF1 },
        { F32V3(+0.5, +0.5, +0.5), ZS, F32V2(1.0, 0.0), ZF1 },

        { F32V3(-0.5, -0.5, +0.5), ZS, F32V2(0.0, 1.0), ZF1 },
        { F32V3(+0.5, -0.5, +0.5), ZS, F32V2(1.0, 1.0), ZF1 },
        { F32V3(+0.5, +0.5, +0.5), ZS, F32V2(1.0, 0.0), ZF1 }
    };

    i32 VertexCount = ArrayCount(BlockData);
    i32 BufferSize = sizeof(BlockData);
    i32 Stride = sizeof(BlockData[0]);

    MatrixPosition = GL->GetUniformLocation(Program, "matrix");

    u32 TerrainTexturePosition = GL->GetUniformLocation(Program, "terrainTexture");
    GL->Uniform1i(TerrainTexturePosition, 0);

    u32 Buffer;
    GL->GenBuffers(1, &Buffer);
    GL->BindBuffer(GL_ARRAY_BUFFER, Buffer);
    GL->BufferData(GL_ARRAY_BUFFER, BufferSize, &BlockData, GL_STATIC_DRAW);

    u32 VAO;
    GL->GenVertexArrays(1, &VAO);
    GL->BindVertexArray(VAO);

    GL->EnableVertexAttribArray(0);
    GL->VertexAttribPointer(0, 3, GL_FLOAT, false, Stride, (void *) 0);

    GL->EnableVertexAttribArray(1);
    GL->VertexAttribPointer(1, 1, GL_FLOAT, false, Stride, (void *) (3 * 4));

    GL->EnableVertexAttribArray(2);
    GL->VertexAttribPointer(2, 2, GL_FLOAT, false, Stride, (void *) (4 * 4));

    GL->EnableVertexAttribArray(3);
    GL->VertexAttribIPointer(3, 1, GL_INT, Stride, (void *) (6 * 4));

    GL->Enable(GL_DEPTH_TEST);
    GL->DepthFunc(GL_LEQUAL);

    OpenGLWindowStartLoop();
}

f32 Theta = 0.0f;

void OpenGLWindowUpdateHook(opengl_window * Window)
{
    Theta += 0.0004f;

    f32_m4x4 TranslateMatrix = F32M4x4ProjTranslate(0, 0, -2.0);
    f32_m4x4 ProjectionMatrix = F32M4x4ProjPerspective(PiConst / 2.0f, 1.0f, 0.1f, 5.0f);
    f32_q Quaternion1 = F32QuaternionRotation(Theta, 1.0f, 0, 0);
    f32_q Quaternion2 = F32QuaternionRotation(Theta, 0, 1.0f, 0);
    f32_m4x4 RotationMatrix = F32M4x4ProjRotation(F32QuaternionMultiply(Quaternion1, Quaternion2));
    f32_m4x4 FinalMatrix = F32M4x4Multiply(ProjectionMatrix, F32M4x4Multiply(TranslateMatrix, RotationMatrix));

    GL->UniformMatrix4fv(MatrixPosition, 1, true, &FinalMatrix);

    GL->ClearColor(0.1F, 0.1F, 0.3F, 1.0F);
    GL->ClearDepth(1.0f);
    GL->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GL->DrawArrays(GL_TRIANGLES, 0, 6 * 6);
}