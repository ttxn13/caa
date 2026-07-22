#include "pch.hpp"
#include "ModuleGraph.hpp"

QMutex ModuleGraph::mutex;
ModuleGraph* ModuleGraph::instance = nullptr;

ModuleGraph* ModuleGraph::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleGraph();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleGraph::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleGraph::ModuleGraph(QObject* parent)
	: QObject(parent)
{
}

ModuleGraph::~ModuleGraph()
{
}

void ModuleGraph::update_concept_graph(const S_CONCEPT_GRAPH& graph)
{
	QMutexLocker locker(&data_mutex);
	concept_graph = graph;
}

S_CONCEPT_GRAPH ModuleGraph::concept_graph_snapshot() const
{
	QMutexLocker locker(&data_mutex);
	return concept_graph;
}
