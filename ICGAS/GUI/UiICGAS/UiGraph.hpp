#pragma once
#include "pch.hpp"

class UiICGAS;

class UiGraph : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QObject* backend READ backend CONSTANT)

public:
	explicit UiGraph(UiICGAS* backend, QObject* parent = nullptr);

	QObject* backend() const;

private:
	UiICGAS* ui_icgas = nullptr;
};
