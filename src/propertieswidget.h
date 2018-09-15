#ifndef PROPERTIESWIDGET_H
#define PROPERTIESWIDGET_H

#include <QWidget>
class QPlainTextEdit;

class PartWidget;

namespace Ui {
class PropertiesWidget;
}

class PropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesWidget(QWidget *parent = nullptr);
    ~PropertiesWidget();

	PartWidget* targetPartWidget();
	void setTargetPartWidget(PartWidget* p); // if p is null then no target widget

public slots:
	void textPropertiesEdited();
	void targetPartPropertiesChanged();

private:
    Ui::PropertiesWidget *ui = nullptr;
	PartWidget* mTarget = nullptr;
	QString mCurrentMode {};
	QPlainTextEdit* mTextEditProperties = nullptr;
};

#endif // PROPERTIESWIDGET_H
