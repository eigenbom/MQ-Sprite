#include "propertieswidget.h"
#include "ui_propertieswidget.h"

#include "commands.h"
#include "partwidget.h"

PropertiesWidget::PropertiesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PropertiesWidget)
{
    ui->setupUi(this);

	mTextEditProperties = findChild<QPlainTextEdit*>("textEditProperties");
	connect(mTextEditProperties, SIGNAL(textChanged()), this, SLOT(textPropertiesEdited()));
	mTextEditProperties->setPlaceholderText(tr("\"emit\": true,\n\"wiggle\": 0.5,\n\"display_name\":\"worm\""));

	this->setEnabled(false);
}

PropertiesWidget::~PropertiesWidget()
{
    delete ui;
}

PartWidget* PropertiesWidget::targetPartWidget() {
	return mTarget;
}

void PropertiesWidget::setTargetPartWidget(PartWidget* p) {
	if (mTarget) {
		mTarget = nullptr;
		mCurrentMode.clear();
	}

	if (p) {
		mTarget = p;
		Part* part = PM()->getPart(p->partRef());
		Q_ASSERT(part);
		setProperties(part->properties);
		QTextCursor cursor = mTextEditProperties->textCursor();
		cursor.movePosition(QTextCursor::Start);
		this->setEnabled(true);
	}
	else {
		this->setEnabled(false);
	}
}

void PropertiesWidget::textPropertiesEdited() {
	if (mTarget) {
		TryCommand(new CChangePartProperties(mTarget->partRef(), mTextEditProperties->toPlainText()));
	}
}

void PropertiesWidget::targetPartPropertiesChanged() {
	if (mTarget) {
		Part* part = PM()->getPart(mTarget->partRef());
		if (part) {
			setProperties(part->properties);
		}
	}
}

void PropertiesWidget::setProperties(const QString& properties) {
	// TODO: Format it ?

	// Remove start and end
	int cursorPos = mTextEditProperties->textCursor().position();

	QString text = properties;

	mTextEditProperties->blockSignals(true);
	mTextEditProperties->setPlainText(text);
	mTextEditProperties->blockSignals(false);

	QTextCursor cursor = mTextEditProperties->textCursor();
	cursor.setPosition(cursorPos, QTextCursor::MoveAnchor);
	mTextEditProperties->setTextCursor(cursor);
}