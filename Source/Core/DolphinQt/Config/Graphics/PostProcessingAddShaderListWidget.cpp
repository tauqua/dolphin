// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DolphinQt/Config/Graphics/PostProcessingAddShaderListWidget.h"

#include <string>

#include <QComboBox>
#include <QDir>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QVBoxLayout>

#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/FileSearch.h"
#include "Common/FileUtil.h"

PostProcessingAddShaderListWidget::PostProcessingAddShaderListWidget(QWidget* parent,
                                                                     const QDir& root_path,
                                                                     bool allows_drag_drop)
    : QWidget(parent), m_root_path(root_path), m_allows_drag_drop(allows_drag_drop)
{
  CreateWidgets();
  ConnectWidgets();
  setLayout(m_main_layout);
}

void PostProcessingAddShaderListWidget::CreateWidgets()
{
  m_main_layout = new QVBoxLayout;

  m_shader_type = new QComboBox();

  for (const QFileInfo& info : m_root_path.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot))
  {
    m_shader_type->addItem(info.fileName(), info.absoluteFilePath());
  }
  m_main_layout->addWidget(m_shader_type);

  m_search_text = new QLineEdit();
  m_search_text->setPlaceholderText(tr("Filter shaders..."));
  m_main_layout->addWidget(m_search_text);

  m_shader_list = new QListWidget();
  m_shader_list->setSortingEnabled(false);
  m_shader_list->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  m_shader_list->setSelectionMode(QAbstractItemView::SelectionMode::MultiSelection);
  m_shader_list->setSelectionRectVisible(true);
  m_main_layout->addWidget(m_shader_list);
}

void PostProcessingAddShaderListWidget::ConnectWidgets()
{
  connect(m_shader_list, &QListWidget::itemSelectionChanged, this, [this] { OnShadersSelected(); });
  connect(m_shader_type, &QComboBox::currentTextChanged, this, [this] { OnShaderTypeChanged(); });
  connect(m_search_text, &QLineEdit::textEdited, this,
          [this](QString text) { OnSearchTextChanged(text); });
}

void PostProcessingAddShaderListWidget::OnShadersSelected()
{
  m_chosen_shader_pathes.clear();
  if (m_shader_list->selectedItems().empty())
    return;
  for (auto item : m_shader_list->selectedItems())
  {
    m_chosen_shader_pathes.push_back(item->data(Qt::UserRole).toString());
  }
}

void PostProcessingAddShaderListWidget::OnShaderTypeChanged()
{
  const int current_index = m_shader_type->currentIndex();
  if (current_index == -1)
    return;
  const auto available_shaders =
      GetAvailableShaders(m_shader_type->itemData(current_index).toString().toStdString());
  m_shader_list->clear();
  for (const std::string& shader : available_shaders)
  {
    std::string name;
    SplitPath(shader, nullptr, &name, nullptr);
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(name));
    item->setData(Qt::UserRole, QString::fromStdString(shader));
    m_shader_list->addItem(item);
  }
}

void PostProcessingAddShaderListWidget::OnSearchTextChanged(QString search_text)
{
  for (int i = 0; i < m_shader_list->count(); i++)
    m_shader_list->item(i)->setHidden(true);

  const QList<QListWidgetItem*> matched_items =
      m_shader_list->findItems(search_text, Qt::MatchFlag::MatchContains);
  for (auto* item : matched_items)
    item->setHidden(false);
}

std::vector<std::string>
PostProcessingAddShaderListWidget::GetAvailableShaders(const std::string& directory_path)
{
  const std::vector<std::string> search_dirs = {directory_path};
  const std::vector<std::string> search_extensions = {".glsl"};
  std::vector<std::string> result;
  std::vector<std::string> paths;

  // main folder
  paths = Common::DoFileSearch(search_dirs, search_extensions, false);
  for (const std::string& path : paths)
  {
    if (std::find(result.begin(), result.end(), path) == result.end())
      result.push_back(path);
  }

  // folders/sub-shaders
  // paths = Common::DoFileSearch(search_dirs, false);
  for (const std::string& dirname : paths)
  {
    // find sub-shaders in this folder
    size_t pos = dirname.find_last_of(DIR_SEP_CHR);
    if (pos != std::string::npos && (pos != dirname.length() - 1))
    {
      std::string shader_dirname = dirname.substr(pos + 1);
      std::vector<std::string> sub_paths =
          Common::DoFileSearch(search_extensions, {dirname}, false);
      for (const std::string& sub_path : sub_paths)
      {
        if (std::find(result.begin(), result.end(), sub_path) == result.end())
          result.push_back(sub_path);
      }
    }
  }

  // sort by path
  std::sort(result.begin(), result.end());
  return result;
}
