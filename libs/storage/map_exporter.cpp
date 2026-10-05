#include "storage/map_exporter.hpp"

#include "storage/storage.hpp"

#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

namespace storage
{
namespace
{
std::string g_lastExportError;
MapExportResult g_lastExportResult = MapExportResult::Ok;
uint64_t g_lastExportCount = 0;

void SetError(std::string message)
{
  g_lastExportError = std::move(message);
  LOG(LWARNING, ("Map export:", g_lastExportError));
}

bool CopySingleFile(std::string const & srcPath, std::string const & destPath)
{
  if (!base::CopyFileX(srcPath, destPath))
  {
    SetError("Failed to copy " + srcPath + " to " + destPath);
    return false;
  }
  return true;
}
}  // namespace

std::vector<MapExportFile> GetFilesForExport(Storage const & storage, CountryId const & countryId)
{
  std::vector<MapExportFile> result;

  LocalFilePtr const file = storage.GetLatestLocalFile(countryId);
  if (!file)
    return result;

  // World/WorldCoasts may live inside the app bundle (Android resources, iOS app bundle),
  // in which case there are no real files to copy - GetPath() returns a meaningless value.
  if (file->IsInBundle())
    return result;

  if (!file->HasFiles())
    return result;

  // The map itself. Without it there is nothing to export.
  if (file->OnDisk(MapFileType::Map))
  {
    result.push_back({file->GetPath(MapFileType::Map), file->GetFileName(MapFileType::Map),
                      file->GetSize(MapFileType::Map)});
  }

  // Routing data is optional but is required to build routes, so it always travels with the map.
  if (file->OnDisk(MapFileType::Route))
  {
    result.push_back({file->GetPath(MapFileType::Route), file->GetFileName(MapFileType::Route),
                      file->GetSize(MapFileType::Route)});
  }

  return result;
}

uint64_t GetExportSize(Storage const & storage, std::vector<CountryId> const & countryIds)
{
  uint64_t total = 0;
  for (auto const & countryId : countryIds)
  {
    for (auto const & f : GetFilesForExport(storage, countryId))
      total += f.m_size;
  }
  return total;
}

MapExportResult ExportMaps(Storage const & storage, std::vector<CountryId> const & countryIds,
                           std::string const & destDir,
                           std::function<void(uint64_t, uint64_t)> const & onProgress)
{
  g_lastExportError.clear();
  g_lastExportCount = 0;

  std::vector<MapExportFile> files;
  for (auto const & countryId : countryIds)
  {
    auto const countryFiles = GetFilesForExport(storage, countryId);
    if (countryFiles.empty())
      LOG(LDEBUG, ("Nothing to export for", countryId, "(not downloaded?)"));
    files.insert(files.end(), countryFiles.begin(), countryFiles.end());
  }

  if (files.empty())
  {
    g_lastExportResult = MapExportResult::NoMaps;
    if (onProgress)
      onProgress(0, 0);
    return g_lastExportResult;
  }

  uint64_t const totalSize = [&files] {
    uint64_t sum = 0;
    for (auto const & f : files)
      sum += f.m_size;
    return sum;
  }();

  Platform & platform = GetPlatform();
  if (!platform.IsFileExistsByFullPath(destDir))
  {
    SetError("Destination directory does not exist: " + destDir);
    g_lastExportResult = MapExportResult::DestinationError;
    return g_lastExportResult;
  }

  // Writing to a different storage on Android is a common use-case, so ask about free space
  // on the writable storage, keeping the same safety margin as the downloader does.
  uint64_t constexpr kExtraSizeBytes = 10 * 1024 * 1024;
  if (platform.GetWritableStorageStatus(totalSize + kExtraSizeBytes) != Platform::TStorageStatus::STORAGE_OK)
  {
    SetError("Not enough free space for export, need " + strings::to_string(totalSize) + " bytes");
    g_lastExportResult = MapExportResult::DestinationError;
    return g_lastExportResult;
  }

  uint64_t copied = 0;
  uint64_t exportedMaps = 0;
  for (auto const & countryId : countryIds)
  {
    if (GetFilesForExport(storage, countryId).empty())
      continue;
    ++exportedMaps;
  }
  for (auto const & f : files)
  {
    std::string const destPath = base::JoinPath(destDir, f.m_name);
    if (!CopySingleFile(f.m_path, destPath))
    {
      g_lastExportResult = MapExportResult::CopyError;
      return g_lastExportResult;
    }

    copied += f.m_size;
    if (onProgress)
      onProgress(copied, totalSize);
  }

  g_lastExportCount = exportedMaps;
  g_lastExportResult = MapExportResult::Ok;
  return g_lastExportResult;
}

std::string const & GetLastExportError()
{
  return g_lastExportError;
}

MapExportResult GetLastExportResult()
{
  return g_lastExportResult;
}

uint64_t GetLastExportCount()
{
  return g_lastExportCount;
}
}  // namespace storage
