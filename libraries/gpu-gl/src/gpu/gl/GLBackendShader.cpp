//
//  GLBackendShader.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/28/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLBackendShared.h"

using namespace gpu;
using namespace gpu::gl;

GLBackend::GLShader::GLShader()
{
}

GLBackend::GLShader::~GLShader() {
    for (auto& so : _shaderObjects) {
        if (so.glshader != 0) {
            glDeleteShader(so.glshader);
        }
        if (so.glprogram != 0) {
            glDeleteProgram(so.glprogram);
        }
    }
}

bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject, GLuint &programObject) {
    if (shaderSource.empty()) {
        qCDebug(gpugllogging) << "GLShader::compileShader - no GLSL shader source code ? so failed to create";
        return false;
    }

    // Create the shader object
    GLuint glshader = glCreateShader(shaderDomain);
    if (!glshader) {
        qCDebug(gpugllogging) << "GLShader::compileShader - failed to create the gl shader object";
        return false;
    }

    // Assign the source
    const int NUM_SOURCE_STRINGS = 2;
    const GLchar* srcstr[] = { defines.c_str(), shaderSource.c_str() };
    glShaderSource(glshader, NUM_SOURCE_STRINGS, srcstr, NULL);

    // Compile !
    glCompileShader(glshader);

    // check if shader compiled
    GLint compiled = 0;
    glGetShaderiv(glshader, GL_COMPILE_STATUS, &compiled);

    // if compilation fails
    if (!compiled) {
        // save the source code to a temp file so we can debug easily
        /* std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetShaderiv(glshader, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetShaderInfoLog(glshader, infoLength, NULL, temp);


        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << std::string(temp);
        filestream.close();
        }
        */

        qCWarning(gpugllogging) << "GLShader::compileShader - failed to compile the gl shader object:";
        for (auto s : srcstr) {
            qCWarning(gpugllogging) << s;
        }
        qCWarning(gpugllogging) << "GLShader::compileShader - errors:";
        qCWarning(gpugllogging) << temp;
        delete[] temp;

        glDeleteShader(glshader);
        return false;
    }

    GLuint glprogram = 0;
#ifdef SEPARATE_PROGRAM
    // so far so good, program is almost done, need to link:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(gpugllogging) << "GLShader::compileShader - failed to create the gl shader & gl program object";
        return false;
    }

    glProgramParameteri(glprogram, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(glprogram, glshader);
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(gpugllogging) << "GLShader::compileShader -  failed to LINK the gl program object :";
        qCDebug(gpugllogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << String(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteShader(glshader);
        glDeleteProgram(glprogram);
        return false;
    }
#endif

    shaderObject = glshader;
    programObject = glprogram;

    return true;
}

GLuint compileProgram(const std::vector<GLuint>& glshaders) {
    // A brand new program:
    GLuint glprogram = glCreateProgram();
    if (!glprogram) {
        qCDebug(gpugllogging) << "GLShader::compileProgram - failed to create the gl program object";
        return 0;
    }

    // glProgramParameteri(glprogram, GL_PROGRAM_, GL_TRUE);
    // Create the program from the sub shaders
    for (auto so : glshaders) {
        glAttachShader(glprogram, so);
    }

    // Link!
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);

    if (!linked) {
        /*
        // save the source code to a temp file so we can debug easily
        std::ofstream filestream;
        filestream.open("debugshader.glsl");
        if (filestream.is_open()) {
        filestream << shaderSource->source;
        filestream.close();
        }
        */

        GLint infoLength = 0;
        glGetProgramiv(glprogram, GL_INFO_LOG_LENGTH, &infoLength);

        char* temp = new char[infoLength];
        glGetProgramInfoLog(glprogram, infoLength, NULL, temp);

        qCDebug(gpugllogging) << "GLShader::compileProgram -  failed to LINK the gl program object :";
        qCDebug(gpugllogging) << temp;

        /*
        filestream.open("debugshader.glsl.info.txt");
        if (filestream.is_open()) {
        filestream << std::string(temp);
        filestream.close();
        }
        */
        delete[] temp;

        glDeleteProgram(glprogram);
        return 0;
    }

    return glprogram;
}


void makeProgramBindings(GLBackend::GLShader::ShaderObject& shaderObject) {
    if (!shaderObject.glprogram) {
        return;
    }
    GLuint glprogram = shaderObject.glprogram;
    GLint loc = -1;

    //Check for gpu specific attribute slotBindings
    loc = glGetAttribLocation(glprogram, "inPosition");
    if (loc >= 0 && loc != gpu::Stream::POSITION) {
        glBindAttribLocation(glprogram, gpu::Stream::POSITION, "inPosition");
    }

    loc = glGetAttribLocation(glprogram, "inNormal");
    if (loc >= 0 && loc != gpu::Stream::NORMAL) {
        glBindAttribLocation(glprogram, gpu::Stream::NORMAL, "inNormal");
    }

    loc = glGetAttribLocation(glprogram, "inColor");
    if (loc >= 0 && loc != gpu::Stream::COLOR) {
        glBindAttribLocation(glprogram, gpu::Stream::COLOR, "inColor");
    }

    loc = glGetAttribLocation(glprogram, "inTexCoord0");
    if (loc >= 0 && loc != gpu::Stream::TEXCOORD) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD, "inTexCoord0");
    }

    loc = glGetAttribLocation(glprogram, "inTangent");
    if (loc >= 0 && loc != gpu::Stream::TANGENT) {
        glBindAttribLocation(glprogram, gpu::Stream::TANGENT, "inTangent");
    }

    loc = glGetAttribLocation(glprogram, "inTexCoord1");
    if (loc >= 0 && loc != gpu::Stream::TEXCOORD1) {
        glBindAttribLocation(glprogram, gpu::Stream::TEXCOORD1, "inTexCoord1");
    }

    loc = glGetAttribLocation(glprogram, "inSkinClusterIndex");
    if (loc >= 0 && loc != gpu::Stream::SKIN_CLUSTER_INDEX) {
        glBindAttribLocation(glprogram, gpu::Stream::SKIN_CLUSTER_INDEX, "inSkinClusterIndex");
    }

    loc = glGetAttribLocation(glprogram, "inSkinClusterWeight");
    if (loc >= 0 && loc != gpu::Stream::SKIN_CLUSTER_WEIGHT) {
        glBindAttribLocation(glprogram, gpu::Stream::SKIN_CLUSTER_WEIGHT, "inSkinClusterWeight");
    }

    loc = glGetAttribLocation(glprogram, "_drawCallInfo");
    if (loc >= 0 && loc != gpu::Stream::DRAW_CALL_INFO) {
        glBindAttribLocation(glprogram, gpu::Stream::DRAW_CALL_INFO, "_drawCallInfo");
    }

    // Link again to take into account the assigned attrib location
    glLinkProgram(glprogram);

    GLint linked = 0;
    glGetProgramiv(glprogram, GL_LINK_STATUS, &linked);
    if (!linked) {
        qCDebug(gpugllogging) << "GLShader::makeBindings - failed to link after assigning slotBindings?";
    }

    // now assign the ubo binding, then DON't relink!

    //Check for gpu specific uniform slotBindings
#ifdef GPU_SSBO_DRAW_CALL_INFO
    loc = glGetProgramResourceIndex(glprogram, GL_SHADER_STORAGE_BLOCK, "transformObjectBuffer");
    if (loc >= 0) {
        glShaderStorageBlockBinding(glprogram, loc, gpu::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = gpu::TRANSFORM_OBJECT_SLOT;
    }
#else
    loc = glGetUniformLocation(glprogram, "transformObjectBuffer");
    if (loc >= 0) {
        glProgramUniform1i(glprogram, loc, gpu::TRANSFORM_OBJECT_SLOT);
        shaderObject.transformObjectSlot = gpu::TRANSFORM_OBJECT_SLOT;
    }
#endif

    loc = glGetUniformBlockIndex(glprogram, "transformCameraBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(glprogram, loc, gpu::TRANSFORM_CAMERA_SLOT);
        shaderObject.transformCameraSlot = gpu::TRANSFORM_CAMERA_SLOT;
    }

    (void)CHECK_GL_ERROR();
}

GLBackend::GLShader* compileBackendShader(const Shader& shader) {
    // Any GLSLprogram ? normally yes...
    const std::string& shaderSource = shader.getSource().getCode();

    // GLSL version
    const std::string glslVersion = {
        "#version 410 core"
    };

    // Shader domain
    const int NUM_SHADER_DOMAINS = 2;
    const GLenum SHADER_DOMAINS[NUM_SHADER_DOMAINS] = {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER
    };
    GLenum shaderDomain = SHADER_DOMAINS[shader.getType()];

    // Domain specific defines
    const std::string domainDefines[NUM_SHADER_DOMAINS] = {
        "#define GPU_VERTEX_SHADER",
        "#define GPU_PIXEL_SHADER"
    };

    // Versions specific of the shader
    const std::string versionDefines[GLBackend::GLShader::NumVersions] = {
        ""
    };

    GLBackend::GLShader::ShaderObjects shaderObjects;

    for (int version = 0; version < GLBackend::GLShader::NumVersions; version++) {
        auto& shaderObject = shaderObjects[version];

        std::string shaderDefines = glslVersion + "\n" + domainDefines[shader.getType()] + "\n" + versionDefines[version];

        bool result = compileShader(shaderDomain, shaderSource, shaderDefines, shaderObject.glshader, shaderObject.glprogram);
        if (!result) {
            return nullptr;
        }
    }

    // So far so good, the shader is created successfully
    GLBackend::GLShader* object = new GLBackend::GLShader();
    object->_shaderObjects = shaderObjects;

    return object;
}

GLBackend::GLShader* compileBackendProgram(const Shader& program) {
    if (!program.isProgram()) {
        return nullptr;
    }

    GLBackend::GLShader::ShaderObjects programObjects;

    for (int version = 0; version < GLBackend::GLShader::NumVersions; version++) {
        auto& programObject = programObjects[version];

        // Let's go through every shaders and make sure they are ready to go
        std::vector< GLuint > shaderGLObjects;
        for (auto subShader : program.getShaders()) {
            auto object = GLBackend::syncGPUObject(*subShader);
            if (object) {
                shaderGLObjects.push_back(object->_shaderObjects[version].glshader);
            } else {
                qCDebug(gpugllogging) << "GLShader::compileBackendProgram - One of the shaders of the program is not compiled?";
                return nullptr;
            }
        }

        GLuint glprogram = compileProgram(shaderGLObjects);
        if (glprogram == 0) {
            return nullptr;
        }

        programObject.glprogram = glprogram;

        makeProgramBindings(programObject);
    }

    // So far so good, the program versions have all been created successfully
    GLBackend::GLShader* object = new GLBackend::GLShader();
    object->_shaderObjects = programObjects;

    return object;
}

GLBackend::GLShader* GLBackend::syncGPUObject(const Shader& shader) {
    GLShader* object = Backend::getGPUObject<GLBackend::GLShader>(shader);

    // If GPU object already created then good
    if (object) {
        return object;
    }
    // need to have a gpu object?
    if (shader.isProgram()) {
        GLShader* tempObject = compileBackendProgram(shader);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    } else if (shader.isDomain()) {
        GLShader* tempObject = compileBackendShader(shader);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    }

    return object;
}

class ElementResource {
public:
    gpu::Element _element;
    uint16 _resource;

    ElementResource(Element&& elem, uint16 resource) : _element(elem), _resource(resource) {}
};

ElementResource getFormatFromGLUniform(GLenum gltype) {
    switch (gltype) {
    case GL_FLOAT: return ElementResource(Element(SCALAR, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_VEC2: return ElementResource(Element(VEC2, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_VEC3: return ElementResource(Element(VEC3, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_VEC4: return ElementResource(Element(VEC4, gpu::FLOAT, UNIFORM), Resource::BUFFER);
/*
    case GL_DOUBLE: return ElementResource(Element(SCALAR, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_DOUBLE_VEC2: return ElementResource(Element(VEC2, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_DOUBLE_VEC3: return ElementResource(Element(VEC3, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_DOUBLE_VEC4: return ElementResource(Element(VEC4, gpu::FLOAT, UNIFORM), Resource::BUFFER);
*/
    case GL_INT: return ElementResource(Element(SCALAR, gpu::INT32, UNIFORM), Resource::BUFFER);
    case GL_INT_VEC2: return ElementResource(Element(VEC2, gpu::INT32, UNIFORM), Resource::BUFFER);
    case GL_INT_VEC3: return ElementResource(Element(VEC3, gpu::INT32, UNIFORM), Resource::BUFFER);
    case GL_INT_VEC4: return ElementResource(Element(VEC4, gpu::INT32, UNIFORM), Resource::BUFFER);

    case GL_UNSIGNED_INT: return ElementResource(Element(SCALAR, gpu::UINT32, UNIFORM), Resource::BUFFER);
#if defined(Q_OS_WIN)
    case GL_UNSIGNED_INT_VEC2: return ElementResource(Element(VEC2, gpu::UINT32, UNIFORM), Resource::BUFFER);
    case GL_UNSIGNED_INT_VEC3: return ElementResource(Element(VEC3, gpu::UINT32, UNIFORM), Resource::BUFFER);
    case GL_UNSIGNED_INT_VEC4: return ElementResource(Element(VEC4, gpu::UINT32, UNIFORM), Resource::BUFFER);
#endif
            
    case GL_BOOL: return ElementResource(Element(SCALAR, gpu::BOOL, UNIFORM), Resource::BUFFER);
    case GL_BOOL_VEC2: return ElementResource(Element(VEC2, gpu::BOOL, UNIFORM), Resource::BUFFER);
    case GL_BOOL_VEC3: return ElementResource(Element(VEC3, gpu::BOOL, UNIFORM), Resource::BUFFER);
    case GL_BOOL_VEC4: return ElementResource(Element(VEC4, gpu::BOOL, UNIFORM), Resource::BUFFER);


    case GL_FLOAT_MAT2: return ElementResource(Element(gpu::MAT2, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_MAT3: return ElementResource(Element(MAT3, gpu::FLOAT, UNIFORM), Resource::BUFFER);
    case GL_FLOAT_MAT4: return ElementResource(Element(MAT4, gpu::FLOAT, UNIFORM), Resource::BUFFER);

/*    {GL_FLOAT_MAT2x3    mat2x3},
    {GL_FLOAT_MAT2x4    mat2x4},
    {GL_FLOAT_MAT3x2    mat3x2},
    {GL_FLOAT_MAT3x4    mat3x4},
    {GL_FLOAT_MAT4x2    mat4x2},
    {GL_FLOAT_MAT4x3    mat4x3},
    {GL_DOUBLE_MAT2    dmat2},
    {GL_DOUBLE_MAT3    dmat3},
    {GL_DOUBLE_MAT4    dmat4},
    {GL_DOUBLE_MAT2x3    dmat2x3},
    {GL_DOUBLE_MAT2x4    dmat2x4},
    {GL_DOUBLE_MAT3x2    dmat3x2},
    {GL_DOUBLE_MAT3x4    dmat3x4},
    {GL_DOUBLE_MAT4x2    dmat4x2},
    {GL_DOUBLE_MAT4x3    dmat4x3},
    */

    case GL_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_1D);
    case GL_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_2D);

    case GL_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_3D);
    case GL_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_CUBE);

#if defined(Q_OS_WIN)
    case GL_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);
#endif
            
    case GL_SAMPLER_2D_SHADOW: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_SHADOW), Resource::TEXTURE_2D);
#if defined(Q_OS_WIN)
    case GL_SAMPLER_CUBE_SHADOW: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_SHADOW), Resource::TEXTURE_CUBE);

    case GL_SAMPLER_2D_ARRAY_SHADOW: return ElementResource(Element(SCALAR, gpu::FLOAT, SAMPLER_SHADOW), Resource::TEXTURE_2D_ARRAY);
#endif
            
//    {GL_SAMPLER_1D_SHADOW    sampler1DShadow},
 //   {GL_SAMPLER_1D_ARRAY_SHADOW    sampler1DArrayShadow},

//    {GL_SAMPLER_BUFFER    samplerBuffer},
//    {GL_SAMPLER_2D_RECT    sampler2DRect},
 //   {GL_SAMPLER_2D_RECT_SHADOW    sampler2DRectShadow},

#if defined(Q_OS_WIN)
    case GL_INT_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_1D);
    case GL_INT_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_2D);
    case GL_INT_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_INT_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_3D);
    case GL_INT_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_CUBE);
          
    case GL_INT_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_INT_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::INT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);

 //   {GL_INT_SAMPLER_BUFFER    isamplerBuffer},
 //   {GL_INT_SAMPLER_2D_RECT    isampler2DRect},

    case GL_UNSIGNED_INT_SAMPLER_1D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_1D);
    case GL_UNSIGNED_INT_SAMPLER_2D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_2D);
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D);
    case GL_UNSIGNED_INT_SAMPLER_3D: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_3D);
    case GL_UNSIGNED_INT_SAMPLER_CUBE: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_CUBE);
          
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_1D_ARRAY);
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER), Resource::TEXTURE_2D_ARRAY);
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return ElementResource(Element(SCALAR, gpu::UINT32, SAMPLER_MULTISAMPLE), Resource::TEXTURE_2D_ARRAY);
#endif
//    {GL_UNSIGNED_INT_SAMPLER_BUFFER    usamplerBuffer},
//    {GL_UNSIGNED_INT_SAMPLER_2D_RECT    usampler2DRect},
/*
    {GL_IMAGE_1D    image1D},
    {GL_IMAGE_2D    image2D},
    {GL_IMAGE_3D    image3D},
    {GL_IMAGE_2D_RECT    image2DRect},
    {GL_IMAGE_CUBE    imageCube},
    {GL_IMAGE_BUFFER    imageBuffer},
    {GL_IMAGE_1D_ARRAY    image1DArray},
    {GL_IMAGE_2D_ARRAY    image2DArray},
    {GL_IMAGE_2D_MULTISAMPLE    image2DMS},
    {GL_IMAGE_2D_MULTISAMPLE_ARRAY    image2DMSArray},
    {GL_INT_IMAGE_1D    iimage1D},
    {GL_INT_IMAGE_2D    iimage2D},
    {GL_INT_IMAGE_3D    iimage3D},
    {GL_INT_IMAGE_2D_RECT    iimage2DRect},
    {GL_INT_IMAGE_CUBE    iimageCube},
    {GL_INT_IMAGE_BUFFER    iimageBuffer},
    {GL_INT_IMAGE_1D_ARRAY    iimage1DArray},
    {GL_INT_IMAGE_2D_ARRAY    iimage2DArray},
    {GL_INT_IMAGE_2D_MULTISAMPLE    iimage2DMS},
    {GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY    iimage2DMSArray},
    {GL_UNSIGNED_INT_IMAGE_1D    uimage1D},
    {GL_UNSIGNED_INT_IMAGE_2D    uimage2D},
    {GL_UNSIGNED_INT_IMAGE_3D    uimage3D},
    {GL_UNSIGNED_INT_IMAGE_2D_RECT    uimage2DRect},
    {GL_UNSIGNED_INT_IMAGE_CUBE    uimageCube},+        [0]    {_name="fInnerRadius" _location=0 _element={_semantic=15 '\xf' _dimension=0 '\0' _type=0 '\0' } }    gpu::Shader::Slot

    {GL_UNSIGNED_INT_IMAGE_BUFFER    uimageBuffer},
    {GL_UNSIGNED_INT_IMAGE_1D_ARRAY    uimage1DArray},
    {GL_UNSIGNED_INT_IMAGE_2D_ARRAY    uimage2DArray},
    {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE    uimage2DMS},
    {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY    uimage2DMSArray},
    {GL_UNSIGNED_INT_ATOMIC_COUNTER    atomic_uint}
*/
    default:
        return ElementResource(Element(), Resource::BUFFER);
    }

};


int makeUniformSlots(GLuint glprogram, const Shader::BindingSet& slotBindings,
    Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers) {
    GLint uniformsCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORMS, &uniformsCount);

    for (int i = 0; i < uniformsCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(glprogram, i, NAME_LENGTH, &length, &size, &type, name);
        GLint location = glGetUniformLocation(glprogram, name);
        const GLint INVALID_UNIFORM_LOCATION = -1;

        // Try to make sense of the gltype
        auto elementResource = getFormatFromGLUniform(type);

        // The uniform as a standard var type
        if (location != INVALID_UNIFORM_LOCATION) {
            // Let's make sure the name doesn't contains an array element
            std::string sname(name);
            auto foundBracket = sname.find_first_of('[');
            if (foundBracket != std::string::npos) {
              //  std::string arrayname = sname.substr(0, foundBracket);

                if (sname[foundBracket + 1] == '0') {
                    sname = sname.substr(0, foundBracket);
                } else {
                    // skip this uniform since it's not the first element of an array
                    continue;
                }
            }

            if (elementResource._resource == Resource::BUFFER) {
                uniforms.insert(Shader::Slot(sname, location, elementResource._element, elementResource._resource));
            } else {
                // For texture/Sampler, the location is the actual binding value
                GLint binding = -1;
                glGetUniformiv(glprogram, location, &binding);

                auto requestedBinding = slotBindings.find(std::string(sname));
                if (requestedBinding != slotBindings.end()) {
                    if (binding != (*requestedBinding)._location) {
                        binding = (*requestedBinding)._location;
                        glProgramUniform1i(glprogram, location, binding);
                    }
                }

                textures.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
                samplers.insert(Shader::Slot(name, binding, elementResource._element, elementResource._resource));
            }
        }
    }

    return uniformsCount;
}

const GLint UNUSED_SLOT = -1;
bool isUnusedSlot(GLint binding) {
    return (binding == UNUSED_SLOT);
}

int makeUniformBlockSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers) {
    GLint buffersCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_UNIFORM_BLOCKS, &buffersCount);

    // fast exit
    if (buffersCount == 0) {
        return 0;
    }

    GLint maxNumUniformBufferSlots = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxNumUniformBufferSlots);
    std::vector<GLint> uniformBufferSlotMap(maxNumUniformBufferSlots, -1);

    for (int i = 0; i < buffersCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLint binding = -1;

        glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &length);
        glGetActiveUniformBlockName(glprogram, i, NAME_LENGTH, &length, name);
        glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_BINDING, &binding);
        glGetActiveUniformBlockiv(glprogram, i, GL_UNIFORM_BLOCK_DATA_SIZE, &size);

        GLuint blockIndex = glGetUniformBlockIndex(glprogram, name);

        // CHeck if there is a requested binding for this block
        auto requestedBinding = slotBindings.find(std::string(name));
        if (requestedBinding != slotBindings.end()) {
        // If yes force it
            if (binding != (*requestedBinding)._location) {
                binding = (*requestedBinding)._location;
                glUniformBlockBinding(glprogram, blockIndex, binding);
            }
        } else if (binding == 0) {
        // If no binding was assigned then just do it finding a free slot
            auto slotIt = std::find_if(uniformBufferSlotMap.begin(), uniformBufferSlotMap.end(), isUnusedSlot); 
            if (slotIt != uniformBufferSlotMap.end()) {
                binding = slotIt - uniformBufferSlotMap.begin();
                glUniformBlockBinding(glprogram, blockIndex, binding);
            } else {
                // This should neve happen, an active ubo cannot find an available slot among the max available?!
                binding = -1;
            }
        }
        // If binding is valid record it
        if (binding >= 0) {
            uniformBufferSlotMap[binding] = blockIndex;
        }

        Element element(SCALAR, gpu::UINT32, gpu::UNIFORM_BUFFER);
        buffers.insert(Shader::Slot(name, binding, element, Resource::BUFFER));
    }
    return buffersCount;
}

int makeInputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& inputs) {
    GLint inputsCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_ATTRIBUTES, &inputsCount);

    for (int i = 0; i < inputsCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveAttrib(glprogram, i, NAME_LENGTH, &length, &size, &type, name);

        GLint binding = glGetAttribLocation(glprogram, name);

        auto elementResource = getFormatFromGLUniform(type);
        inputs.insert(Shader::Slot(name, binding, elementResource._element, -1));
    }

    return inputsCount;
}

int makeOutputSlots(GLuint glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& outputs) {
 /*   GLint outputsCount = 0;

    glGetProgramiv(glprogram, GL_ACTIVE_, &outputsCount);

    for (int i = 0; i < inputsCount; i++) {
        const GLint NAME_LENGTH = 256;
        GLchar name[NAME_LENGTH];
        GLint length = 0;
        GLint size = 0;
        GLenum type = 0;
        glGetActiveAttrib(glprogram, i, NAME_LENGTH, &length, &size, &type, name);

        auto element = getFormatFromGLUniform(type);
        outputs.insert(Shader::Slot(name, i, element));
    }
    */
    return 0; //inputsCount;
}

bool GLBackend::makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {

    // First make sure the Shader has been compiled
    GLShader* object = GLBackend::syncGPUObject(shader);
    if (!object) {
        return false;
    }

    // Apply bindings to all program versions and generate list of slots from default version
    for (int version = 0; version < GLBackend::GLShader::NumVersions; version++) {
        auto& shaderObject = object->_shaderObjects[version];
        if (shaderObject.glprogram) {
            Shader::SlotSet buffers;
            makeUniformBlockSlots(shaderObject.glprogram, slotBindings, buffers);

            Shader::SlotSet uniforms;
            Shader::SlotSet textures;
            Shader::SlotSet samplers;
            makeUniformSlots(shaderObject.glprogram, slotBindings, uniforms, textures, samplers);

            Shader::SlotSet inputs;
            makeInputSlots(shaderObject.glprogram, slotBindings, inputs);

            Shader::SlotSet outputs;
            makeOutputSlots(shaderObject.glprogram, slotBindings, outputs);

            // Define the public slots only from the default version
            if (version == 0) {
                shader.defineSlots(uniforms, buffers, textures, samplers, inputs, outputs);
            } else {
                GLShader::UniformMapping mapping;
                for (auto srcUniform : shader.getUniforms()) {
                    mapping[srcUniform._location] = uniforms.findLocation(srcUniform._name);
                }
                object->_uniformMappings.push_back(mapping);
            }
        }
    }


    return true;
}
