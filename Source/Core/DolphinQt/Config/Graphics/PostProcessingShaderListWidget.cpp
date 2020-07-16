// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DolphinQt/Config/Graphics/PostProcessingShaderListWidget.h"

#include <cmath>

#include <QCheckbox>
#include <QColorDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinbox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "Core/Config/GraphicsSettings.h"
#include "Core/ConfigManager.h"
#include "Core/Core.h"

#include "DolphinQt/Config/Graphics/GraphicsColor.h"
#include "DolphinQt/Config/Graphics/PostProcessingAddShaderDialog.h"
#include "DolphinQt/Settings.h"

#include "VideoCommon/VideoConfig.h"

PostProcessingShaderListWidget::PostProcessingShaderListWidget(QWidget* parent, ShaderType type)
    : QWidget(parent), m_type(type)
{
  CreateWidgets();
  ConnectWidgets();

  g_Config.LoadCustomShaderPresetFromConfig(true);
}

void PostProcessingShaderListWidget::CreateWidgets()
{
  auto* main_layout = new QHBoxLayout(this);

  auto* left_v_layout = new QVBoxLayout;

  m_shader_list = new QListWidget;
  m_shader_list->setSortingEnabled(false);
  m_shader_list->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectItems);
  m_shader_list->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  m_shader_list->setSelectionRectVisible(true);
  m_shader_list->setDragDropMode(QAbstractItemView::InternalMove);

  m_add_shader = new QPushButton(tr("Add..."));
  m_remove_shader = new QPushButton(tr("Remove"));
  QHBoxLayout* hlayout = new QHBoxLayout;
  hlayout->addStretch();
  hlayout->addWidget(m_add_shader);
  hlayout->addWidget(m_remove_shader);

  left_v_layout->addWidget(m_shader_list);
  left_v_layout->addLayout(hlayout);

  auto* right_v_layout = new QVBoxLayout;

  auto default_shader_text = tr("No shader selected");
  m_selected_shader_name = new QLabel(default_shader_text);
  right_v_layout->addWidget(m_selected_shader_name);

  m_shader_meta_layout = new QVBoxLayout;
  right_v_layout->addLayout(m_shader_meta_layout);

  m_shader_buttons_layout = new QHBoxLayout;
  right_v_layout->addLayout(m_shader_buttons_layout);

  m_passes_box = new QGroupBox(tr("Passes"));
  auto* passes_v_layout = new QVBoxLayout;
  m_passes_box->setLayout(passes_v_layout);
  right_v_layout->addWidget(m_passes_box);

  m_options_snapshots_box = new QGroupBox(tr("Snapshots"));
  auto* snapshots_v_layout = new QVBoxLayout;
  m_options_snapshots_box->setLayout(snapshots_v_layout);
  right_v_layout->addWidget(m_options_snapshots_box);

  m_options_box = new QGroupBox(tr("Options"));
  auto* options_v_layout = new QVBoxLayout;
  m_options_box->setLayout(options_v_layout);
  right_v_layout->addWidget(m_options_box);
  right_v_layout->addStretch(1);

  main_layout->addLayout(left_v_layout);
  main_layout->addLayout(right_v_layout, 1);

  setLayout(main_layout);
}

void PostProcessingShaderListWidget::ConnectWidgets()
{
  connect(m_shader_list, &QListWidget::itemSelectionChanged, this,
          [this] { ShaderSelectionChanged(); });

  connect(m_shader_list, &QListWidget::itemChanged, this,
          &PostProcessingShaderListWidget::ShaderItemChanged);

  connect(m_shader_list->model(), &QAbstractItemModel::rowsMoved, this,
          [this] { SaveShaderList(); });

  connect(m_add_shader, &QPushButton::clicked, this,
          &PostProcessingShaderListWidget::OnShaderAdded);
  connect(m_remove_shader, &QPushButton::clicked, this,
          &PostProcessingShaderListWidget::OnShaderRemoved);

  connect(&Settings::Instance(), &Settings::ShaderConfigLoaded, this,
          [this] { RefreshShaderList(); });
}

void PostProcessingShaderListWidget::RefreshShaderList()
{
  m_shader_list->setCurrentItem(nullptr);
  m_shader_list->clear();

  for (u32 i = 0; i < GetConfig()->GetShaderCount(); i++)
  {
    const auto& shader = GetConfig()->GetShader(i);
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(shader.GetShaderName()));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setData(Qt::UserRole, static_cast<int>(i));

    if (shader.IsEnabled())
    {
      item->setCheckState(Qt::Checked);
    }
    else
    {
      item->setCheckState(Qt::Unchecked);
    }
    m_shader_list->addItem(item);
  }
}

void PostProcessingShaderListWidget::ShaderSelectionChanged()
{
  if (m_shader_list->currentItem() == nullptr)
    return;
  if (m_shader_list->count() == 0)
    return;
  const int index = m_shader_list->currentItem()->data(Qt::UserRole).toInt();
  auto& shader = GetConfig()->GetShader(static_cast<u32>(index));
  OnShaderChanged(&shader);
}

void PostProcessingShaderListWidget::ShaderItemChanged(QListWidgetItem* item)
{
  const int index = item->data(Qt::UserRole).toInt();
  auto& shader = GetConfig()->GetShader(static_cast<u32>(index));
  const bool was_enabled = shader.IsEnabled();
  const bool should_enable = item->checkState() == Qt::Checked;
  shader.SetEnabled(should_enable);
  GetConfig()->IncrementChangeCount();
}

void PostProcessingShaderListWidget::OnShaderChanged(VideoCommon::PostProcessing::Shader* shader)
{
  QVBoxLayout* options_layout = static_cast<QVBoxLayout*>(m_options_box->layout());
  ClearLayoutRecursively(options_layout);

  QVBoxLayout* passes_layout = static_cast<QVBoxLayout*>(m_passes_box->layout());
  ClearLayoutRecursively(passes_layout);

  QVBoxLayout* snapshot_layout = static_cast<QVBoxLayout*>(m_options_snapshots_box->layout());
  ClearLayoutRecursively(snapshot_layout);

  ClearLayoutRecursively(m_shader_meta_layout);
  ClearLayoutRecursively(m_shader_buttons_layout);

  adjustSize();

  if (shader == nullptr)
  {
    m_selected_shader_name->setText(QStringLiteral("No shader selected"));
  }
  else
  {
    m_selected_shader_name->setText(QString::fromStdString(shader->GetShaderName()));
    QFont font = m_selected_shader_name->font();
    font.setWeight(QFont::Bold);
    m_selected_shader_name->setFont(font);

    if (!shader->GetAuthor().empty())
    {
      auto* author_label = new QLabel(QString::fromStdString(shader->GetAuthor()));
      m_shader_meta_layout->addWidget(author_label);
    }

    if (!shader->GetDescription().empty())
    {
      auto* description_label = new QLabel(QString::fromStdString(shader->GetDescription()));
      m_shader_meta_layout->addWidget(description_label);
    }

    auto reset_button = new QPushButton(tr("Reset to Defaults"));
    connect(reset_button, &QPushButton::clicked, [shader, this] {
      shader->Reset();
      GetConfig()->IncrementChangeCount();
      OnShaderChanged(shader);
    });

    auto reload_button = new QPushButton(tr("Reload"));
    connect(reload_button, &QPushButton::clicked, [shader, this] {
      shader->Reload();
      GetConfig()->IncrementChangeCount();
      OnShaderChanged(shader);
    });

    m_shader_buttons_layout->addWidget(reset_button);
    m_shader_buttons_layout->addWidget(reload_button);

    const bool has_error = shader->GetRuntimeInfo() && shader->GetRuntimeInfo()->HasError();

    if (std::none_of(
            shader->GetPasses().begin(), shader->GetPasses().end(),
            [](const VideoCommon::PostProcessing::Pass& pass) { return pass.gui_name != ""; }))
    {
      passes_layout->addWidget(new QLabel(QStringLiteral("No passes are editable")));
    }
    else
    {
      if (has_error)
      {
        passes_layout->addWidget(
            new QLabel(QStringLiteral("Shader error, fix and reload to edit passes")));
      }
      else
      {
        passes_layout->addLayout(BuildPassesLayout(shader));
      }
    }

    if (shader->GetOptions().empty())
    {
      options_layout->addWidget(new QLabel(QStringLiteral("No options available")));
    }
    else
    {
      if (has_error)
      {
        options_layout->addWidget(
            new QLabel(QStringLiteral("Shader error, fix and reload to edit options")));
      }
      else
      {
        snapshot_layout->addLayout(BuildSnapshotsLayout(shader));
        options_layout->addLayout(BuildOptionsLayout(shader));
      }
    }
  }
}

QLayout*
PostProcessingShaderListWidget::BuildPassesLayout(VideoCommon::PostProcessing::Shader* shader)
{
  QGridLayout* pass_layout = new QGridLayout;
  for (u32 i = 0; i < shader->GetPassCount(); i++)
  {
    auto& pass = shader->GetPass(i);
    pass_layout->addWidget(new QLabel(QString::fromStdString(pass.gui_name)), 2 * i, 0);

    auto* pass_options_layout = new QHBoxLayout;
    pass_options_layout->addWidget(new QLabel(QStringLiteral("Scale")));

    auto box = new QDoubleSpinBox;
    box->setValue(pass.output_scale);
    connect(box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [shader, &pass, this](double value) mutable {
              pass.output_scale = static_cast<float>(value);
              GetConfig()->IncrementChangeCount();
            });
    pass_options_layout->addWidget(box);

    pass_layout->addLayout(pass_options_layout, 2 * i + 1, 0);
  }
  return pass_layout;
}

QLayout*
PostProcessingShaderListWidget::BuildSnapshotsLayout(VideoCommon::PostProcessing::Shader* shader)
{
  QGridLayout* snapshots_layout = new QGridLayout;

  const int max_snapshots =
      static_cast<int>(VideoCommon::PostProcessing::Shader::SnapshotSlot::Max);

  for (int column = 0; column < max_snapshots; column++)
  {
    auto* label = new QLabel(QString::fromStdString(std::to_string(column + 1)));
    snapshots_layout->addWidget(label, 0, column, Qt::AlignCenter);
  }

  for (int row = 1; row < 3; row++)
  {
    for (int column = 0; column < max_snapshots; column++)
    {
      QPushButton* button;
      if (row == 1)
      {
        button = new QPushButton(tr("L"));
        if (shader->IsSlotSet(
                static_cast<VideoCommon::PostProcessing::Shader::SnapshotSlot>(column)))
        {
          button->setEnabled(true);
          connect(button, &QPushButton::clicked, this, [shader, column, this]() {
            shader->LoadFromSlot(
                static_cast<VideoCommon::PostProcessing::Shader::SnapshotSlot>(column));
            OnShaderChanged(shader);
            GetConfig()->IncrementChangeCount();
          });
        }
        else
        {
          button->setEnabled(false);
        }
      }
      else
      {
        button = new QPushButton(tr("S"));
        connect(button, &QPushButton::clicked, this, [shader, column, this]() {
          shader->SetToSlot(static_cast<VideoCommon::PostProcessing::Shader::SnapshotSlot>(column));
          OnShaderChanged(shader);
        });
      }

      button->setMaximumWidth(button->fontMetrics().boundingRect(button->text()).width() * 4);

      snapshots_layout->addWidget(button, row, column, Qt::AlignCenter);
    }
  }

  return snapshots_layout;
}

QLayout*
PostProcessingShaderListWidget::BuildOptionsLayout(VideoCommon::PostProcessing::Shader* shader)
{
  std::array<QString, 4> vector_labels = {QStringLiteral("X"), QStringLiteral("Y"),
                                          QStringLiteral("Z"), QStringLiteral("W")};
  int rows = 0;

  QVBoxLayout* options_layout = new QVBoxLayout;

  QTabWidget* tabs_widget = new QTabWidget;

  connect(tabs_widget, &QTabWidget::currentChanged, this, [tabs_widget, this](int index) {
    for (int i = 0; i < tabs_widget->count(); i++)
    {
      if (i != index)
        tabs_widget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    auto* widget = tabs_widget->widget(index);
    widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    widget->resize(widget->minimumSizeHint());
    widget->adjustSize();
    resize(minimumSizeHint());
    adjustSize();
  });

  options_layout->addWidget(tabs_widget);

  using namespace VideoCommon::PostProcessing;
  for (auto& [group_name, options] : shader->GetGroups())
  {
    QWidget* group_widget = new QWidget;
    QGridLayout* option_layout = new QGridLayout;

    group_widget->setLayout(option_layout);

    tabs_widget->addTab(group_widget, QString::fromStdString(group_name));

    for (auto& option_name : options)
    {
      Option& option = *shader->FindOption(option_name.data());
      switch (option.type)
      {
      case OptionType::Bool:
      {
        auto* checkbox = new QCheckBox;
        checkbox->setChecked(option.value.bool_value);
        connect(checkbox, &QCheckBox::stateChanged, this, [shader, &option, this](int state) {
          shader->SetOptionb(option, state == 2);

          if (option.compile_time_constant || option.is_pass_dependent_option)
            GetConfig()->IncrementChangeCount();
        });
        option_layout->addWidget(new QLabel(QString::fromStdString(option.gui_name)), rows, 0);
        option_layout->addWidget(checkbox, rows, 1);
      }
      break;
      case OptionType::Float:
      case OptionType::Float2:
      case OptionType::Float3:
      case OptionType::Float4:
      {
        option_layout->addWidget(new QLabel(QString::fromStdString(option.gui_name)), rows, 0);

        auto* values_layout = new QHBoxLayout;
        option_layout->addLayout(values_layout, rows, 1);
        for (std::size_t i = 0; i < Option::GetComponents(option.type); i++)
        {
          if (Option::GetComponents(option.type) > 1)
          {
            auto axis_label = new QLabel(vector_labels[i] + QStringLiteral(":"));
            values_layout->addWidget(axis_label);
          }
          auto box = new QDoubleSpinBox;
          box->setValue(option.value.float_values[i]);
          box->setMinimum(option.min_value.float_values[i]);
          box->setMaximum(option.max_value.float_values[i]);
          box->setSingleStep(option.step_amount.float_values[i]);
          box->setDecimals(5);
          connect(box, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                  [shader, &option, i, this](double value) mutable {
                    shader->SetOptionf(option, static_cast<int>(i), static_cast<float>(value));

                    if (option.compile_time_constant || option.is_pass_dependent_option)
                      GetConfig()->IncrementChangeCount();
                  });
          values_layout->addWidget(box);
        }
      }
      break;
      case OptionType::Float16:
      {
        // TODO:
      }
      break;
      case OptionType::Int:
      case OptionType::Int2:
      case OptionType::Int3:
      case OptionType::Int4:
      {
        option_layout->addWidget(new QLabel(QString::fromStdString(option.gui_name)), rows, 0);

        auto* values_layout = new QHBoxLayout;
        option_layout->addLayout(values_layout, rows, 1);
        for (std::size_t i = 0; i < Option::GetComponents(option.type); i++)
        {
          if (Option::GetComponents(option.type) > 1)
          {
            auto axis_label = new QLabel(vector_labels[i] + QStringLiteral(":"));
            values_layout->addWidget(axis_label);
          }
          auto box = new QSpinBox;
          box->setValue(option.value.int_values[i]);
          box->setMinimum(option.min_value.int_values[i]);
          box->setMaximum(option.max_value.int_values[i]);
          box->setSingleStep(option.step_amount.int_values[i]);
          connect(box, QOverload<int>::of(&QSpinBox::valueChanged), this,
                  [&shader, &option, i, this](int value) {
                    shader->SetOptioni(option, static_cast<int>(i), value);

                    if (option.compile_time_constant || option.is_pass_dependent_option)
                      GetConfig()->IncrementChangeCount();
                  });
          values_layout->addWidget(box);
        }
      }
      break;
      case OptionType::RGB:
      {
        option_layout->addWidget(new QLabel(QString::fromStdString(option.gui_name)), rows, 0);

        auto* color_button = new GraphicsColor(this, false);

        QColor initial_color;
        initial_color.setRgbF(option.value.float_values[0], option.value.float_values[1],
                              option.value.float_values[2]);
        color_button->SetColor(initial_color);
        connect(color_button, &GraphicsColor::ColorIsUpdated, this,
                [&shader, &option, this, color_button]() {
                  const auto& color = color_button->GetColor();
                  shader->SetOptionf(option, 0, static_cast<float>(color.redF()));
                  shader->SetOptionf(option, 1, static_cast<float>(color.greenF()));
                  shader->SetOptionf(option, 2, static_cast<float>(color.blueF()));

                  if (option.compile_time_constant || option.is_pass_dependent_option)
                    GetConfig()->IncrementChangeCount();
                });
        option_layout->addWidget(color_button, rows, 1);
      }
      break;
      case OptionType::RGBA:
      {
        option_layout->addWidget(new QLabel(QString::fromStdString(option.gui_name)), rows, 0);

        auto* color_button = new GraphicsColor(this, false);
        QColor initial_color;
        initial_color.setRgbF(option.value.float_values[0], option.value.float_values[1],
                              option.value.float_values[2], option.value.float_values[3]);
        color_button->SetColor(initial_color);

        connect(color_button, &GraphicsColor::ColorIsUpdated, this,
                [&shader, &option, this, color_button]() {
                  const auto& color = color_button->GetColor();
                  shader->SetOptionf(option, 0, static_cast<float>(color.redF()));
                  shader->SetOptionf(option, 1, static_cast<float>(color.greenF()));
                  shader->SetOptionf(option, 2, static_cast<float>(color.blueF()));
                  shader->SetOptionf(option, 3, static_cast<float>(color.alphaF()));

                  if (option.compile_time_constant || option.is_pass_dependent_option)
                    GetConfig()->IncrementChangeCount();
                });
        option_layout->addWidget(color_button, rows, 1);
      }
      break;
      }
      rows++;
    }
  }

  return options_layout;
}

void PostProcessingShaderListWidget::SaveShaderList()
{
  std::vector<u32> shader_order;
  shader_order.resize(m_shader_list->count());
  for (int i = 0; i < m_shader_list->count(); ++i)
  {
    QListWidgetItem* item = m_shader_list->item(i);
    const int index = item->data(Qt::UserRole).toInt();
    shader_order[i] = static_cast<u32>(index);
  }

  GetConfig()->SetShaderIndices(shader_order);
  GetConfig()->IncrementChangeCount();
}

void PostProcessingShaderListWidget::ClearLayoutRecursively(QLayout* layout)
{
  while (QLayoutItem* child = layout->takeAt(0))
  {
    if (child == nullptr)
      continue;

    if (child->widget())
    {
      layout->removeWidget(child->widget());
      delete child->widget();
    }
    else if (child->layout())
    {
      ClearLayoutRecursively(child->layout());
      layout->removeItem(child);
    }
    else
    {
      layout->removeItem(child);
    }
    delete child;
  }
}

void PostProcessingShaderListWidget::OnShaderAdded()
{
  PostProcessingAddShaderDialog add_shader_dialog(this);
  add_shader_dialog.setModal(true);
  if (add_shader_dialog.exec() == QDialog::Accepted)
  {
    const auto shader_pathes = add_shader_dialog.ChosenShaderPathes();
    if (!shader_pathes.empty())
    {
      for (const auto& shader_path : shader_pathes)
      {
        GetConfig()->AddShader(shader_path.toStdString());
      }

      RefreshShaderList();
    }
  }
}

void PostProcessingShaderListWidget::OnShaderRemoved()
{
  if (m_shader_list->currentItem() == nullptr)
    return;
  const int index = m_shader_list->currentItem()->data(Qt::UserRole).toInt();
  GetConfig()->RemoveShader(static_cast<u32>(index));

  RefreshShaderList();
  OnShaderChanged(nullptr);
}

VideoCommon::PostProcessing::Config* PostProcessingShaderListWidget::GetConfig() const
{
  if (m_type == ShaderType::Textures)
  {
    return &g_Config.shader_config_textures;
  }
  else if (m_type == ShaderType::EFB)
  {
    return &g_Config.shader_config_efb;
  }
  else if (m_type == ShaderType::XFB)
  {
    return &g_Config.shader_config_xfb;
  }
  else if (m_type == ShaderType::Post)
  {
    return &g_Config.shader_config_post_process;
  }

  return nullptr;
}
