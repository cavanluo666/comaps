#pragma once

#include "storage/storage_defines.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace storage
{
class Storage;

/// Snapshot of what a directory contains, before anything is imported.
struct ImportableMap
{
  /// Country (region) id deducted from the file name, e.g. "Japan_Tokyo".
  CountryId m_countryId;
  /// Full path of the map file (<countryId>.mwm) inside |directory|.
  std::string m_mapPath;
  /// Full path of the routing file (<countryId>.mwm.routing), empty when absent.
  std::string m_routingPath;
  uint64_t m_size = 0;
};

/// Result of an import operation.
enum class MapImportResult
{
  /// Every discovered file was imported.
  Ok,
  /// Nothing to import: no supported map files were found in the directory.
  NoMaps,
  /// Source directory does not exist / cannot be enumerated.
  SourceError,
  /// Destination has not enough free space to hold the imported maps.
  DestinationError,
  /// At least one file could not be copied.
  CopyError
};

/// Scans |directory| (non recursively) for map files that can be imported.
///
/// A "<name>.mwm" file is recognized as an importable map; its optional
/// "<name>.mwm.routing" companion is picked up automatically when present.
/// Files belonging to maps already downloaded to the app storage are still
/// reported, the caller decides whether to skip them.
std::vector<ImportableMap> GetMapsForImport(std::string const & directory);

/// Copies every map found in |srcDir| into the current maps storage directory and
/// makes them visible to the app.
///
/// Maps are always copied, never moved, so a failed or cancelled import can't
/// damage the source location. After copying, the storage registry is refreshed
/// so the imported maps become immediately usable.
///
/// \param srcDir Directory to import from, e.g. the one previously produced by ExportMaps().
/// \param onProgress Optional callback receiving bytes copied so far and the total to copy.
MapImportResult ImportMaps(Storage & storage, std::string const & srcDir,
                           std::function<void(uint64_t, uint64_t)> const & onProgress = {});

/// Human readable reason of the last import failure. Empty when the last result was Ok.
std::string const & GetLastImportError();

/// Result of the most recent ImportMaps() call. Useful for the UI thread that
/// runs the copy on a worker thread and inspects the outcome afterwards.
MapImportResult GetLastImportResult();

/// Number of maps actually copied by the most recent ImportMaps() call.
uint64_t GetLastImportCount();
}  // namespace storage
