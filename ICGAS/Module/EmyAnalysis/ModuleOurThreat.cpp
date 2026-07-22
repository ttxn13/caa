#include "pch.hpp"
#include "ModuleOurThreat.hpp"

QMutex ModuleOurThreat::mutex;
ModuleOurThreat* ModuleOurThreat::instance = nullptr;

ModuleOurThreat* ModuleOurThreat::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleOurThreat();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleOurThreat::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleOurThreat::ModuleOurThreat(QObject* parent)
	: QObject(parent)
{
}

ModuleOurThreat::~ModuleOurThreat()
{
}
