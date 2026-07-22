#include "pch.hpp"
#include "ModuleEmyCOG.hpp"

QMutex ModuleEmyCOG::mutex;
ModuleEmyCOG* ModuleEmyCOG::instance = nullptr;

ModuleEmyCOG* ModuleEmyCOG::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleEmyCOG();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleEmyCOG::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleEmyCOG::ModuleEmyCOG(QObject* parent)
	: QObject(parent)
{
}

ModuleEmyCOG::~ModuleEmyCOG()
{
}
