#pragma once

#include <cstdint>
#include <string>
#include <utility>

// Note: new values must be added before MapFileType::Count.
enum class MapFileType : uint8_t
{
  Map,
  Diff,
  /// Routing data file (<name>.mwm.routing) kept alongside the corresponding
  /// <name>.mwm. Required to build routes, so it must travel with the map
  /// whenever the map is copied or exported.
  Route,

  Count
};

using MwmCounter = uint32_t;
using MwmSize = uint64_t;
using LocalAndRemoteSize = std::pair<MwmSize, MwmSize>;

std::string DebugPrint(MapFileType type);
