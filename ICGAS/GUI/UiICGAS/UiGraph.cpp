#include "pch.hpp"
#include "UiGraph.hpp"
#include "UiICGAS.hpp"

UiGraph::UiGraph(UiICGAS* backend, QObject* parent)
	: QObject(parent),
	ui_icgas(backend)
{
}

QObject* UiGraph::backend() const
{
	return ui_icgas;
}
