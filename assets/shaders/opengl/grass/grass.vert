#version 450
#extension GL_ARB_separate_shader_objects : enable

// Note: Make planet normal a chunk thing to save on vertex memory

layout(binding = 0) uniform GrassUniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

layout(location = 0) in vec4 in_position_color;

layout(location = 0) out float frag_color;

void main() {
  gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position_color.xyz, 1.0);
  frag_color = in_position_color.w;
}
