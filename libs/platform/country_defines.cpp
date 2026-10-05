#include "platform/country_defines.hpp"

#include "base/assert.hpp"

std::string DebugPrint(MapFileType type)
{
  switch (type)
  {
  case MapFileType::Map: return "Map";
  case MapFileType::Diff: return "Diff";
  case MapFileType::Route: return "Route";
  case MapFileType::Count: return "Count";
  }
  UNREACHABLE();
}
