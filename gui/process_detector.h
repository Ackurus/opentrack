/* Copyright (c) 2015, Stanislaw Halik <sthalik@misaki.pl>

 * Permission to use, copy, modify, and/or distribute this
 * software for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice and this permission
 * notice appear in all copies.
 */

#pragma once

#include "export.hpp"

#include <QObject>
#include <QWidget>
#include <QTableWidget>

#include "gui/ui_process_widget.h"
#include "process-detector-fancy-table.hpp"
#include "options/options.hpp"

struct OTR_GUI_EXPORT proc_detector_settings
{
    QHash<QString, QString> split_process_names();
    QString get_game_list();
    void set_game_list(const QString& game_list);
    bool is_enabled();
    void set_is_enabled(bool enabled);
};

class OTR_GUI_EXPORT process_detector final : public QWidget
{
    Q_OBJECT

    Ui_process_detector ui;
    proc_detector_settings s;

    int add_row(const QString& exe_name = "...", const QString& profile = "");
    void load_rows();
public:
    process_detector(QWidget* parent = nullptr);
public slots:
    void save();
    void revert();
private slots:
    void add();
    void remove();
};

class BrowseButton : public QPushButton
{
    Q_OBJECT
    QTableWidgetItem* twi;
public:
    BrowseButton(QTableWidgetItem* twi) : twi(twi)
    {}
public slots:
    void browse();
};

class OTR_GUI_EXPORT process_detector_worker : QObject
{
    Q_OBJECT
    proc_detector_settings s;
    QString last_exe_name;
public:
    bool profile_to_start(QString& str);
    bool should_stop();
};

