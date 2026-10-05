#pragma once

#include "map/framework.hpp"

#include "platform/downloader_defines.hpp"

#include "base/thread_checker.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "3party/ankerl/unordered_dense.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QPushButton;
class QProgressDialog;

class Framework;

namespace qt
{
class UpdateDialog : public QDialog
{
  Q_OBJECT

public:
  explicit UpdateDialog(QWidget * parent, Framework & framework);
  virtual ~UpdateDialog();

  /// @name Called from downloader to notify GUI
  //@{
  void OnCountryChanged(storage::CountryId const & countryId);
  void OnCountryDownloadProgress(storage::CountryId const & countryId, downloader::Progress const & progress);
  //@}

  void ShowModal();

protected:
  void done(int r) override;

private slots:
  void OnItemClick(QTreeWidgetItem * item, int column);
  void OnCheckUpdatesClick();
  void OnCloseClick();
  void OnLocaleTextChanged(QString const & text);
  void OnQueryTextChanged(QString const & text);
  void OnExportAllClick();
  void OnImportClick();
  void OnContextMenuRequest(QPoint const & pos);

private:
  // CountryId to its ranking position and matched string (assuming no duplicates).
  using Filter = ankerl::unordered_dense::map<storage::CountryId, std::pair<size_t, std::string>>;

  void RefillTree();
  void StartSearchInDownloader();

  // Adds only those countries present in |filter|.
  // Calls whose timestamp is not the latest are discarded.
  void FillTree(std::optional<Filter> const & filter, uint64_t timestamp);
  void FillTreeImpl(QTreeWidgetItem * parent, storage::CountryId const & countryId,
                    std::optional<Filter> const & filter);

  void UpdateRowWithCountryInfo(storage::CountryId const & countryId);
  void UpdateRowWithCountryInfo(QTreeWidgetItem * item, storage::CountryId const & countryId);
  QString GetNodeName(storage::CountryId const & countryId);

  QTreeWidgetItem * CreateTreeItem(storage::CountryId const & countryId, size_t posInRanking, std::string matchedBy,
                                   QTreeWidgetItem * parent);
  std::vector<QTreeWidgetItem *> GetTreeItemsByCountryId(storage::CountryId const & countryId);
  storage::CountryId GetCountryIdByTreeItem(QTreeWidgetItem *);

  /// Collects ids of all locally downloaded leaf maps (OnDisk / OnDiskOutOfDate).
  std::vector<storage::CountryId> CollectDownloadedLeaves() const;

  /// Runs |ExportMaps| for |countryIds| on a worker thread and shows a progress dialog.
  void RunExport(std::vector<storage::CountryId> const & countryIds);

  /// Runs |ImportMaps| from |srcDir| on a worker thread and shows a progress dialog.
  void RunImport(std::string const & srcDir);

  inline storage::Storage & GetStorage() const { return m_framework.GetStorage(); }

  QTreeWidget * m_tree;
  QLabel * m_pCheckUpdatesLabel;
  QPushButton * m_pCheckUpdatesButton;
  QPushButton * m_pExportAllButton;
  QPushButton * m_pImportButton;
  QProgressDialog * m_pProgress;
  Framework & m_framework;
  int m_observerSlotId;

  // Params of the queries to the search engine.
  std::string m_query;
  std::string m_locale = "en";
  uint64_t m_fillTreeTimestamp = 0;

  std::unordered_multimap<storage::CountryId, QTreeWidgetItem *> m_treeItemByCountryId;

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
}  // namespace qt
