#pragma once
#ifndef MANIFOLD_PLANET_H
#define MANIFOLD_PLANET_H

#include "mana/core/memoryallocator.h"
//
#include <cnoise/cnoise.h>
#include <mana/core/gpuapi.h>

#include "mana/graphics/dualcontouring/manifold/manifolddualcontouring.h"
#include "mana/graphics/utilities/camera.h"

// TODO: Should take care of LOD

enum ManifoldPlanetType {
  MANIFOLD_ROUND_PLANET
};

struct ManifoldPlanet {
  enum ManifoldPlanetType planet_type;
  struct ManifoldDualContouring manifold_dual_contouring;
  struct Shader* terrain_shader;
  struct NoiseModule* planet_shape;
  vec3 position;
};

void manifold_planet_init(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api, float planet_size, struct Shader* shader, struct NoiseModule* planet_shape, vec3 position);
void manifold_planet_delete(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api);
void manifold_planet_render(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api);
void manifold_planet_update_uniforms(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api, struct Camera* camera, vec3 light_pos);

#endif  // MANIFOLD_PLANET_H
