#include "pch.hpp"
#include "ModuleOurGroup.hpp"

QMutex ModuleOurGroup::mutex;
ModuleOurGroup* ModuleOurGroup::instance = nullptr;

ModuleOurGroup* ModuleOurGroup::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleOurGroup();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleOurGroup::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleOurGroup::ModuleOurGroup(QObject* parent)
	: QObject(parent)
{
}

ModuleOurGroup::~ModuleOurGroup()
{
}
