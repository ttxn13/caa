#include "pch.hpp"
#include "UiWorkflow.hpp"
#include "UiICGAS.hpp"

UiWorkflow::UiWorkflow(UiICGAS* backend, QObject* parent)
	: QObject(parent),
	ui_icgas(backend)
{
}

QObject* UiWorkflow::backend() const
{
	return ui_icgas;
}
