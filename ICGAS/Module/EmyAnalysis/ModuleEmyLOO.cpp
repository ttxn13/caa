#include "pch.hpp"
#include "ModuleEmyLOO.hpp"

QMutex ModuleEmyLOO::mutex;
ModuleEmyLOO* ModuleEmyLOO::instance = nullptr;

ModuleEmyLOO* ModuleEmyLOO::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleEmyLOO();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleEmyLOO::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleEmyLOO::ModuleEmyLOO(QObject* parent)
	: QObject(parent)
{
}

ModuleEmyLOO::~ModuleEmyLOO()
{
}
