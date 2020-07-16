// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QDialog>
#include <QString>

class QComboBox;
class QGroupBox;
class QDialogButtonBox;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QTabWidget;
class QWidget;

class CustomShaderWindow final : public QDialog
{
  Q_OBJECT
public:
  explicit CustomShaderWindow(QWidget* parent);

private:
  void CreateProfilesLayout();
  void CreateTabs();
  void CreateMainLayout();
  void ConnectWidgets();

  void OnSelectProfile(int index);
  void OnProfileTextChanged(const QString& text);
  void OnDeleteProfilePressed();
  void OnLoadProfilePressed();
  void OnSaveProfilePressed();
  void UpdateProfileIndex();
  void UpdateProfileButtonState();
  void PopulateProfileSelection();
  
  // Main
  QVBoxLayout* m_main_layout;
  QHBoxLayout* m_config_layout;
  QDialogButtonBox* m_button_box;

  // Profiles
  QGroupBox* m_profiles_box;
  QHBoxLayout* m_profiles_layout;
  QComboBox* m_profiles_combo;
  QPushButton* m_profiles_load;
  QPushButton* m_profiles_save;
  QPushButton* m_profiles_delete;
  
  QTabWidget* m_tab_widget;
};
