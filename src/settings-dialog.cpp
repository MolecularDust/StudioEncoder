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

	for (int pass = 0; pass < 2; pass++) {
		const bool experimental = pass == 1;
		QVBoxLayout *target = layout;
		if (experimental) {
			/* The Experimental section is a collapsible group, collapsed by default. */
			auto *toggle = new QToolButton;
			toggle->setText(T("Settings.Experimental"));
			toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			toggle->setArrowType(Qt::RightArrow);
			toggle->setCheckable(true);
			toggle->setChecked(false);
			toggle->setAutoRaise(true);
			QFont font = toggle->font();
			font.setBold(true);
			toggle->setFont(font);
			layout->addSpacing(10);
			layout->addWidget(toggle, 0, Qt::AlignLeft);

			auto *section = new QWidget;
			section->setVisible(false);
			target = new QVBoxLayout(section);
			target->setContentsMargins(0, 0, 0, 0);
			layout->addWidget(section);
			QObject::connect(toggle, &QToolButton::toggled, &dialog, [toggle, section, &dialog](bool open) {
				toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
				section->setVisible(open);
				dialog.adjustSize();
			});
			target->addWidget(make_note(T("Settings.ExperimentalNote"), 0));
		} else {
			layout->addWidget(make_heading(T("Settings.Recommended")));
		}

		for (size_t i = 0; i < count; i++) {
			if (entries[i].experimental != experimental)
				continue;
			const char *name = obs_encoder_get_display_name(entries[i].id);
			auto *box = new QCheckBox(QString::fromUtf8(name ? name : entries[i].id));
			const bool available = pr_encoder_available(&entries[i]);
			box->setChecked(available && pr_encoder_enabled(&entries[i]));
			box->setEnabled(available);
			boxes[i] = box;
			target->addWidget(box);
			target->addWidget(make_note(
				available ? T(entries[i].desc_key) : T(entries[i].desc_key) + QStringLiteral(" ") +
						T(entries[i].unavailable_key ? entries[i].unavailable_key : "Settings.Unavailable"),
				24));
		}
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
