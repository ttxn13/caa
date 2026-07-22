#pragma once
#include "pch.hpp"

class UiICGAS;

class UiScenario : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QObject* backend READ backend CONSTANT)

public:
	explicit UiScenario(UiICGAS* backend, QObject* parent = nullptr);

	QObject* backend() const;

private:
	UiICGAS* ui_icgas = nullptr;
};
