#pragma once
#include "pch.hpp"
#include "../../Core/Comm/CoreGraph.hpp"

class ModuleGraph : public QObject
{
	Q_OBJECT

public:
	static ModuleGraph* GetInstance();
	static void DstInstance();

public:
	void update_concept_graph(const S_CONCEPT_GRAPH& graph);
	S_CONCEPT_GRAPH concept_graph_snapshot() const;

private:
	explicit ModuleGraph(QObject* parent = nullptr);
	~ModuleGraph() override;

	mutable QMutex data_mutex;
	S_CONCEPT_GRAPH concept_graph;

private:
	static QMutex mutex;
	static ModuleGraph* instance;
};
