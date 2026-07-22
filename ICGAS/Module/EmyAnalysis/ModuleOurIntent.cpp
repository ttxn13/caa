#include "pch.hpp"
#include "ModuleOurIntent.hpp"

QMutex ModuleOurIntent::mutex;
ModuleOurIntent* ModuleOurIntent::instance = nullptr;

ModuleOurIntent* ModuleOurIntent::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleOurIntent();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleOurIntent::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleOurIntent::ModuleOurIntent(QObject* parent)
	: QObject(parent)
{
}

ModuleOurIntent::~ModuleOurIntent()
{
}
