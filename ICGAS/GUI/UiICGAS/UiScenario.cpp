#include "pch.hpp"
#include "UiScenario.hpp"
#include "UiICGAS.hpp"

UiScenario::UiScenario(UiICGAS* backend, QObject* parent)
	: QObject(parent),
	ui_icgas(backend)
{
}

QObject* UiScenario::backend() const
{
	return ui_icgas;
}
