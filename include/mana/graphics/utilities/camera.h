#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <ubermath/ubermath.h>

#include "mana//graphics/render/window.h"
#include "mana/graphics/graphicscommon.h"

#define YAW -90.0f
#define PITCH 0.0f
#define ROLL 90.0f
#define SPEED 2.5f
#define SENSITIVITY 0.1f
#define ZOOM 45.0f

enum CameraMovement {
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT,
  UP,
  DOWN
};

struct Camera {
  vec3 position;
  vec3 front;
  vec3 up;
  vec3 right;
  vec3 world_up;
  float yaw;
  float pitch;
  float roll;
  float sensitivity;
  float zoom;
  float z_near;
  float z_far;
  float speed;
  bool mouse_locked;
  quat orientation;
  mat4 view;
};

void camera_init(struct Camera* camera);
mat4 camera_get_projection_matrix(struct Camera* camera, struct Window* window);
mat4 camera_get_view_matrix(struct Camera* camera);
void camera_update_vectors(struct Camera* camera);

#endif  // CAMERA_H