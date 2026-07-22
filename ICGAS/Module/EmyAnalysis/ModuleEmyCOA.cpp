#include "pch.hpp"
#include "ModuleEmyCOA.hpp"

QMutex ModuleEmyCOA::mutex;
ModuleEmyCOA* ModuleEmyCOA::instance = nullptr;

ModuleEmyCOA* ModuleEmyCOA::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleEmyCOA();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleEmyCOA::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleEmyCOA::ModuleEmyCOA(QObject* parent)
	: QObject(parent)
{
}

ModuleEmyCOA::~ModuleEmyCOA()
{
}
