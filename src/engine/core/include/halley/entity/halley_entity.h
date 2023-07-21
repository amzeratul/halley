#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "halley/entity/component.h"
#include "halley/entity/data_interpolator.h"
#include "halley/entity/ecs_reflection.h"
#include "halley/entity/message.h"
#include "halley/entity/prefab.h"
#include "halley/entity/prefab_scene_data.h"
#include "halley/entity/registry.h"
#include "halley/entity/service.h"
#include "halley/entity/system.h"
#include "halley/entity/system_message.h"
#include "halley/entity/world.h"
#include "halley/entity/world_scene_data.h"
#include "halley/entity/family_binding.h"
#include "halley/entity/family.h"
#include "halley/entity/entity_data.h"
#include "halley/entity/entity_data_delta.h"
#include "halley/entity/entity_id.h"
#include "halley/entity/entity_scene.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/entity_stage.h"

#include "halley/diagnostics/frame_debugger.h"
#include "halley/diagnostics/performance_stats.h"
#include "halley/diagnostics/world_stats.h"

#include "halley/scripting/script_environment.h"
#include "halley/scripting/script_graph.h"
#include "halley/scripting/script_node_type.h"
#include "halley/scripting/script_renderer.h"
#include "halley/scripting/script_state.h"