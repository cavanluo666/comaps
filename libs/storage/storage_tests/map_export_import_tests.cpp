#include "testing/testing.hpp"

#include "storage/map_exporter.hpp"
#include "storage/map_importer.hpp"
#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include <map>
#include <vector>

using namespace storage;
using namespace platform;
using namespace platform::tests_support;

namespace
{
}  // namespace

UNIT_TEST(MapExporterImporter_RoutingPairedWithMap)
{
  ScopedDir srcDir("export_src");
  CountryFile cf("TestCountry");
  ScopedFile mapFile(srcDir, cf, MapFileType::Map);
  ScopedFile routingFile(srcDir, cf, MapFileType::Route);
  TEST(mapFile.Exists() && routingFile.Exists(), ());

  // GetMapsForImport must pair the routing file with its map by country id.
  auto const maps = GetMapsForImport(srcDir.GetFullPath());
  TEST_EQUAL(maps.size(), 1, ());
  TEST_EQUAL(maps.front().m_countryId, "TestCountry", ());
  TEST(!maps.front().m_mapPath.empty(), ());
  TEST(!maps.front().m_routingPath.empty(), ("routing should be detected"));
}

UNIT_TEST(MapImporter_OrphanRoutingSkipped)
{
  ScopedDir srcDir("orphan_src");
  // A lone routing file with no matching .mwm must not produce an importable map.
  CountryFile cf("Orphan");
  ScopedFile routingOnly(srcDir, cf, MapFileType::Route);
  TEST(routingOnly.Exists(), ());

  auto const maps = GetMapsForImport(srcDir.GetFullPath());
  TEST(maps.empty(), ("orphan routing must be skipped"));
}

UNIT_TEST(MapImporter_WithoutRouting)
{
  ScopedDir srcDir("no_routing_src");
  CountryFile cf("NoRoute");
  ScopedFile mapOnly(srcDir, cf, MapFileType::Map);
  TEST(mapOnly.Exists(), ());

  auto const maps = GetMapsForImport(srcDir.GetFullPath());
  TEST_EQUAL(maps.size(), 1, ());
  TEST_EQUAL(maps.front().m_countryId, "NoRoute", ());
  TEST(maps.front().m_routingPath.empty(), ("no routing expected"));
  TEST(!maps.front().m_mapPath.empty(), ());
}
