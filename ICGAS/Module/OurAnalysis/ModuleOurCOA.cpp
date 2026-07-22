#include "pch.hpp"
#include "ModuleOurCOA.hpp"

QMutex ModuleOurCOA::mutex;
ModuleOurCOA* ModuleOurCOA::instance = nullptr;

ModuleOurCOA* ModuleOurCOA::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleOurCOA();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleOurCOA::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleOurCOA::ModuleOurCOA(QObject* parent)
	: QObject(parent)
{
}

ModuleOurCOA::~ModuleOurCOA()
{
}
