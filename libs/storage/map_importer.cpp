#include "storage/map_importer.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <map>

namespace storage
{
namespace
{
std::string g_lastImportError;
MapImportResult g_lastImportResult = MapImportResult::Ok;
uint64_t g_lastImportCount = 0;

void SetImportError(std::string message)
{
  g_lastImportError = std::move(message);
  LOG(LWARNING, ("Map import:", g_lastImportError));
}

/// Strips a known map-related extension from |name| in place. Returns false when the
/// name does not carry any of the recognized extensions.
bool StripKnownExtension(std::string & name)
{
  if (name.ends_with(DATA_FILE_EXTENSION))
  {
    base::GetNameWithoutExt(name);
    return true;
  }
  return false;
}
}  // namespace

std::vector<ImportableMap> GetMapsForImport(std::string const & directory)
{
  std::vector<ImportableMap> result;

  Platform::TFilesWithType files;
  Platform::GetFilesByType(directory, Platform::EFileType::Regular, files);

  // Keyed by country id so that a routing file is attached to its map regardless of
  // the order in which the directory enumerates them.
  std::map<CountryId, ImportableMap> byId;

  for (auto const & [name, type] : files)
  {
    if (type != Platform::EFileType::Regular)
      continue;

    auto const fullPath = base::JoinPath(directory, name);

    if (name.ends_with(ROUTING_FILE_EXTENSION))
    {
      // "<countryId>.mwm.routing" -> strip ".routing" then ".mwm".
      std::string baseName = name.substr(0, name.size() - std::string(ROUTING_FILE_EXTENSION).size());
      auto & entry = byId[std::move(baseName)];
      entry.m_routingPath = fullPath;
      uint64_t size = 0;
      if (Platform::GetFileSizeByFullPath(fullPath, size))
        entry.m_size += size;
      continue;
    }

    std::string countryId = name;
    if (!StripKnownExtension(countryId))
      continue;

    auto & entry = byId[countryId];
    entry.m_countryId = std::move(countryId);
    entry.m_mapPath = fullPath;
    uint64_t size = 0;
    if (Platform::GetFileSizeByFullPath(fullPath, size))
      entry.m_size += size;
  }

  for (auto & [id, entry] : byId)
  {
    // The map file itself is mandatory: a lone routing file carries no map data.
    if (entry.m_countryId.empty() || entry.m_mapPath.empty())
      continue;
    result.push_back(std::move(entry));
  }

  return result;
}

MapImportResult ImportMaps(Storage & storage, std::string const & srcDir,
                           std::function<void(uint64_t, uint64_t)> const & onProgress)
{
  g_lastImportError.clear();
  g_lastImportCount = 0;

  if (!Platform::IsFileExistsByFullPath(srcDir))
  {
    SetImportError("Source directory does not exist: " + srcDir);
    g_lastImportResult = MapImportResult::SourceError;
    return g_lastImportResult;
  }

  auto const maps = GetMapsForImport(srcDir);
  if (maps.empty())
  {
    g_lastImportResult = MapImportResult::NoMaps;
    if (onProgress)
      onProgress(0, 0);
    return g_lastImportResult;
  }

  uint64_t const totalSize = [&maps] {
    uint64_t sum = 0;
    for (auto const & m : maps)
      sum += m.m_size;
    return sum;
  }();

  Platform & platform = GetPlatform();
  uint64_t constexpr kExtraSizeBytes = 10 * 1024 * 1024;
  if (platform.GetWritableStorageStatus(totalSize + kExtraSizeBytes) != Platform::TStorageStatus::STORAGE_OK)
  {
    SetImportError("Not enough free space to import, need " + strings::to_string(totalSize) + " bytes");
    g_lastImportResult = MapImportResult::DestinationError;
    return g_lastImportResult;
  }

  std::string const destDir = platform.WritableDir();
  uint64_t copied = 0;
  uint64_t importedMaps = 0;

  for (auto const & map : maps)
  {
    if (!map.m_mapPath.empty())
    {
      std::string const destPath = base::JoinPath(destDir, map.m_countryId + DATA_FILE_EXTENSION);
      if (!base::CopyFileX(map.m_mapPath, destPath))
      {
        SetImportError("Failed to import map file to " + destPath);
        return MapImportResult::CopyError;
      }
      copied += map.m_size;
      ++importedMaps;
    }

    if (!map.m_routingPath.empty())
    {
      std::string const destPath = base::JoinPath(destDir, map.m_countryId + ROUTING_FILE_EXTENSION);
      if (!base::CopyFileX(map.m_routingPath, destPath))
      {
        // A missing/broken routing file only degrades routing, the map itself stays usable,
        // so keep going instead of failing the whole import.
        LOG(LWARNING, ("Map import: failed to copy routing file to", destPath));
      }
    }

    if (onProgress)
      onProgress(copied, totalSize);
  }

  // Let the storage rescan the maps directory so the freshly copied files become usable
  // without restarting the app.
  // Diffs are disabled on re-registration to match what Storage itself does on init
  // (see Storage::Init), applying diffs is a separate explicit step.
  storage.RegisterAllLocalMaps(false /* enableDiffs */);

  g_lastImportCount = importedMaps;
  g_lastImportResult = MapImportResult::Ok;
  return g_lastImportResult;
}

std::string const & GetLastImportError()
{
  return g_lastImportError;
}

MapImportResult GetLastImportResult()
{
  return g_lastImportResult;
}

uint64_t GetLastImportCount()
{
  return g_lastImportCount;
}
}  // namespace storage
