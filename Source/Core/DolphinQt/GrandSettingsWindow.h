// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <optional>

#include <QDialog>

#include "DolphinQt/Config/Graphics/GraphicsDialog.h"

class QLineEdit;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace X11Utils
{
class XRRConfiguration;
}

class GrandSettingsWindow final : public GraphicsDialog
{
  Q_OBJECT
public:
  explicit GrandSettingsWindow(X11Utils::XRRConfiguration* xrr_config, QWidget* parent = nullptr);
  void SetControllersPane();
  void SetGraphicsPane();
  void SetControllersEnabled(bool enable);

private:
  enum class SelectedPane
  {
    Default,
    Graphics,
    Controllers
  };
  void PopulateWidgets();
  void OnSearch(const QString& text);
  bool TreeNodeContainsText(const QString& text, QTreeWidgetItem* item);
  void OnBackendChanged(const QString& backend_name);

  X11Utils::XRRConfiguration* m_xrr_config;
  QTreeWidget* m_tree;
  QLineEdit* m_search_bar;
  QStackedWidget* m_widget_stack;

  SelectedPane m_selected_pane = SelectedPane::Default;
  bool m_controllers_enabled = true;
};
