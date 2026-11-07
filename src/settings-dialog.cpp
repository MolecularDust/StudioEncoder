#include "settings.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

static QString T(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

static QLabel *make_heading(const QString &text)
{
	auto *label = new QLabel(text);
	QFont font = label->font();
	font.setBold(true);
	label->setFont(font);
	label->setContentsMargins(0, 10, 0, 0);
	return label;
}

static QLabel *make_note(const QString &text, int left_margin)
{
	auto *label = new QLabel(text);
	label->setWordWrap(true);
	label->setForegroundRole(QPalette::PlaceholderText);
	label->setContentsMargins(left_margin, 0, 0, 4);
	return label;
}

static void show_settings(void *)
{
	QWidget *parent = static_cast<QWidget *>(obs_frontend_get_main_window());

	QDialog dialog(parent);
	dialog.setWindowTitle(T("Settings.Title"));
	dialog.setMinimumWidth(540);
	auto *layout = new QVBoxLayout(&dialog);

	auto *intro = new QLabel(T("Settings.Intro"));
	intro->setWordWrap(true);
	layout->addWidget(intro);

	size_t count = 0;
	const struct pr_encoder_entry *entries = pr_encoder_entries(&count);
	std::vector<QCheckBox *> boxes(count, nullptr);

	/* one checkbox and its description for encoder i, added to the given layout */
	auto add_entry = [&](size_t i, QVBoxLayout *into) {
		const char *name = obs_encoder_get_display_name(entries[i].id);
		auto *box = new QCheckBox(QString::fromUtf8(name ? name : entries[i].id));
		const bool available = pr_encoder_available(&entries[i]);
		box->setChecked(available && pr_encoder_enabled(&entries[i]));
		box->setEnabled(available);
		boxes[i] = box;
		into->addWidget(box);
		into->addWidget(make_note(
			available ? T(entries[i].desc_key)
				  : T(entries[i].desc_key) + QStringLiteral(" ") +
					    T(entries[i].unavailable_key ? entries[i].unavailable_key : "Settings.Unavailable"),
			24));
	};

	/* 0 = recommended 10-bit 4:2:2, 1 = 10-bit 4:2:0 (collapsible, closed at first), 2 = experimental */
	auto group_of = [&](size_t i) { return entries[i].experimental ? 2 : (entries[i].yuv420 ? 1 : 0); };

	for (int group = 0; group < 3; group++) {
		size_t members = 0;
		for (size_t i = 0; i < count; i++)
			members += group_of(i) == group;
		if (!members)
			continue;

		if (group == 1) {
			auto *toggle = new QToolButton();
			toggle->setText(T("Settings.Group420"));
			toggle->setCheckable(true);
			toggle->setChecked(false);
			toggle->setArrowType(Qt::RightArrow);
			toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			toggle->setAutoRaise(true);
			QFont font = toggle->font();
			font.setBold(true);
			toggle->setFont(font);
			layout->addSpacing(10);
			layout->addWidget(toggle, 0, Qt::AlignLeft);

			auto *content = new QWidget();
			auto *inner = new QVBoxLayout(content);
			inner->setContentsMargins(18, 0, 0, 0);
			inner->addWidget(make_note(T("Settings.Group420Note"), 0));
			for (size_t i = 0; i < count; i++)
				if (group_of(i) == group)
					add_entry(i, inner);
			content->setVisible(false);
			layout->addWidget(content);
			QObject::connect(toggle, &QToolButton::toggled, &dialog, [toggle, content, &dialog](bool open) {
				toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
				content->setVisible(open);
				dialog.adjustSize();
			});
			continue;
		}

		layout->addWidget(make_heading(T(group == 2 ? "Settings.Experimental" : "Settings.Recommended")));
		if (group == 2)
			layout->addWidget(make_note(T("Settings.ExperimentalNote"), 0));
		for (size_t i = 0; i < count; i++)
			if (group_of(i) == group)
				add_entry(i, layout);
	}

	layout->addSpacing(8);
	auto *version = new QLabel(T("Settings.Version").arg(QString::fromUtf8(PLUGIN_VERSION)));
	version->setForegroundRole(QPalette::PlaceholderText);
	layout->addWidget(version);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	layout->addWidget(buttons);

	if (dialog.exec() != QDialog::Accepted)
		return;

	bool changed = false;
	for (size_t i = 0; i < count; i++) {
		if (!boxes[i] || !pr_encoder_available(&entries[i]))
			continue; /* an unavailable encoder keeps whatever was saved */
		if (boxes[i]->isChecked() != pr_encoder_enabled(&entries[i]))
			changed = true;
		pr_encoder_set_enabled(&entries[i], boxes[i]->isChecked());
	}
	pr_settings_save();
	if (changed)
		QMessageBox::information(parent, T("Settings.Title"), T("Settings.Restart"));
}

extern "C" void pr_ui_init(void)
{
	obs_frontend_add_tools_menu_item(obs_module_text("Settings.Menu"), show_settings, nullptr);
}
