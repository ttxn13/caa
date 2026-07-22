#include "pch.hpp"
#include "ModuleEmyIntent.hpp"

QMutex ModuleEmyIntent::mutex;
ModuleEmyIntent* ModuleEmyIntent::instance = nullptr;

ModuleEmyIntent* ModuleEmyIntent::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleEmyIntent();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleEmyIntent::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleEmyIntent::ModuleEmyIntent(QObject* parent)
	: QObject(parent)
{
}

ModuleEmyIntent::~ModuleEmyIntent()
{
}
