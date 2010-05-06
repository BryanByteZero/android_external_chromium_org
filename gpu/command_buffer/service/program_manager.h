// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_PROGRAM_MANAGER_H_
#define GPU_COMMAND_BUFFER_SERVICE_PROGRAM_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/ref_counted.h"
#include "gpu/command_buffer/service/gl_utils.h"

namespace gpu {
namespace gles2 {

// Tracks the Programs.
//
// NOTE: To support shared resources an instance of this class will
// need to be shared by multiple GLES2Decoders.
class ProgramManager {
 public:
  // This is used to track which attributes a particular program needs
  // so we can verify at glDrawXXX time that every attribute is either disabled
  // or if enabled that it points to a valid source.
  class ProgramInfo : public base::RefCounted<ProgramInfo> {
   public:
    typedef scoped_refptr<ProgramInfo> Ref;

    struct UniformInfo {
      UniformInfo(GLsizei _size, GLenum _type, const std::string& _name)
          : size(_size),
            type(_type),
            name(_name) {
      }
      bool IsSampler() const {
        return type == GL_SAMPLER_2D || type == GL_SAMPLER_CUBE;
      }

      GLsizei size;
      GLenum type;
      bool is_array;
      std::string name;
      std::vector<GLint> element_locations;
      std::vector<GLuint> texture_units;
    };
    struct VertexAttribInfo {
      VertexAttribInfo(GLsizei _size, GLenum _type, const std::string& _name,
                       GLint _location)
          : size(_size),
            type(_type),
            location(_location),
            name(_name) {
      }
      GLsizei size;
      GLenum type;
      GLint location;
      std::string name;
    };

    typedef std::vector<UniformInfo> UniformInfoVector;
    typedef std::vector<VertexAttribInfo> AttribInfoVector;
    typedef std::vector<int> SamplerIndices;

    explicit ProgramInfo(GLuint service_id)
        : max_attrib_name_length_(0),
          max_uniform_name_length_(0),
          service_id_(service_id),
          valid_(false) {
    }

    GLuint service_id() const {
      return service_id_;
    }

    const SamplerIndices& sampler_indices() {
      return sampler_indices_;
    }

    // Resets the program after an unsuccessful link.
    void Reset();

    // Updates the program info after a successful link.
    void Update();

    const AttribInfoVector& GetAttribInfos() const {
      return attrib_infos_;
    }

    const VertexAttribInfo* GetAttribInfo(GLint index) const {
      return (static_cast<size_t>(index) < attrib_infos_.size()) ?
         &attrib_infos_[index] : NULL;
    }

    GLint GetAttribLocation(const std::string& name) const;

    const UniformInfo* GetUniformInfo(GLint index) const {
      return (static_cast<size_t>(index) < uniform_infos_.size()) ?
         &uniform_infos_[index] : NULL;
    }

    // Gets the location of a uniform by name.
    GLint GetUniformLocation(const std::string& name) const;

    // Gets the type of a uniform by location.
    bool GetUniformTypeByLocation(GLint location, GLenum* type) const;

    // Sets the sampler values for a uniform.
    // This is safe to call for any location. If the location is not
    // a sampler uniform nothing will happen.
    bool SetSamplers(GLint location, GLsizei count, const GLint* value);

    bool IsDeleted() const {
      return service_id_ == 0;
    }

    void GetProgramiv(GLenum pname, GLint* params);

    bool IsValid() const {
      return valid_;
    }

   private:
    friend class base::RefCounted<ProgramInfo>;
    friend class ProgramManager;

    ~ProgramInfo() { }

    void MarkAsDeleted() {
      service_id_ = 0;
    }

    const UniformInfo* AddUniformInfo(
        GLsizei size, GLenum type, GLint location, const std::string& name);

    GLsizei max_attrib_name_length_;

    AttribInfoVector attrib_infos_;

    GLsizei max_uniform_name_length_;

    // Uniform info by index.
    UniformInfoVector uniform_infos_;

    // Uniform location to index.
    std::vector<GLint> location_to_index_map_;

    // The indices of the uniforms that are samplers.
    SamplerIndices sampler_indices_;

    // The program this ProgramInfo is tracking.
    GLuint service_id_;

    // This is true if glLinkProgram was successful.
    bool valid_;
  };

  ProgramManager() { }

  // Creates a new program info.
  void CreateProgramInfo(GLuint client_id, GLuint service_id);

  // Gets a program info
  ProgramInfo* GetProgramInfo(GLuint client_id);

  // Deletes the program info for the given program.
  void RemoveProgramInfo(GLuint client_id);

  // Gets a client id for a given service id.
  bool GetClientId(GLuint service_id, GLuint* client_id) const;

  // Returns true if prefix is invalid for gl.
  static bool IsInvalidPrefix(const char* name, size_t length);

 private:
  // Info for each "successfully linked" program by service side program Id.
  // TODO(gman): Choose a faster container.
  typedef std::map<GLuint, ProgramInfo::Ref> ProgramInfoMap;
  ProgramInfoMap program_infos_;

  DISALLOW_COPY_AND_ASSIGN(ProgramManager);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_PROGRAM_MANAGER_H_

