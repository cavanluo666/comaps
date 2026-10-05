#pragma once

#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace storage
{
class Storage;

/// A single file that has to be copied to make a downloaded map usable elsewhere.
struct MapExportFile
{
  /// Absolute path of the source file inside the app's writable directory.
  std::string m_path;
  /// Destination file name (without any directory component).
  std::string m_name;
  uint64_t m_size = 0;
};

/// Result of an export operation.
enum class MapExportResult
{
  /// All requested maps were copied successfully.
  Ok,
  /// Nothing was exported: none of the requested countries is downloaded.
  NoMaps,
  /// The destination directory is missing / not writable, or there is not enough space.
  DestinationError,
  /// At least one file could not be copied.
  CopyError
};

/// Collects every file needed to transfer |countryId| to another device/installation.
///
/// Besides the map itself (<name>.mwm) this includes the routing data
/// (<name>.mwm.routing) when it exists: without it an exported map can be rendered
/// and searched, but routes cannot be built over it.
///
/// \param countryId A leaf country id as used in countries.txt, e.g. "Abkhazia".
/// \returns Empty vector when the country is not downloaded to disk.
std::vector<MapExportFile> GetFilesForExport(Storage const & storage, CountryId const & countryId);

/// Total size in bytes of all files that ExportMaps() would copy for |countryIds|.
uint64_t GetExportSize(Storage const & storage, std::vector<CountryId> const & countryIds);

/// Copies files of the given already downloaded maps into |destDir|.
///
/// Files keep their original names so the destination directory can later be imported
/// simply by selecting it as the maps storage directory.
///
/// \param storage    Storage holding the local map registry.
/// \param countryIds Countries to export. Unknown or not downloaded ones are skipped.
/// \param destDir    An existing writable directory to copy into.
/// \param onProgress Optional callback receiving bytes copied so far and the total
///                   amount to copy. Called with (0, 0) for an empty request.
MapExportResult ExportMaps(Storage const & storage, std::vector<CountryId> const & countryIds,
                           std::string const & destDir,
                           std::function<void(uint64_t, uint64_t)> const & onProgress = {});

/// Human readable reason of the last failure. Empty when there was no failure yet.
std::string const & GetLastExportError();

/// Result of the most recent ExportMaps() call. Useful for the UI thread that
/// runs the copy on a worker thread and inspects the outcome afterwards.
MapExportResult GetLastExportResult();

/// Number of maps actually copied by the most recent ExportMaps() call.
uint64_t GetLastExportCount();
}  // namespace storage
