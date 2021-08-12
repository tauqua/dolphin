// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DolphinQt/Config/Graphics/PostProcessingAddShaderDialog.h"

#include <string>

#include <fmt/format.h>

#include <QDialogButtonBox>
#include <QDir>
#include <QPushButton>
#include <QString>
#include <QTabWidget>
#include <QVBoxLayout>

#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/FileSearch.h"
#include "Common/FileUtil.h"

#include "DolphinQt/Config/Graphics/PostProcessingAddShaderListWidget.h"
#include "VideoCommon/PEShaderSystem/Constants.h"

PostProcessingAddShaderDialog::PostProcessingAddShaderDialog(QWidget* parent) : QDialog(parent)
{
  CreateWidgets();
  setLayout(m_main_layout);
}

void PostProcessingAddShaderDialog::CreateWidgets()
{
  setWindowTitle(tr("Add New Shader"));
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  m_main_layout = new QVBoxLayout;

  const auto sys_shader_dir = QDir{QString::fromStdString(
      fmt::format("{}/{}", File::GetSysDirectory(),
                  VideoCommon::PE::Constants::dolphin_shipped_public_shader_directory))};
  const auto user_shader_dir = QDir{QString::fromStdString(File::GetUserPath(D_SHADERS_IDX))};

  m_system_shader_list = new PostProcessingAddShaderListWidget(this, sys_shader_dir, false);
  m_user_shader_list = new PostProcessingAddShaderListWidget(this, user_shader_dir, true);

  m_shader_tabs = new QTabWidget;
  m_shader_tabs->addTab(m_system_shader_list, tr("System"));
  m_shader_tabs->addTab(m_user_shader_list, tr("User"));
  m_main_layout->addWidget(m_shader_tabs);

  m_buttonbox = new QDialogButtonBox();
  auto* add_button = new QPushButton(tr("Add"));
  auto* cancel_button = new QPushButton(tr("Cancel"));
  m_buttonbox->addButton(add_button, QDialogButtonBox::AcceptRole);
  m_buttonbox->addButton(cancel_button, QDialogButtonBox::RejectRole);
  connect(add_button, &QPushButton::clicked, this, &PostProcessingAddShaderDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &PostProcessingAddShaderDialog::reject);
  add_button->setDefault(true);

  m_main_layout->addWidget(m_buttonbox);
}

std::vector<QString> PostProcessingAddShaderDialog::ChosenShaderPathes() const
{
  const auto system_shaders = m_system_shader_list->ChosenShaderPathes();
  const auto user_shaders = m_user_shader_list->ChosenShaderPathes();
  std::vector<QString> result;
  result.reserve(system_shaders.size() + user_shaders.size());
  result.insert(result.end(), system_shaders.begin(), system_shaders.end());
  result.insert(result.end(), user_shaders.begin(), user_shaders.end());
  return result;
}
